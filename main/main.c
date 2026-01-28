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
#include "esp_sleep.h"
#include "sensor_manager.h"
#include "storage_manager.h"
#include "smart_time.h"
#include "config_manager.h"
#include "usb_interface.h"
#include "usb_flash_partition.h"
#include "usb_flash_partition.h"
#include "display_manager.h"
#include "oled_display.h"
#include "gps.h"
#include "led_strip.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "tusb.h"
#include "data_processor_c_wrapper.h"

static const char *TAG = "MAIN";

// LED Configuration
#define RGB_LED_PIN GPIO_NUM_48

// Mode Switch Button Configuration
#define MODE_BUTTON_PIN GPIO_NUM_20
#define MODE_BUTTON_DEBOUNCE_MS 200

// GPS Configuration (Neo-M9N)
// CRITICAL: GPIO 43/44 are UART0 (console) pins - cannot use for GPS!
// Using free GPIO pins: 38 (TX) and 47 (RX)
// GPIO 48 is used by WS2812 LED, so we use 38 instead
#define GPS_UART_NUM UART_NUM_1     // UART1
#define GPS_TX_PIN GPIO_NUM_38      // ESP32 TX → GPS RX (FREE GPIO)
#define GPS_RX_PIN GPIO_NUM_47      // ESP32 RX ← GPS TX (FREE GPIO)
#define GPS_BAUD_RATE 38400         // Neo-M9N default: 38400 baud, 8N1

// Distance thresholds (in mm)
#define CLOSE_THRESHOLD 100    // < 10cm = RED
#define MEDIUM_THRESHOLD 500   // 10-50cm = YELLOW, > 50cm = GREEN

// Near-miss detection thresholds
#define RAPID_APPROACH_THRESHOLD 1000.0f   // mm/s (1.0 m/s) - fast approach
#define HIGH_JERK_THRESHOLD 7.0f           // m/s³ - sudden reaction (increased from 5.0)
#define CRITICAL_PROXIMITY_DISTANCE 100    // mm (10 cm) - immediate danger
#define CRITICAL_PROXIMITY_APPROACH 500.0f // mm/s (0.5 m/s) - fast at close range
#define COMBINED_DETECTION_MAX_DISTANCE 150 // mm (15 cm) - near-miss zone (reduced from 500)

static led_strip_handle_t led_strip = NULL;
static data_processor_handle_t *data_processor = NULL;

// Mode button state
static uint32_t last_mode_button_press = 0;
static bool last_mode_button_state = true;  // Pull-up, so HIGH when not pressed

/**
 * Initialize mode switch button GPIO
 */
static esp_err_t init_mode_button(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << MODE_BUTTON_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    
    esp_err_t err = gpio_config(&io_conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Mode button GPIO config failed: %s", esp_err_to_name(err));
        return err;
    }
    
    ESP_LOGI(TAG, "✓ Mode switch button initialized (GPIO %d)", MODE_BUTTON_PIN);
    ESP_LOGI(TAG, "💡 Press button to cycle through display modes");
    return ESP_OK;
}

/**
 * Check mode button and cycle display mode if pressed
 * Returns true if mode was changed
 */
static bool check_mode_button(void)
{
    uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
    
    // Read button state (active LOW with pull-up)
    bool current_state = (gpio_get_level(MODE_BUTTON_PIN) == 0);
    
    // Detect falling edge (button press)
    if (current_state && !last_mode_button_state) {
        // Check debounce
        if ((current_time - last_mode_button_press) >= MODE_BUTTON_DEBOUNCE_MS) {
            last_mode_button_press = current_time;
            
            // Cycle to next display mode
            oled_display_mode_t current_mode = oled_display_get_mode();
            oled_display_mode_t next_mode = (oled_display_mode_t)((current_mode + 1) % OLED_MODE_MAX);
            oled_display_set_mode(next_mode);
            
            const char *mode_names[] = {"Dashboard", "Status", "ToF Focus", "IMU Focus", "GPS", "Near Miss", "WiFi Settings", "Shutdown"};
            ESP_LOGI(TAG, "🔄 Display mode changed: %s → %s", 
                     mode_names[current_mode], mode_names[next_mode]);
            
            last_mode_button_state = current_state;
            return true;
        }
    }
    
    last_mode_button_state = current_state;
    return false;
}

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
 * Update LED based on system mode, distance, and near-miss detection
 * Priority: USB MSC > Near-Miss Alert > Distance-based color
 */
static void update_led(uint16_t min_distance_mm, bool usb_msc_active, bool near_miss_detected)
{
    if (usb_msc_active) {
        // BLUE - USB Mass Storage mode (flash drive active)
        led_strip_set_pixel(led_strip, 0, 0, 0, 100);
    } else if (near_miss_detected) {
        // ORANGE/RED FLASH - Near-miss event detected
        // Alternate between orange and red for high visibility
        static uint32_t last_flash = 0;
        static bool flash_state = false;
        uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
        
        if ((now - last_flash) > 250) {  // 4Hz flash rate
            flash_state = !flash_state;
            last_flash = now;
        }
        
        if (flash_state) {
            // Bright orange
            led_strip_set_pixel(led_strip, 0, 100, 50, 0);
        } else {
            // Bright red
            led_strip_set_pixel(led_strip, 0, 100, 0, 0);
        }
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
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "╔════════════════════════════════════════════╗");
    ESP_LOGI(TAG, "║   ProxiTof Near-Miss Detection System     ║");
    ESP_LOGI(TAG, "║   ESP32-S3 + VL53L8CX + BNO08x            ║");
    ESP_LOGI(TAG, "╚════════════════════════════════════════════╝");
    ESP_LOGI(TAG, "");
    
    // Declare all variables at the top of the function
    esp_err_t err;
    system_config_t config;
    sensor_health_t health;
    display_config_t display_config;
    tof_data_t tof_data;
    imu_data_t imu_data;
    uint32_t frame_count = 0;
    uint32_t tof_count = 0;
    uint32_t imu_count = 0;
    
    // Initialize LED
    err = init_led();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "LED initialization failed, continuing without LED");
    }
    
    // Initialize mode switch button
    err = init_mode_button();
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Mode button initialization failed, continuing without mode switching");
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
    // GPS MODULE INITIALIZATION (Neo-M9N)
    // =========================================================================
    ESP_LOGI(TAG, "Initializing GPS module (Neo-M9N)...");
    ESP_LOGI(TAG, "⚠️  IMPORTANT: GPIO 43/44 are UART0 (console) - using GPIO 38/47 instead!");
    ESP_LOGI(TAG, "Wiring: TX=GPIO38 (ESP32→GPS RX), RX=GPIO47 (ESP32←GPS TX)");
    ESP_LOGI(TAG, "Power: 3.3V (Neo-M9N operates at 3.3V, NOT 5V!)");
    ESP_LOGI(TAG, "UART crossover wiring is CORRECT (TX→RX, RX→TX)");
    ESP_LOGI(TAG, "Using UART1 with GPIO 38/47 (NOT the console pins 43/44)");
    
    err = gps_init(GPS_UART_NUM, GPS_TX_PIN, GPS_RX_PIN, GPS_BAUD_RATE);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "✓ GPS initialized (waiting for satellite fix)");
        ESP_LOGI(TAG, "💡 GPS fix may take 1-5 minutes outdoors");
        ESP_LOGI(TAG, "💡 Check GPS mode (Mode 6) to see status");
    } else {
        ESP_LOGW(TAG, "❌ GPS initialization failed: %s", esp_err_to_name(err));
        ESP_LOGW(TAG, "💡 Check wiring: TX=44, RX=43, VCC=5V, GND=GND");
        ESP_LOGW(TAG, "💡 System will continue without GPS");
    }
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
    // USB SYSTEM INITIALIZATION - DIRECT SD CARD ACCESS
    // =========================================================================
    ESP_LOGI(TAG, "Initializing USB system with SD card MSC...");
    
    // Initialize USB MSC for direct SD card access
    err = usb_msc_sdcard_init();
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "✓ USB MSC SD card initialized");
        ESP_LOGI(TAG, "💾 SD card ready for direct USB access");
        ESP_LOGI(TAG, "🔄 Automatic logging pause/resume during USB access");
        ESP_LOGI(TAG, "🔌 Connect LEFT port to computer for flash drive access");
    } else {
        ESP_LOGW(TAG, "❌ USB MSC SD card initialization failed: %s", esp_err_to_name(err));
        ESP_LOGW(TAG, "💡 System will continue with SD card logging only");
        ESP_LOGW(TAG, "💡 USB MSC functionality disabled");
    }
    
    // Initialize USB Serial JTAG for RIGHT port (programming, monitoring, time sync)
    err = usb_interface_init();
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "✓ USB Serial JTAG initialized (RIGHT port)");
        ESP_LOGI(TAG, "🔧 Connect RIGHT port → Programming, monitoring, time sync");
        ESP_LOGI(TAG, "📝 Send TIME:YYYY-MM-DD HH:MM:SS for time sync");
        ESP_LOGI(TAG, "📝 Send USB_MSC_START for flash drive mode");
    } else {
        ESP_LOGW(TAG, "USB Serial JTAG initialization failed: %s", esp_err_to_name(err));
    }
    
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "🔌 USB System Overview:");
    ESP_LOGI(TAG, "   RIGHT port: Programming, monitoring, time sync, USB MSC control");
    ESP_LOGI(TAG, "   LEFT port:  Direct SD card access as USB flash drive");
    ESP_LOGI(TAG, "   SD card: Primary data logging + direct USB access");
    ESP_LOGI(TAG, "   Operation: Safe switching between logging and USB modes");
    ESP_LOGI(TAG, "💡 Connect LEFT port → Complete SD card appears as removable drive");
    ESP_LOGI(TAG, "💡 LED turns BLUE when USB flash drive is active");
    ESP_LOGI(TAG, "⚠️  Data logging pauses during USB access (safe switching)");
    ESP_LOGI(TAG, "");
    
    // Set manual time if not synchronized for proper logging
    if (!smart_time_is_valid()) {
        ESP_LOGW(TAG, "⚠️  Time not synchronized - setting manual time for logging");
        ESP_LOGW(TAG, "💡 Send TIME:2026-01-10 18:30:00 via USB Serial JTAG for accurate time");
        
        // Set a reasonable default time for logging (current date)
        esp_err_t time_ret = smart_time_set_manual("2026-01-10 18:30:00");
        if (time_ret == ESP_OK) {
            ESP_LOGI(TAG, "✓ Manual time set for logging purposes");
        } else {
            ESP_LOGW(TAG, "❌ Failed to set manual time: %s", esp_err_to_name(time_ret));
        }
    }
    
    // Get sensor health status
    sensor_manager_get_health(&health);
    ESP_LOGI(TAG, "Sensor Health:");
    ESP_LOGI(TAG, "  ToF: %s (errors: %lu)", 
             health.tof_healthy ? "✓ HEALTHY" : "✗ FAILED", health.tof_error_count);
    ESP_LOGI(TAG, "  IMU: %s (errors: %lu)", 
             health.imu_healthy ? "✓ HEALTHY" : "✗ FAILED", health.imu_error_count);
    ESP_LOGI(TAG, "");
    
    // =========================================================================
    // DATA LOGGING STARTUP (ALWAYS ACTIVE)
    // =========================================================================
    if (storage_manager_is_initialized()) {
        ESP_LOGI(TAG, "🚀 Starting continuous data logging...");
        ESP_LOGI(TAG, "💾 SD card logging is always active (no USB conflicts)");
        ESP_LOGI(TAG, "📁 Flash partition available for USB access");
        
        // Start logging immediately - no conflicts with flash partition USB MSC
        err = storage_manager_start_logging();
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "✓ Data logging started");
            ESP_LOGI(TAG, "💡 Use USB_SYNC_DATA to copy recent files to USB drive");
        } else {
            ESP_LOGW(TAG, "Failed to start logging: %s", esp_err_to_name(err));
            ESP_LOGW(TAG, "💡 This is likely due to SD card write protection");
        }
    } else {
        ESP_LOGW(TAG, "SD card not available - no data logging");
        ESP_LOGW(TAG, "💡 Flash partition USB MSC still available for testing");
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
    
    // Initialize display manager with config from SD card
    // Using COMPACT mode to avoid serial output freezes (50-100ms with full boxes)
    // Compact mode prints 2-3 lines instead of big ASCII boxes (~5ms vs 100ms)
    display_config.refresh_rate_hz = config.display_refresh_hz;  // Use config value
    display_config.show_tof_grid = true;        // Show ToF 4x4 grid (compact)
    display_config.show_imu_data = true;        // Show IMU data (compact)
    display_config.show_system_stats = true;    // Show logging statistics
    display_config.compact_mode = true;         // FORCE compact mode for performance
    
    err = display_manager_init(&display_config);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "✓ Display manager initialized (COMPACT mode for performance)");
        ESP_LOGI(TAG, "💡 Serial output: 2-3 lines instead of ASCII boxes");
    } else {
        ESP_LOGW(TAG, "Display manager initialization failed: %s", esp_err_to_name(err));
        ESP_LOGW(TAG, "System will use basic console output");
    }
    
    // =========================================================================
    // OLED DISPLAY INITIALIZATION (REAL-TIME 60 FPS VISUALIZATION - SPI MODE)
    // =========================================================================
    bool oled_available = false;
    if (config.oled_enabled) {
        ESP_LOGI(TAG, "Initializing OLED display (SSD1306 128x64 SPI)...");
        ESP_LOGI(TAG, "Wiring: SCK=GPIO7, MOSI=GPIO1, DC=GPIO35, RST=GPIO21");
        ESP_LOGI(TAG, "4-wire SPI mode (no CS pin required)");
        
        err = oled_display_init();
        if (err == ESP_OK) {
            oled_available = true;
            ESP_LOGI(TAG, "✓ OLED display initialized successfully");
            ESP_LOGI(TAG, "📺 Real-time sensor visualization at %d FPS", config.oled_refresh_hz);
            ESP_LOGI(TAG, "🎨 Display mode: %d (0=Dashboard, 1=ToF, 2=IMU, 3=Status)", config.oled_mode);
            ESP_LOGI(TAG, "💡 Brightness: %d/255", config.oled_brightness);
            
            // Set initial configuration
            oled_display_set_brightness(config.oled_brightness);
            
            // =========================================================================
            // STARTUP ANIMATION - Show Proxitof logo with loading progress
            // =========================================================================
            ESP_LOGI(TAG, "");
            ESP_LOGI(TAG, "╔════════════════════════════════════════════╗");
            ESP_LOGI(TAG, "║   Starting Startup Animation               ║");
            ESP_LOGI(TAG, "╚════════════════════════════════════════════╝");
            ESP_LOGI(TAG, "");
            
            // Stage 1: Display initialization (0%)
            oled_display_show_startup(0, "Starting...");
            vTaskDelay(pdMS_TO_TICKS(300));
            
            // Stage 2: Sensor manager ready (20%)
            oled_display_show_startup(20, "Sensors ready");
            vTaskDelay(pdMS_TO_TICKS(300));
            
            // Stage 3: Storage initialized (40%)
            oled_display_show_startup(40, "Storage ready");
            vTaskDelay(pdMS_TO_TICKS(300));
            
            // Stage 4: Time system ready (60%)
            oled_display_show_startup(60, "Time sync ready");
            vTaskDelay(pdMS_TO_TICKS(300));
            
            // Stage 5: USB system ready (80%)
            oled_display_show_startup(80, "USB ready");
            vTaskDelay(pdMS_TO_TICKS(300));
            
            // Stage 6: All systems ready (100%)
            oled_display_show_startup(100, "System ready!");
            vTaskDelay(pdMS_TO_TICKS(800));
            
            ESP_LOGI(TAG, "✓ Startup animation complete");
            ESP_LOGI(TAG, "");
            
            // Now set the actual display mode
            oled_display_set_mode((oled_display_mode_t)config.oled_mode);
        } else {
            ESP_LOGW(TAG, "❌ OLED display initialization failed: %s", esp_err_to_name(err));
            ESP_LOGW(TAG, "💡 Check wiring: SCK=GPIO7, MOSI=GPIO1, DC=GPIO35, RST=GPIO21, VCC=3.3V");
            ESP_LOGW(TAG, "💡 System will continue without OLED display");
        }
    } else {
        ESP_LOGI(TAG, "OLED display disabled in config.txt");
        ESP_LOGI(TAG, "💡 Set OLED_ENABLED=true to enable");
    }
    
    ESP_LOGI(TAG, "");
    
    // =========================================================================
    // DATA PROCESSOR INITIALIZATION (NEAR-MISS DETECTION)
    // =========================================================================
    ESP_LOGI(TAG, "Initializing Data Processor (Near-Miss Detection)...");
    ESP_LOGI(TAG, "Algorithm: Sensor fusion + Feature extraction + Rule-based detection");
    ESP_LOGI(TAG, "");
    
    data_processor = data_processor_create();
    if (data_processor == NULL) {
        ESP_LOGE(TAG, "❌ Failed to allocate DataProcessor");
        ESP_LOGE(TAG, "System will continue without near-miss detection");
    } else {
        // Configure synchronization window (10ms default)
        data_processor_set_sync_window(data_processor, 10);
        
        ESP_LOGI(TAG, "✓ Data Processor initialized successfully");
        ESP_LOGI(TAG, "");
        ESP_LOGI(TAG, "Near-Miss Detection Configuration:");
        ESP_LOGI(TAG, "  Sensor Sync Window: %lu ms", data_processor_get_sync_window(data_processor));
        ESP_LOGI(TAG, "  Rapid Approach Threshold: %.0f mm/s (%.2f m/s)", 
                 RAPID_APPROACH_THRESHOLD, RAPID_APPROACH_THRESHOLD / 1000.0f);
        ESP_LOGI(TAG, "  High Jerk Threshold: %.1f m/s³ (filters daily vibrations)", HIGH_JERK_THRESHOLD);
        ESP_LOGI(TAG, "  Critical Proximity: <%d mm with >%.0f mm/s approach", 
                 CRITICAL_PROXIMITY_DISTANCE, CRITICAL_PROXIMITY_APPROACH);
        ESP_LOGI(TAG, "  Combined Detection: Distance < %d mm (15cm near-miss zone)", COMBINED_DETECTION_MAX_DISTANCE);
        ESP_LOGI(TAG, "  Combined Detection Window: 500 ms");
        ESP_LOGI(TAG, "");
        ESP_LOGI(TAG, "Detection Scenario (Motorcycle Pass):");
        ESP_LOGI(TAG, "  Phase 1: Object approaches at 15cm + >1m/s (early warning)");
        ESP_LOGI(TAG, "  Phase 2: Body reacts with jerk >7m/s³ (confirmed near-miss)");
        ESP_LOGI(TAG, "");
        ESP_LOGI(TAG, "Filtered Out (Not Near-Miss):");
        ESP_LOGI(TAG, "  ✗ Turning: Gradual angular velocity (low jerk)");
        ESP_LOGI(TAG, "  ✗ Bumpy road: High-frequency vibration (filtered)");
        ESP_LOGI(TAG, "  ✗ Walking/pedaling: Periodic pattern (<5m/s³)");
        ESP_LOGI(TAG, "  ✓ Near-miss reaction: Sudden swerve (>7m/s³)");
    }
    
    ESP_LOGI(TAG, "");
    
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    // Clear screen for clean display
    printf("\033[2J\033[H");
    
    // Near-miss detection state tracking
    bool near_miss_active = false;
    uint32_t near_miss_start_time = 0;
    uint32_t near_miss_event_count = 0;
    
    while (1) {
        uint32_t current_time = sensor_manager_get_timestamp();
        
        // Check if USB MSC is active (SD card being accessed via USB)
        bool usb_msc_active = usb_msc_sdcard_is_active();
        
        // Debug USB MSC state every 5 seconds
        static uint32_t last_usb_debug = 0;
        if ((current_time - last_usb_debug) >= 5000) {
            last_usb_debug = current_time;
            ESP_LOGI(TAG, "🔌 USB Status: MSC_active=%s, USB_connected=%s", 
                     usb_msc_active ? "YES" : "NO",
                     usb_msc_sdcard_is_initialized() ? (tud_mounted() ? "YES" : "NO") : "DISABLED");
        }
        
        // Only read sensors and log data if NOT in USB MSC mode
        if (!usb_msc_active) {
            // Read ToF sensor at full speed (60Hz)
            err = sensor_manager_read_tof(&tof_data);
            if (err == ESP_OK) {
                tof_count++;
            } else if (err != ESP_ERR_NOT_FOUND && frame_count < 5) {
                // Log actual ToF errors during first few frames (ignore "no data ready")
                ESP_LOGW(TAG, "ToF read failed: %s", esp_err_to_name(err));
            }
            
            // Read IMU sensor at full speed (100Hz)
            err = sensor_manager_read_imu(&imu_data);
            if (err == ESP_OK) {
                imu_count++;
            }
            
            // =====================================================================
            // NEAR-MISS DETECTION PIPELINE (NEW)
            // =====================================================================
            bool near_miss_detected = false;
            feature_vector_t features = {0};
            
            if (data_processor != NULL && tof_count > 0 && imu_count > 0) {
                // Step 1: Synchronize sensor data (align ToF and IMU timestamps)
                synced_data_t synced_data = {0};
                bool sync_success = data_processor_synchronize_data(data_processor, &tof_data, &imu_data, &synced_data);
                
                if (sync_success) {
                    // Step 2: Extract features (raw + derived)
                    data_processor_extract_features(data_processor, &synced_data, &features);
                    
                    // Step 3: Detect near-miss conditions
                    near_miss_detected = data_processor_detect_near_miss(data_processor, &features);
                    
                    // Step 4: Handle near-miss event
                    if (near_miss_detected) {
                        if (!near_miss_active) {
                            // New near-miss event started
                            near_miss_active = true;
                            near_miss_start_time = current_time;
                            near_miss_event_count++;
                            
                            ESP_LOGW(TAG, "");
                            ESP_LOGW(TAG, "╔════════════════════════════════════════════════╗");
                            ESP_LOGW(TAG, "║  ⚠️  NEAR-MISS EVENT DETECTED #%lu              ║", near_miss_event_count);
                            ESP_LOGW(TAG, "╠════════════════════════════════════════════════╣");
                            ESP_LOGW(TAG, "║  Timestamp: %lu ms                            ║", current_time);
                            ESP_LOGW(TAG, "║  Min Distance: %u mm (%.2f m)                 ║", 
                                     features.min_distance_mm, features.min_distance_mm / 1000.0f);
                            ESP_LOGW(TAG, "║  Approach Rate: %.1f mm/s (%.2f m/s)          ║", 
                                     features.approach_rate_mm_s, features.approach_rate_mm_s / 1000.0f);
                            ESP_LOGW(TAG, "║  Jerk Magnitude: %.2f m/s³                    ║", 
                                     features.jerk_magnitude_m_s3);
                            ESP_LOGW(TAG, "║  Angular Velocity: %.1f deg/s                 ║", 
                                     features.angular_velocity_deg_s);
                            ESP_LOGW(TAG, "╚════════════════════════════════════════════════╝");
                            ESP_LOGW(TAG, "");
                            
                            // Set automatic flag for this event
                            features.flag_auto = true;
                        }
                    } else {
                        // Clear near-miss state after 2 seconds of no detection
                        if (near_miss_active && (current_time - near_miss_start_time) > 2000) {
                            near_miss_active = false;
                            ESP_LOGI(TAG, "✓ Near-miss event cleared (Duration: %.1f seconds)", 
                                     (current_time - near_miss_start_time) / 1000.0f);
                        }
                    }
                    
                    // Debug feature extraction every 300 frames (5 seconds at 60Hz)
                    static uint32_t debug_feature_count = 0;
                    debug_feature_count++;
                    if (debug_feature_count % 300 == 0) {
                        ESP_LOGI(TAG, "📊 Features: min_dist=%u mm, approach=%.1f mm/s, jerk=%.2f m/s³, ang_vel=%.1f deg/s",
                                 features.min_distance_mm, features.approach_rate_mm_s, 
                                 features.jerk_magnitude_m_s3, features.angular_velocity_deg_s);
                    }
                }
            }
            
            // Update LED with near-miss detection state
            uint16_t min_dist = get_min_distance(&tof_data);
            if (led_strip != NULL) {
                update_led(min_dist, usb_msc_active, near_miss_active);
            }
            
            // =====================================================================
            // DATA LOGGING WITH ENHANCED FEATURES
            // =====================================================================
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
                
                // Note: Derived features (min_distance, approach_rate, jerk, angular_velocity)
                // are calculated but not yet added to log_entry_t structure
                // This will be added in a future update to storage_manager
                
                // Write to buffer (non-blocking)
                esp_err_t log_err = storage_manager_write(&log_entry);
                
                // Debug logging every 300 frames (once every 5 seconds at 60Hz)
                static uint32_t debug_log_count = 0;
                debug_log_count++;
                if (debug_log_count % 300 == 0) {
                    storage_stats_t stats;
                    storage_manager_get_stats(&stats);
                    ESP_LOGI(TAG, "📊 Logging: %lu entries, %.1f%% buffer, %lu bytes, near-miss events: %lu", 
                             stats.entries_logged, stats.buffer_fill_percent, 
                             stats.current_file_size, near_miss_event_count);
                    
                    if (log_err != ESP_OK) {
                        ESP_LOGW(TAG, "⚠️  Log write failed: %s", esp_err_to_name(log_err));
                    }
                }
            } else {
                // Debug why logging is not happening
                static uint32_t debug_no_log_count = 0;
                debug_no_log_count++;
                if (debug_no_log_count % 300 == 0) { // Every 5 seconds
                    ESP_LOGW(TAG, "🚫 Not logging: storage_active=%s, tof_count=%lu, imu_count=%lu", 
                             storage_manager_is_logging() ? "YES" : "NO", tof_count, imu_count);
                }
            }
            
            // Display update using display manager (handles timing automatically)
            // Always update to ensure smooth display, even if sensor data hasn't changed
            // Reuse min_dist from LED update above
            display_manager_update(&tof_data, &imu_data, min_dist);
            
            // Update OLED display at configured refresh rate (60 FPS by default)
            if (oled_available) {
                static uint32_t last_oled_update = 0;
                uint32_t oled_interval_ms = 1000 / config.oled_refresh_hz;
                
                if ((current_time - last_oled_update) >= oled_interval_ms) {
                    oled_display_update(&tof_data, &imu_data, min_dist);
                    last_oled_update = current_time;
                }
            }
            
            // Check button status (check frequently for responsiveness)
            static uint32_t last_button_check = 0;
            if ((current_time - last_button_check) >= 50) { // Check every 50ms (20Hz)
                last_button_check = current_time;
                
                // Get current display mode to determine button behavior
                oled_display_mode_t current_mode = oled_available ? oled_display_get_mode() : 0;
                
                // Context-aware button handling
                if (current_mode == 0 || current_mode == 5) {  // Dashboard or Near-Miss mode
                    // Button 2 (GPIO 3): Manual event flagging
                    if (sensor_manager_is_button_held()) {
                        uint32_t duration = sensor_manager_get_button_duration();
                        
                        // Blue LED while button is held
                        if (led_strip != NULL) {
                            led_strip_set_pixel(led_strip, 0, 0, 0, 255);  // Blue
                            led_strip_refresh(led_strip);
                        }
                        
                        // Log every second while held
                        static uint32_t last_held_log = 0;
                        if ((current_time - last_held_log) >= 1000) {
                            ESP_LOGI(TAG, "⚠️  BUTTON HELD - Flagging event (Duration: %.1f seconds)", duration / 1000.0f);
                            last_held_log = current_time;
                        }
                    } else if (sensor_manager_is_button_pressed()) {
                        ESP_LOGI(TAG, "✓ EVENT FLAGGED - Button released (Timestamp: %lu ms)", 
                                 sensor_manager_get_last_button_press_time());
                        
                        // Flash blue LED briefly
                        if (led_strip != NULL) {
                            led_strip_set_pixel(led_strip, 0, 0, 0, 255);  // Blue
                            led_strip_refresh(led_strip);
                            vTaskDelay(pdMS_TO_TICKS(100));  // 100ms flash
                        }
                    }
                } else if (current_mode == 6 && oled_available) {  // WiFi Settings mode
                    // Button 2 (GPIO 3): Trigger WiFi sync
                    if (sensor_manager_is_button_pressed()) {
                        ESP_LOGI(TAG, "🌐 WiFi sync triggered from button");
                        
                        // Blue LED during sync
                        if (led_strip != NULL) {
                            led_strip_set_pixel(led_strip, 0, 0, 0, 255);  // Blue
                            led_strip_refresh(led_strip);
                        }
                        
                        oled_display_handle_action_button();
                    }
                } else if (current_mode == 3 && oled_available) {  // IMU Focus mode
                    // Button 2 (GPIO 3): Trigger IMU rescan
                    if (sensor_manager_is_button_pressed()) {
                        ESP_LOGI(TAG, "🔄 IMU rescan triggered from button");
                        
                        // Yellow LED during rescan
                        if (led_strip != NULL) {
                            led_strip_set_pixel(led_strip, 0, 255, 255, 0);  // Yellow
                            led_strip_refresh(led_strip);
                        }
                        
                        oled_display_handle_action_button();
                    }
                } else if (current_mode == 4 && oled_available) {  // GPS mode
                    // Button 2 (GPIO 3): Trigger GPS reconnect
                    if (sensor_manager_is_button_pressed()) {
                        ESP_LOGI(TAG, "📡 GPS reconnect triggered from button");
                        
                        // Green LED during reconnect
                        if (led_strip != NULL) {
                            led_strip_set_pixel(led_strip, 0, 0, 255, 0);  // Green
                            led_strip_refresh(led_strip);
                        }
                        
                        oled_display_handle_action_button();
                    }
                } else if (current_mode == 7 && oled_available) {  // Shutdown mode
                    // Button 2 (GPIO 3): Confirm shutdown
                    if (sensor_manager_is_button_pressed()) {
                        ESP_LOGW(TAG, "🔴 Shutdown button pressed");
                        
                        // Red LED during shutdown
                        if (led_strip != NULL) {
                            led_strip_set_pixel(led_strip, 0, 255, 0, 0);  // Red
                            led_strip_refresh(led_strip);
                        }
                        
                        oled_display_handle_action_button();
                        
                        // Check if shutdown was confirmed
                        if (oled_display_is_shutdown_confirmed()) {
                            ESP_LOGW(TAG, "");
                            ESP_LOGW(TAG, "╔════════════════════════════════════════════════╗");
                            ESP_LOGW(TAG, "║  🔴 SHUTDOWN CONFIRMED - POWERING DOWN         ║");
                            ESP_LOGW(TAG, "╚════════════════════════════════════════════════╝");
                            ESP_LOGW(TAG, "");
                            
                            // Stop data logging
                            ESP_LOGI(TAG, "Stopping data logging...");
                            storage_manager_stop_logging();
                            vTaskDelay(pdMS_TO_TICKS(500));
                            
                            // Flush all data to SD card
                            ESP_LOGI(TAG, "Flushing data to SD card...");
                            storage_manager_flush();
                            vTaskDelay(pdMS_TO_TICKS(1000));
                            
                            // Stop sensors
                            ESP_LOGI(TAG, "Stopping sensors...");
                            // ToF and IMU will be stopped when power is cut
                            
                            // Final display message
                            ESP_LOGI(TAG, "✓ All data saved");
                            ESP_LOGI(TAG, "✓ Safe to power off");
                            ESP_LOGI(TAG, "");
                            ESP_LOGI(TAG, "You can now switch off the device");
                            
                            // Show final message on OLED
                            vTaskDelay(pdMS_TO_TICKS(1000));
                            
                            // Turn off LED
                            if (led_strip != NULL) {
                                led_strip_clear(led_strip);
                            }
                            
                            // Enter deep sleep (effectively powers down)
                            ESP_LOGI(TAG, "Entering deep sleep...");
                            vTaskDelay(pdMS_TO_TICKS(2000));
                            esp_deep_sleep_start();
                        }
                    }
                }
                // Other modes: Button 2 does nothing
            }
            
            // Check mode switch button (only if OLED is enabled)
            if (oled_available) {
                check_mode_button();
            }
        } else {
            // USB MSC mode - show blue LED and minimal processing
            if (led_strip != NULL) {
                update_led(0, true, false);  // Force blue LED (no near-miss in USB mode)
            }
            
            // Longer delay in USB mode to reduce CPU usage
            vTaskDelay(pdMS_TO_TICKS(100));  // 10Hz loop rate in USB mode
        }
        
        // Small delay to prevent CPU spinning (check sensors frequently in normal mode)
        if (!usb_msc_active) {
            vTaskDelay(pdMS_TO_TICKS(5));  // 200Hz loop rate for responsive LED
        }
        
        // Smart time maintenance (every ~60 seconds)
        smart_time_maintenance();
        
        // USB interface processing (handle commands from RIGHT port)
        usb_interface_process();
        
        // Handle USB MSC events and logging pause/resume
        usb_msc_sdcard_process();
        
        frame_count++;
    }
}
