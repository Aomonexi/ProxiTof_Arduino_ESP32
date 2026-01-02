/**
 * main.c
 * 
 * ProxiTof Near-Miss Detection System - Main Application
 * ESP-IDF Framework
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "sensor_manager.h"
#include "storage_manager.h"
#include "smart_time.h"
#include "config_manager.h"
#include "usb_interface.h"
#include "usb_msc.h"
#include "display_manager.h"
#include "led_strip.h"
#include "driver/gpio.h"

static const char *TAG = "MAIN";

// LED Configuration
#define RGB_LED_PIN GPIO_NUM_48

// Distance thresholds (in mm)
#define CLOSE_THRESHOLD 100    // < 10cm = RED
#define MEDIUM_THRESHOLD 500   // 10-50cm = YELLOW, > 50cm = GREEN

static led_strip_handle_t led_strip = NULL;

/**
 * Initialize RGB LED
 */
static esp_err_t init_led(void)
{
    led_strip_config_t strip_config = {
        .strip_gpio_num = RGB_LED_PIN,
        .max_leds = 1,
        .led_pixel_format = LED_PIXEL_FORMAT_GRB,
        .led_model = LED_MODEL_WS2812,
        .flags.invert_out = false,
    };

    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 10 * 1000 * 1000,
        .flags.with_dma = false,
    };

    esp_err_t err = led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create LED strip: %s", esp_err_to_name(err));
        return err;
    }

    led_strip_clear(led_strip);
    ESP_LOGI(TAG, "RGB LED initialized on GPIO%d", RGB_LED_PIN);
    return ESP_OK;
}

/**
 * Update LED based on minimum distance
 */
static void update_led(uint16_t min_distance_mm)
{
    if (min_distance_mm < CLOSE_THRESHOLD) {
        // RED - Close (< 10cm)
        led_strip_set_pixel(led_strip, 0, 50, 0, 0);
    } else if (min_distance_mm < MEDIUM_THRESHOLD) {
        // YELLOW - Medium (10-50cm)
        led_strip_set_pixel(led_strip, 0, 50, 50, 0);
    } else {
        // GREEN - Far (> 50cm)
        led_strip_set_pixel(led_strip, 0, 0, 50, 0);
    }
    led_strip_refresh(led_strip);
}

/**
 * Find minimum distance from ToF grid
 */
static uint16_t get_min_distance(tof_data_t *tof_data)
{
    uint16_t min_dist = 9999;
    for (int i = 0; i < 16; i++) {
        if (tof_data->distances[i] > 0 && tof_data->distances[i] < min_dist) {
            min_dist = tof_data->distances[i];
        }
    }
    return min_dist;
}

void app_main(void)
{
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "╔════════════════════════════════════════════╗");
    ESP_LOGI(TAG, "║   ProxiTof Near-Miss Detection System     ║");
    ESP_LOGI(TAG, "║   ESP32-S3 + VL53L8CX + BNO08x            ║");
    ESP_LOGI(TAG, "╚════════════════════════════════════════════╝");
    ESP_LOGI(TAG, "");
    
    // Initialize LED
    esp_err_t err = init_led();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "LED initialization failed, continuing without LED");
    }
    
    // Initialize sensor manager
    err = sensor_manager_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Sensor manager initialization failed: %s", esp_err_to_name(err));
        ESP_LOGE(TAG, "System cannot operate without sensors");
        return;
    }
    
    ESP_LOGI(TAG, "✓ Sensor manager initialized successfully");
    ESP_LOGI(TAG, "");
    
    // =========================================================================
    // SD CARD INITIALIZATION (MUST BE FIRST FOR CONFIG LOADING)
    // =========================================================================
    ESP_LOGI(TAG, "Initializing SD card storage...");
    err = storage_manager_init();
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "SD card initialization failed: %s", esp_err_to_name(err));
        ESP_LOGW(TAG, "Configuration will use defaults (no SD card access)");
        ESP_LOGW(TAG, "");
        ESP_LOGW(TAG, "Troubleshooting:");
        ESP_LOGW(TAG, "  1. Check SD card is inserted");
        ESP_LOGW(TAG, "  2. Format SD card as FAT32");
        ESP_LOGW(TAG, "  3. Check wiring: CLK=14, CMD=15, D0=2, D1=16, D2=17, D3=18");
        ESP_LOGW(TAG, "  4. Verify 3.3V power to SD card");
    } else {
        ESP_LOGI(TAG, "✓ SD card initialized successfully");
    }
    ESP_LOGI(TAG, "");
    
    // =========================================================================
    // CONFIGURATION MANAGEMENT (SD CARD)
    // =========================================================================
    ESP_LOGI(TAG, "Loading system configuration...");
    
    system_config_t config;
    esp_err_t config_err = config_load_from_sd(&config);
    
    if (config_err != ESP_OK) {
        ESP_LOGW(TAG, "Using default configuration");
        config_load_defaults(&config);
        
        // Save default config to SD card for user to edit (if SD card available)
        if (storage_manager_is_initialized()) {
            if (config_save_to_sd(&config) == ESP_OK) {
                ESP_LOGI(TAG, "✓ Default config saved to /sdcard/config.txt");
                ESP_LOGI(TAG, "💡 Edit config.txt on SD card to change WiFi settings");
            }
        } else {
            ESP_LOGW(TAG, "💡 Insert SD card and restart to use file-based configuration");
            ESP_LOGW(TAG, "💡 Or use USB serial commands: wifi_config SSID PASSWORD");
        }
    } else {
        ESP_LOGI(TAG, "✓ Configuration loaded from SD card");
    }
    
    config_print(&config);
    ESP_LOGI(TAG, "");
    
    // =========================================================================
    // SMART TIME MANAGEMENT SYSTEM
    // =========================================================================
    ESP_LOGI(TAG, "Initializing Smart Time Management...");
    ESP_LOGI(TAG, "Multi-tier time keeping: DS3231 + Internal RTC + WiFi/NTP");
    ESP_LOGI(TAG, "");
    
    err = smart_time_init(
        config.ds3231_enabled ? config.ds3231_sda_pin : -1,
        config.ds3231_enabled ? config.ds3231_scl_pin : -1,
        config.wifi_enabled ? config.wifi_ssid : NULL,
        config.wifi_enabled ? config.wifi_password : NULL,
        config.timezone,
        config.sync_interval_hours
    );
    
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "✓ Smart Time Management initialized");
        
        // Get time status
        time_status_t time_status;
        smart_time_get_status(&time_status);
        
        ESP_LOGI(TAG, "Time Status:");
        ESP_LOGI(TAG, "  DS3231: %s", time_status.ds3231_available ? "✓ Available" : "✗ Not available");
        ESP_LOGI(TAG, "  Current source: %s", smart_time_source_name(time_status.current_source));
        ESP_LOGI(TAG, "  Sync count: %lu", time_status.sync_count);
        
        if (smart_time_is_valid()) {
            ESP_LOGI(TAG, "✓ Time is valid and reliable");
        } else {
            ESP_LOGW(TAG, "⚠️  Time may not be accurate");
            ESP_LOGW(TAG, "💡 To set time manually: smart_time_set_manual(\"2024-12-24 15:30:00\")");
        }
    } else {
        ESP_LOGW(TAG, "❌ Smart Time Management failed: %s", esp_err_to_name(err));
        ESP_LOGW(TAG, "System will use default time");
    }
    
    ESP_LOGI(TAG, "");
    
    // =========================================================================
    // USB SYSTEM INITIALIZATION
    // =========================================================================
    ESP_LOGI(TAG, "Initializing USB system...");
    
    // Initialize USB Mass Storage first (TinyUSB stack)
    err = usb_msc_init();
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "✓ USB Mass Storage initialized");
        ESP_LOGI(TAG, "💾 SD card will appear as USB drive when connected");
    } else {
        ESP_LOGW(TAG, "USB Mass Storage initialization failed: %s", esp_err_to_name(err));
    }
    
    // Initialize USB Serial JTAG interface (compatible with TinyUSB)
    err = usb_interface_init();
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "✓ USB Serial JTAG interface initialized");
        ESP_LOGI(TAG, "💻 Connect USB cable for configuration access");
        ESP_LOGI(TAG, "📝 Use serial terminal or Python tool for commands");
    } else {
        ESP_LOGW(TAG, "USB Serial JTAG interface initialization failed: %s", esp_err_to_name(err));
    }
    
    ESP_LOGI(TAG, "⚠️  Note: USB Serial JTAG and USB MSC share the same USB port");
    ESP_LOGI(TAG, "💡 Programming/debugging works alongside USB MSC functionality");
    
    ESP_LOGI(TAG, "");
    
    // Get sensor health status
    sensor_health_t health;
    sensor_manager_get_health(&health);
    ESP_LOGI(TAG, "Sensor Health:");
    ESP_LOGI(TAG, "  ToF: %s (errors: %lu)", 
             health.tof_healthy ? "✓ HEALTHY" : "✗ FAILED", health.tof_error_count);
    ESP_LOGI(TAG, "  IMU: %s (errors: %lu)", 
             health.imu_healthy ? "✓ HEALTHY" : "✗ FAILED", health.imu_error_count);
    ESP_LOGI(TAG, "");
    
    // =========================================================================
    // USB MSC AND DATA LOGGING STARTUP
    // =========================================================================
    if (storage_manager_is_initialized()) {
        // Don't start USB Mass Storage automatically - let it be started on demand
        ESP_LOGI(TAG, "USB Mass Storage ready (not started automatically)");
        ESP_LOGI(TAG, "💡 Use USB serial command 'usb_msc start' to enable USB drive mode");
        
        // Start logging immediately since USB MSC is not active
        err = storage_manager_start_logging();
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "✓ Data logging started");
        } else {
            ESP_LOGW(TAG, "Failed to start logging: %s", esp_err_to_name(err));
            ESP_LOGW(TAG, "💡 This is likely due to SD card write protection");
        }
    } else {
        ESP_LOGW(TAG, "SD card not available - no USB MSC or data logging");
    }
    
    ESP_LOGI(TAG, "");
    
    // Final sensor health check before starting main loop
    sensor_manager_get_health(&health);
    if (!health.tof_healthy) {
        ESP_LOGE(TAG, "╔════════════════════════════════════════════════╗");
        ESP_LOGE(TAG, "║  ⚠️  ToF SENSOR FAILED TO INITIALIZE           ║");
        ESP_LOGE(TAG, "╠════════════════════════════════════════════════╣");
        ESP_LOGE(TAG, "║  System will continue with IMU only            ║");
        ESP_LOGE(TAG, "║  LED will not work (no distance data)          ║");
        ESP_LOGE(TAG, "║                                                ║");
        ESP_LOGE(TAG, "║  Troubleshooting:                              ║");
        ESP_LOGE(TAG, "║  1. Check I2C wiring (SDA=8, SCL=9, LPN=4)     ║");
        ESP_LOGE(TAG, "║  2. Check 3.3V power to ToF sensor             ║");
        ESP_LOGE(TAG, "║  3. Check I2C scan results above               ║");
        ESP_LOGE(TAG, "║  4. Try external 2.2k pullups on SDA/SCL       ║");
        ESP_LOGE(TAG, "╚════════════════════════════════════════════════╝");
        ESP_LOGE(TAG, "");
    }
    
    ESP_LOGI(TAG, "Starting real-time sensor display in 2 seconds...");
    
    // Initialize display manager
    display_config_t display_config = {
        .refresh_rate_hz = 30,        // 30 Hz default refresh rate
        .show_tof_grid = true,        // Show ToF 4x4 grid
        .show_imu_data = true,        // Show IMU orientation/acceleration
        .show_system_stats = true,    // Show logging statistics
        .compact_mode = false         // Use full display layout
    };
    
    err = display_manager_init(&display_config);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "✓ Display manager initialized (30 Hz)");
        ESP_LOGI(TAG, "💡 Use 'display refresh HZ' to change refresh rate");
    } else {
        ESP_LOGW(TAG, "Display manager initialization failed: %s", esp_err_to_name(err));
        ESP_LOGW(TAG, "System will use basic console output");
    }
    
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    // Clear screen for clean display
    printf("\033[2J\033[H");
    
    tof_data_t tof_data;
    imu_data_t imu_data;
    uint32_t frame_count = 0;
    uint32_t tof_count = 0;
    uint32_t imu_count = 0;
    
    while (1) {
        uint32_t current_time = sensor_manager_get_timestamp();
        
        // Read ToF sensor at full speed (60Hz)
        err = sensor_manager_read_tof(&tof_data);
        if (err == ESP_OK) {
            tof_count++;
            
            // Get minimum distance and update LED IMMEDIATELY (real-time response)
            uint16_t min_dist = get_min_distance(&tof_data);
            if (led_strip != NULL) {
                update_led(min_dist);
            }
            // Store for display (removed last_min_dist variable)
        } else if (frame_count < 5) {
            // Log ToF errors during first few frames for debugging
            ESP_LOGW(TAG, "ToF read failed: %s", esp_err_to_name(err));
        }
        
        // Read IMU sensor at full speed (100Hz)
        err = sensor_manager_read_imu(&imu_data);
        if (err == ESP_OK) {
            imu_count++;
        }
        
        // Log to SD card if both sensors have data
        if (storage_manager_is_logging() && tof_count > 0 && imu_count > 0) {
            log_entry_t log_entry = {
                .timestamp_ms = current_time,
                .flag_manual = sensor_manager_is_button_held() ? 1 : 0
            };
            
            // Copy ToF data
            for (int i = 0; i < 16; i++) {
                log_entry.tof[i] = tof_data.distances[i];
            }
            
            // Copy IMU data
            log_entry.roll = imu_data.roll;
            log_entry.pitch = imu_data.pitch;
            log_entry.yaw = imu_data.yaw;
            log_entry.accel_x = imu_data.accel_x;
            log_entry.accel_y = imu_data.accel_y;
            log_entry.accel_z = imu_data.accel_z;
            log_entry.gyro_x = imu_data.gyro_x;
            log_entry.gyro_y = imu_data.gyro_y;
            log_entry.gyro_z = imu_data.gyro_z;
            
            // Write to buffer (non-blocking)
            storage_manager_write(&log_entry);
        }
        
        // Display update using display manager (handles timing automatically)
        if (tof_count > 0 || imu_count > 0) {
            uint16_t min_dist = get_min_distance(&tof_data);
            display_manager_update(&tof_data, &imu_data, min_dist);
        }
        
        // Check button status (simplified - no display spam)
        static uint32_t last_button_check = 0;
        if ((current_time - last_button_check) >= 1000) { // Check every second
            last_button_check = current_time;
            
            if (sensor_manager_is_button_held()) {
                uint32_t duration = sensor_manager_get_button_duration();
                ESP_LOGI(TAG, "⚠️  BUTTON HELD - Flagging event (Duration: %.1f seconds)", duration / 1000.0f);
            } else if (sensor_manager_is_button_pressed()) {
                ESP_LOGI(TAG, "✓ EVENT FLAGGED - Button released (Timestamp: %lu ms)", 
                         sensor_manager_get_last_button_press_time());
            }
        }
        
        // Small delay to prevent CPU spinning (check sensors frequently)
        vTaskDelay(pdMS_TO_TICKS(5));  // 200Hz loop rate for responsive LED
        
        // Smart time maintenance (every ~60 seconds)
        smart_time_maintenance();
        
        // USB interface processing (handle commands from computer)
        usb_interface_process();
        
        // Handle automatic USB MSC switching based on computer connection
        usb_interface_handle_connection_changes();
    }
}
