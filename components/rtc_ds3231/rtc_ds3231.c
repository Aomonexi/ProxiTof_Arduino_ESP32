/**
 * rtc_ds3231.c
 * 
 * DS3231 Real-Time Clock Implementation
 */

#include "rtc_ds3231.h"
#include "esp_log.h"
#include "driver/i2c.h"
#include <sys/time.h>
#include <string.h>

static const char *TAG = "DS3231";

// DS3231 I2C address
#define DS3231_ADDR 0x68

// DS3231 register addresses
#define DS3231_REG_SECONDS    0x00
#define DS3231_REG_MINUTES    0x01
#define DS3231_REG_HOURS      0x02
#define DS3231_REG_DAY        0x03
#define DS3231_REG_DATE       0x04
#define DS3231_REG_MONTH      0x05
#define DS3231_REG_YEAR       0x06
#define DS3231_REG_CONTROL    0x0E
#define DS3231_REG_STATUS     0x0F
#define DS3231_REG_TEMP_MSB   0x11
#define DS3231_REG_TEMP_LSB   0x12

static i2c_master_dev_handle_t ds3231_handle = NULL;

// Convert decimal to BCD
static uint8_t dec_to_bcd(uint8_t dec) {
    return ((dec / 10) << 4) | (dec % 10);
}

// Convert BCD to decimal
static uint8_t bcd_to_dec(uint8_t bcd) {
    return ((bcd >> 4) * 10) + (bcd & 0x0F);
}

esp_err_t rtc_ds3231_init(int sda_pin, int scl_pin) {
    ESP_LOGI(TAG, "Initializing DS3231 RTC...");
    ESP_LOGI(TAG, "I2C pins: SDA=%d, SCL=%d", sda_pin, scl_pin);
    
    // Configure I2C master
    i2c_master_bus_config_t bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM_1,  // Use I2C1 (separate from sensors on I2C0)
        .scl_io_num = scl_pin,
        .sda_io_num = sda_pin,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    
    i2c_master_bus_handle_t bus_handle;
    esp_err_t ret = i2c_new_master_bus(&bus_config, &bus_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create I2C bus: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Configure DS3231 device
    i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = DS3231_ADDR,
        .scl_speed_hz = 100000,  // 100kHz
    };
    
    ret = i2c_master_bus_add_device(bus_handle, &dev_config, &ds3231_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add DS3231 device: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Test communication
    if (!rtc_ds3231_is_available()) {
        ESP_LOGE(TAG, "DS3231 not responding at address 0x%02X", DS3231_ADDR);
        return ESP_ERR_NOT_FOUND;
    }
    
    ESP_LOGI(TAG, "✓ DS3231 RTC initialized successfully");
    
    // Initialize DS3231 control register (enable oscillator, disable alarms)
    uint8_t control_data[] = {DS3231_REG_CONTROL, 0x00};
    ret = i2c_master_transmit(ds3231_handle, control_data, sizeof(control_data), 1000);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to set control register: %s", esp_err_to_name(ret));
    }
    
    return ESP_OK;
}

bool rtc_ds3231_is_available(void) {
    if (ds3231_handle == NULL) {
        return false;
    }
    
    uint8_t reg = DS3231_REG_SECONDS;
    uint8_t data;
    esp_err_t ret = i2c_master_transmit_receive(ds3231_handle, &reg, 1, &data, 1, 1000);
    return (ret == ESP_OK);
}

esp_err_t rtc_ds3231_get_time(struct tm *timeinfo) {
    if (ds3231_handle == NULL || timeinfo == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    uint8_t reg = DS3231_REG_SECONDS;
    uint8_t data[7];
    
    esp_err_t ret = i2c_master_transmit_receive(ds3231_handle, &reg, 1, data, 7, 1000);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read time: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Convert BCD to decimal and fill tm structure
    timeinfo->tm_sec = bcd_to_dec(data[0] & 0x7F);
    timeinfo->tm_min = bcd_to_dec(data[1] & 0x7F);
    timeinfo->tm_hour = bcd_to_dec(data[2] & 0x3F);  // 24-hour format
    timeinfo->tm_wday = bcd_to_dec(data[3] & 0x07) - 1;  // 0-6 (Sunday = 0)
    timeinfo->tm_mday = bcd_to_dec(data[4] & 0x3F);
    timeinfo->tm_mon = bcd_to_dec(data[5] & 0x1F) - 1;   // 0-11
    timeinfo->tm_year = bcd_to_dec(data[6]) + 100;       // Years since 1900
    
    // Calculate day of year (optional)
    timeinfo->tm_yday = 0;
    timeinfo->tm_isdst = -1;  // Let system determine DST
    
    return ESP_OK;
}

esp_err_t rtc_ds3231_set_time(struct tm *timeinfo) {
    if (ds3231_handle == NULL || timeinfo == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    uint8_t data[8];
    data[0] = DS3231_REG_SECONDS;
    data[1] = dec_to_bcd(timeinfo->tm_sec);
    data[2] = dec_to_bcd(timeinfo->tm_min);
    data[3] = dec_to_bcd(timeinfo->tm_hour);  // 24-hour format
    data[4] = dec_to_bcd(timeinfo->tm_wday + 1);  // 1-7 (Sunday = 1)
    data[5] = dec_to_bcd(timeinfo->tm_mday);
    data[6] = dec_to_bcd(timeinfo->tm_mon + 1);   // 1-12
    data[7] = dec_to_bcd(timeinfo->tm_year - 100); // Years since 2000
    
    esp_err_t ret = i2c_master_transmit(ds3231_handle, data, sizeof(data), 1000);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set time: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "✓ Time set: %04d-%02d-%02d %02d:%02d:%02d",
             timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday,
             timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
    
    return ESP_OK;
}

esp_err_t rtc_ds3231_sync_system_time(void) {
    struct tm timeinfo;
    esp_err_t ret = rtc_ds3231_get_time(&timeinfo);
    if (ret != ESP_OK) {
        return ret;
    }
    
    // Set ESP32 system time
    time_t t = mktime(&timeinfo);
    struct timeval tv = { .tv_sec = t };
    settimeofday(&tv, NULL);
    
    ESP_LOGI(TAG, "✓ System time synced: %04d-%02d-%02d %02d:%02d:%02d",
             timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
             timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    
    return ESP_OK;
}

esp_err_t rtc_ds3231_set_time_string(const char* time_str) {
    if (time_str == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    struct tm timeinfo = {0};
    
    // Parse "YYYY-MM-DD HH:MM:SS" format
    int parsed = sscanf(time_str, "%d-%d-%d %d:%d:%d",
                       &timeinfo.tm_year, &timeinfo.tm_mon, &timeinfo.tm_mday,
                       &timeinfo.tm_hour, &timeinfo.tm_min, &timeinfo.tm_sec);
    
    if (parsed != 6) {
        ESP_LOGE(TAG, "Invalid time format. Use: YYYY-MM-DD HH:MM:SS");
        return ESP_ERR_INVALID_ARG;
    }
    
    // Adjust for tm structure
    timeinfo.tm_year -= 1900;  // Years since 1900
    timeinfo.tm_mon -= 1;      // Months since January (0-11)
    
    // Calculate day of week
    time_t t = mktime(&timeinfo);
    struct tm *tm_ptr = localtime(&t);
    timeinfo.tm_wday = tm_ptr->tm_wday;
    
    return rtc_ds3231_set_time(&timeinfo);
}