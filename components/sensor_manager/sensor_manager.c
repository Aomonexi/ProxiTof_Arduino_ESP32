/**
 * sensor_manager.c
 * 
 * ESP-IDF Sensor Manager Implementation
 */

#include "sensor_manager.h"
#include "driver/i2c_master.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"
#include <string.h>

// VL53L8CX library (RJRP44)
#include "vl53l8cx_api.h"

static const char *TAG = "SENSOR_MGR";

// =============================================================================
// Private State
// =============================================================================

// SPI bus handle
static spi_device_handle_t spi_imu = NULL;

// VL53L8CX ToF sensor
static VL53L8CX_Configuration tof_dev;
static bool tof_initialized = false;

// IMU state (placeholder - will implement BNO08x driver)
static bool imu_initialized = false;

// Button state
static uint32_t last_button_press_time = 0;
static uint32_t last_button_read_time = 0;
static bool last_button_state = false;

// Health tracking
static sensor_health_t health = {
    .tof_healthy = false,
    .imu_healthy = false,
    .tof_error_count = 0,
    .imu_error_count = 0
};

// =============================================================================
// Private Helper Functions
// =============================================================================

// I2C master bus handle (new ESP-IDF 5.x API)
static i2c_master_bus_handle_t i2c_bus_handle = NULL;
static i2c_master_bus_config_t i2c_bus_config;  // Store for platform structure

/**
 * Initialize I2C bus for ToF sensor (ESP-IDF 5.x API)
 */
static esp_err_t init_i2c_bus(void)
{
    i2c_bus_config.i2c_port = I2C_NUM_0;
    i2c_bus_config.sda_io_num = TOF_SDA_PIN;
    i2c_bus_config.scl_io_num = TOF_SCL_PIN;
    i2c_bus_config.clk_source = I2C_CLK_SRC_DEFAULT;
    i2c_bus_config.glitch_ignore_cnt = 7;
    i2c_bus_config.flags.enable_internal_pullup = true;
    
    esp_err_t err = i2c_new_master_bus(&i2c_bus_config, &i2c_bus_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2C master bus init failed: %s", esp_err_to_name(err));
        return err;
    }
    
    ESP_LOGI(TAG, "I2C bus initialized (SDA=%d, SCL=%d)", 
             TOF_SDA_PIN, TOF_SCL_PIN);
    ESP_LOGI(TAG, "  Internal pullups: ENABLED");
    return ESP_OK;
}

/**
 * Initialize SPI bus for IMU sensor
 */
static esp_err_t init_spi_bus(void)
{
    spi_bus_config_t buscfg = {
        .mosi_io_num = IMU_MOSI_PIN,
        .miso_io_num = IMU_MISO_PIN,
        .sclk_io_num = IMU_SCK_PIN,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4096,
    };
    
    esp_err_t err = spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "SPI bus init failed: %s", esp_err_to_name(err));
        return err;
    }
    
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = SPI_MASTER_FREQ_HZ,
        .mode = 0,
        .spics_io_num = IMU_CS_PIN,
        .queue_size = 7,
    };
    
    err = spi_bus_add_device(SPI2_HOST, &devcfg, &spi_imu);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "SPI device add failed: %s", esp_err_to_name(err));
        return err;
    }
    
    ESP_LOGI(TAG, "SPI bus initialized (MOSI=%d, MISO=%d, SCK=%d, CS=%d)", 
             IMU_MOSI_PIN, IMU_MISO_PIN, IMU_SCK_PIN, IMU_CS_PIN);
    return ESP_OK;
}

/**
 * Initialize GPIO for button input
 */
static esp_err_t init_button_gpio(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << FLAG_BUTTON_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    
    esp_err_t err = gpio_config(&io_conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Button GPIO config failed: %s", esp_err_to_name(err));
        return err;
    }
    
    ESP_LOGI(TAG, "Button GPIO initialized (pin=%d)", FLAG_BUTTON_PIN);
    return ESP_OK;
}

/**
 * Scan I2C bus for devices (debugging helper)
 */
static void scan_i2c_bus(void)
{
    ESP_LOGI(TAG, "Scanning I2C bus...");
    int found = 0;
    
    for (uint8_t addr = 1; addr < 127; addr++) {
        i2c_device_config_t dev_cfg = {
            .dev_addr_length = I2C_ADDR_BIT_LEN_7,
            .device_address = addr,
            .scl_speed_hz = I2C_MASTER_FREQ_HZ,
        };
        
        i2c_master_dev_handle_t dev_handle;
        esp_err_t ret = i2c_master_bus_add_device(i2c_bus_handle, &dev_cfg, &dev_handle);
        if (ret == ESP_OK) {
            // Try to probe the device
            uint8_t dummy;
            ret = i2c_master_receive(dev_handle, &dummy, 1, 50);
            if (ret == ESP_OK || ret == ESP_ERR_TIMEOUT) {
                ESP_LOGI(TAG, "  Device found at 0x%02X", addr);
                if (addr == 0x29) {
                    ESP_LOGI(TAG, "    ^ VL53L8CX ToF Sensor!");
                }
                found++;
            }
            i2c_master_bus_rm_device(dev_handle);
        }
    }
    
    if (found == 0) {
        ESP_LOGW(TAG, "No I2C devices found!");
        ESP_LOGW(TAG, "Check wiring:");
        ESP_LOGW(TAG, "  - SDA: GPIO %d", TOF_SDA_PIN);
        ESP_LOGW(TAG, "  - SCL: GPIO %d", TOF_SCL_PIN);
        ESP_LOGW(TAG, "  - LPN: GPIO %d", TOF_LPN_PIN);
        ESP_LOGW(TAG, "  - VCC: 3.3V");
        ESP_LOGW(TAG, "  - GND: Connected");
    } else {
        ESP_LOGI(TAG, "Found %d device(s)", found);
    }
}

/**
 * Initialize VL53L8CX ToF sensor (ESP-IDF 5.x API)
 * Based on working implementation from proxitof_ag_main.cpp
 */
static esp_err_t init_tof_sensor(void)
{
    ESP_LOGI(TAG, "Initializing VL53L8CX ToF sensor...");
    
    // Clear the device structure
    memset(&tof_dev, 0, sizeof(VL53L8CX_Configuration));
    
    // Store bus config in platform structure (CRITICAL for library to work)
    tof_dev.platform.bus_config = i2c_bus_config;
    
    // Configure I2C device with 7-bit address
    // VL53L8CX_DEFAULT_I2C_ADDRESS is 0x52 (8-bit), shift right to get 0x29 (7-bit)
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = VL53L8CX_DEFAULT_I2C_ADDRESS >> 1,  // Convert 8-bit to 7-bit address
        .scl_speed_hz = VL53L8CX_MAX_CLK_SPEED,
    };
    
    esp_err_t err = i2c_master_bus_add_device(i2c_bus_handle, &dev_cfg, &tof_dev.platform.handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add I2C device: %s", esp_err_to_name(err));
        health.tof_error_count++;
        return err;
    }
    
    // Set platform parameters
    tof_dev.platform.reset_gpio = TOF_LPN_PIN;
    
    // Reset sensor using library function
    ESP_LOGI(TAG, "Resetting sensor on GPIO%d...", TOF_LPN_PIN);
    VL53L8CX_Reset_Sensor(&(tof_dev.platform));
    vTaskDelay(pdMS_TO_TICKS(100));  // Wait for sensor to power up
    
    // Check if sensor is alive
    ESP_LOGI(TAG, "Checking if sensor is alive...");
    uint8_t is_alive = 0;
    uint8_t status = vl53l8cx_is_alive(&tof_dev, &is_alive);
    if (status != VL53L8CX_STATUS_OK || !is_alive) {
        ESP_LOGE(TAG, "VL53L8CX NOT DETECTED!");
        ESP_LOGE(TAG, "  Status: %d, IsAlive: %d", status, is_alive);
        ESP_LOGE(TAG, "");
        ESP_LOGE(TAG, "Troubleshooting:");
        ESP_LOGE(TAG, "  1. Check I2C wiring (SDA=GPIO%d, SCL=GPIO%d)", TOF_SDA_PIN, TOF_SCL_PIN);
        ESP_LOGE(TAG, "  2. Check power (3.3V, GND)");
        ESP_LOGE(TAG, "  3. Check LPN pin (GPIO%d)", TOF_LPN_PIN);
        ESP_LOGE(TAG, "  4. Try external 2.2k pullups on SDA/SCL");
        ESP_LOGE(TAG, "  5. Check if sensor appears in I2C scan above");
        health.tof_error_count++;
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "✓ VL53L8CX is alive!");
    
    // Initialize sensor firmware
    ESP_LOGI(TAG, "Initializing sensor firmware...");
    status = vl53l8cx_init(&tof_dev);
    if (status != VL53L8CX_STATUS_OK) {
        ESP_LOGE(TAG, "✗ Init failed (status: %d)", status);
        health.tof_error_count++;
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "✓ VL53L8CX ready! (%s)", VL53L8CX_API_REVISION);
    
    // Configure for 4x4 mode at 60Hz
    ESP_LOGI(TAG, "Configuring 4x4 @ 60Hz...");
    
    status = vl53l8cx_set_resolution(&tof_dev, VL53L8CX_RESOLUTION_4X4);
    if (status != VL53L8CX_STATUS_OK) {
        ESP_LOGE(TAG, "Set resolution failed (status=%d)", status);
        health.tof_error_count++;
        return ESP_FAIL;
    }
    
    status = vl53l8cx_set_ranging_frequency_hz(&tof_dev, 60);
    if (status != VL53L8CX_STATUS_OK) {
        ESP_LOGE(TAG, "Set frequency failed (status=%d)", status);
        health.tof_error_count++;
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "✓ Configuration complete");
    
    // Start ranging
    status = vl53l8cx_start_ranging(&tof_dev);
    if (status != VL53L8CX_STATUS_OK) {
        ESP_LOGE(TAG, "✗ Start ranging failed (status=%d)", status);
        health.tof_error_count++;
        return ESP_FAIL;
    }
    
    tof_initialized = true;
    health.tof_healthy = true;
    ESP_LOGI(TAG, "✓ VL53L8CX initialized successfully (4x4 mode, 60Hz)");
    return ESP_OK;
}

/**
 * Initialize BNO08x IMU sensor
 * TODO: Implement BNO08x SPI driver
 */
static esp_err_t init_imu_sensor(void)
{
    ESP_LOGI(TAG, "Initializing BNO08x IMU sensor...");
    
    // Configure RST pin
    gpio_config_t rst_conf = {
        .pin_bit_mask = (1ULL << IMU_RST_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&rst_conf);
    
    // Configure INT pin
    gpio_config_t int_conf = {
        .pin_bit_mask = (1ULL << IMU_INT_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&int_conf);
    
    // Reset IMU
    gpio_set_level(IMU_RST_PIN, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(IMU_RST_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(100));  // Wait for IMU to boot
    
    // TODO: Implement BNO08x initialization via SPI
    // For now, mark as not initialized
    imu_initialized = false;
    health.imu_healthy = false;
    
    ESP_LOGW(TAG, "BNO08x IMU initialization not yet implemented");
    return ESP_ERR_NOT_SUPPORTED;
}

// =============================================================================
// Public API Implementation
// =============================================================================

esp_err_t sensor_manager_init(void)
{
    ESP_LOGI(TAG, "Initializing Sensor Manager...");
    
    esp_err_t err;
    bool at_least_one_sensor_ok = false;
    
    // Initialize I2C bus
    err = init_i2c_bus();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize I2C bus");
        return err;
    }
    
    // Initialize SPI bus
    err = init_spi_bus();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SPI bus");
        // Continue - ToF might still work
    }
    
    // Initialize button GPIO
    err = init_button_gpio();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize button GPIO");
        // Continue - sensors might still work
    }
    
    // Scan I2C bus to see what's connected
    scan_i2c_bus();
    
    // Initialize ToF sensor (retry up to 3 times)
    for (int retry = 0; retry < 3; retry++) {
        err = init_tof_sensor();
        if (err == ESP_OK) {
            at_least_one_sensor_ok = true;
            break;
        }
        ESP_LOGW(TAG, "ToF init failed, retry %d/3", retry + 1);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    
    // Initialize IMU sensor (retry up to 3 times)
    for (int retry = 0; retry < 3; retry++) {
        err = init_imu_sensor();
        if (err == ESP_OK) {
            at_least_one_sensor_ok = true;
            break;
        }
        if (err == ESP_ERR_NOT_SUPPORTED) {
            // Not implemented yet, don't retry
            break;
        }
        ESP_LOGW(TAG, "IMU init failed, retry %d/3", retry + 1);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    
    if (!at_least_one_sensor_ok) {
        ESP_LOGE(TAG, "No sensors initialized successfully");
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "Sensor Manager initialized (ToF: %s, IMU: %s)",
             health.tof_healthy ? "OK" : "FAIL",
             health.imu_healthy ? "OK" : "FAIL");
    
    return ESP_OK;
}

esp_err_t sensor_manager_read_tof(tof_data_t *data)
{
    if (!tof_initialized || !health.tof_healthy) {
        return ESP_ERR_INVALID_STATE;
    }
    
    if (data == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // Check if data is ready
    uint8_t is_ready = 0;
    uint8_t status = vl53l8cx_check_data_ready(&tof_dev, &is_ready);
    if (status != VL53L8CX_STATUS_OK || !is_ready) {
        return ESP_ERR_NOT_FOUND;  // No new data available
    }
    
    // Get ranging data
    VL53L8CX_ResultsData results;
    status = vl53l8cx_get_ranging_data(&tof_dev, &results);
    if (status != VL53L8CX_STATUS_OK) {
        ESP_LOGE(TAG, "Failed to get ToF data (status=%d)", status);
        health.tof_error_count++;
        return ESP_FAIL;
    }
    
    // Fill output structure
    data->timestamp_ms = sensor_manager_get_timestamp();
    data->valid = true;
    
    // Copy distance data (4x4 grid in row-major order)
    // Use VL53L8CX_NB_TARGET_PER_ZONE to access correct data index
    for (int i = 0; i < 16; i++) {
        data->distances[i] = results.distance_mm[VL53L8CX_NB_TARGET_PER_ZONE * i];
    }
    
    return ESP_OK;
}

esp_err_t sensor_manager_read_imu(imu_data_t *data)
{
    if (!imu_initialized || !health.imu_healthy) {
        return ESP_ERR_INVALID_STATE;
    }
    
    if (data == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // TODO: Implement BNO08x data reading
    // For now, return not supported
    return ESP_ERR_NOT_SUPPORTED;
}

bool sensor_manager_is_button_pressed(void)
{
    uint32_t current_time = sensor_manager_get_timestamp();
    
    // Read button state (active low with pull-up)
    bool current_state = (gpio_get_level(FLAG_BUTTON_PIN) == 0);
    
    // Debouncing logic
    if (current_state != last_button_state) {
        last_button_read_time = current_time;
        last_button_state = current_state;
    }
    
    // Check if button has been stable for debounce period
    if (current_state && 
        (current_time - last_button_read_time) >= BUTTON_DEBOUNCE_MS &&
        (current_time - last_button_press_time) >= BUTTON_DEBOUNCE_MS) {
        
        last_button_press_time = current_time;
        ESP_LOGI(TAG, "Button pressed at %lu ms", current_time);
        return true;
    }
    
    return false;
}

uint32_t sensor_manager_get_timestamp(void)
{
    return (uint32_t)(esp_timer_get_time() / 1000);  // Convert µs to ms
}

void sensor_manager_get_health(sensor_health_t *health_out)
{
    if (health_out != NULL) {
        memcpy(health_out, &health, sizeof(sensor_health_t));
    }
}

uint32_t sensor_manager_get_last_button_press_time(void)
{
    return last_button_press_time;
}
