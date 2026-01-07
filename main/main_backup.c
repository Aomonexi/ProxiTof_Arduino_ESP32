/**
 * main.c - BACKUP
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
 * Update LED based on system mode and distance
 */
static void update_led(uint16_t min_distance_mm, bool usb_msc_active)
{
    if (usb_msc_active) {
        // BLUE - USB Mass Storage mode (flash drive active)
        led_strip_set_pixel(led_strip, 0, 0, 0, 100);
    } else if (min_distance_mm < CLOSE_THRESHOLD) {
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
    // Declare all variables at the top
    esp_err_t err;
    system_config_t config;
    sensor_health_t health;
    display_config_t display_config;
    tof_data_t tof_data;
    imu_data_t imu_data;
    uint32_t frame_count = 0;
    uint32_t tof_count = 0;
    uint32_t imu_count = 0;
    
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "╔════════════════════════════════════════════╗");
    ESP_LOGI(TAG, "║   ProxiTof Near-Miss Detection System     ║");
    ESP_LOGI(TAG, "║   ESP32-S3 + VL53L8CX + BNO08x            ║");
    ESP_LOGI(TAG, "╚════════════════════════════════════════════╝");
    ESP_LOGI(TAG, "");
    
    // Initialize LED
    err = init_led();
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
    
    // Initialize SD card storage
    ESP_LOGI(TAG, "Initializing SD card storage...");
    err = storage_manager_init();
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "SD card initialization failed: %s", esp_err_to_name(err));
        ESP_LOGW(TAG, "Configuration will use defaults (no SD card access)");
    } else {
        ESP_LOGI(TAG, "✓ SD card initialized successfully");
    }
    ESP_LOGI(TAG, "");
    
    // Load configuration
    ESP_LOGI(TAG, "Loading system configuration...");
    esp_err_t config_err = config_load_from_sd(&config);
    
    if (config_err != ESP_OK) {
        ESP_LOGW(TAG, "Using default configuration");
        config_load_defaults(&config);
        
        if (storage_manager_is_initialized()) {
            if (config_save_to_sd(&config) == ESP_OK) {
                ESP_LOGI(TAG, "✓ Default config saved to /sdcard/config.txt");
            }
        }
    } else {
        ESP_LOGI(TAG, "✓ Configuration loaded from SD card");
    }
    
    config_print(&config);
    ESP_LOGI(TAG, "");
    
    // Initialize Smart Time Management
    ESP_LOGI(TAG, "Initializing Smart Time Management...");
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
    } else {
        ESP_LOGW(TAG, "❌ Smart Time Management failed: %s", esp_err_to_name(err));
    }
    ESP_LOGI(TAG, "");
    
    // Initialize USB system
    ESP_LOGI(TAG, "Initializing USB system with dedicated ports...");
    
    // Initialize USB Mass Storage for LEFT port
    err = usb_msc_init();
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "✓ USB Mass Storage initialized (LEFT port)");
    } else {
        ESP_LOGW(TAG, "❌ USB Mass Storage initialization failed: %s", esp_err_to_name(err));
    }
    
    // Initialize USB Serial JTAG for RIGHT port
    err = usb_interface_init();
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "✓ USB Serial JTAG initialized (RIGHT port)");
    } else {
        ESP_LOGW(TAG, "USB Serial JTAG initialization failed: %s", esp_err_to_name(err));
    }
    
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "🔌 USB Port Functions:");
    ESP_LOGI(TAG, "   RIGHT port: Programming, monitoring, time sync");
    ESP_LOGI(TAG, "   LEFT port:  Flash drive (automatic detection)");
    ESP_LOGI(TAG, "💡 Use ONE cable at a time for each function");
    ESP_LOGI(TAG, "");
    
    // Start logging if SD card available
    if (storage_manager_is_initialized()) {
        err = storage_manager_start_logging();
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "✓ Data logging started");
        } else {
            ESP_LOGW(TAG, "Failed to start logging: %s", esp_err_to_name(err));
        }
    } else {
        ESP_LOGW(TAG, "SD card not available - no data logging");
    }
    
    ESP_LOGI(TAG, "");
    
    // Initialize display manager
    display_config.refresh_rate_hz = config.display_refresh_hz;
    display_config.show_tof_grid = true;
    display_config.show_imu_data = true;
    display_config.show_system_stats = true;
    display_config.compact_mode = config.display_compact_mode;
    
    err = display_manager_init(&display_config);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "✓ Display manager initialized (%d Hz)", config.display_refresh_hz);
    } else {
        ESP_LOGW(TAG, "Display manager initialization failed: %s", esp_err_to_name(err));
    }
    
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    // Clear screen for clean display
    printf("\033[2J\033[H");
    
    // Main loop
    while (1) {
        uint32_t current_time = sensor_manager_get_timestamp();
        
        // Check if USB MSC is active (flash drive mode)
        bool usb_msc_active = usb_interface_is_msc_active();
        
        // Only read sensors and log data if NOT in USB MSC mode
        if (!usb_msc_active) {
            // Read ToF sensor
            err = sensor_manager_read_tof(&tof_data);
            if (err == ESP_OK) {
                tof_count++;
                
                // Update LED immediately
                uint16_t min_dist = get_min_distance(&tof_data);
                if (led_strip != NULL) {
                    update_led(min_dist, usb_msc_active);
                }
            }
            
            // Read IMU sensor
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
                
                // Write to buffer
                storage_manager_write(&log_entry);
            }
            
            // Display update
            if (tof_count > 0 || imu_count > 0) {
                uint16_t min_dist = get_min_distance(&tof_data);
                display_manager_update(&tof_data, &imu_data, min_dist);
            }
        } else {
            // USB MSC mode - show blue LED
            if (led_strip != NULL) {
                update_led(0, true);
            }
            vTaskDelay(pdMS_TO_TICKS(100));
        }
        
        // Small delay
        if (!usb_msc_active) {
            vTaskDelay(pdMS_TO_TICKS(5));
        }
        
        // Maintenance tasks
        smart_time_maintenance();
        usb_interface_process();
        usb_msc_handle_auto_detection();
        
        frame_count++;
    }
}