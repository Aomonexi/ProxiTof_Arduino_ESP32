/**
 * config_manager.c
 * 
 * SD Card Configuration Manager Implementation
 */

#include "config_manager.h"
#include "esp_log.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

static const char *TAG = "CONFIG";

#define CONFIG_FILE_PATH "/sdcard/config.txt"

// Global configuration (loaded at startup)
static system_config_t g_config;
static bool g_config_loaded = false;

void config_load_defaults(system_config_t *config) {
    if (!config) return;
    
    // Default configuration - Edit these or use SD card config.txt
    strcpy(config->wifi_ssid, "YourWiFiNetwork");
    strcpy(config->wifi_password, "YourWiFiPassword");
    strcpy(config->timezone, "CST-8");  // China Standard Time (UTC+8)
    config->ds3231_sda_pin = 8;   // Updated to match I2C bus with ToF sensor
    config->ds3231_scl_pin = 9;   // Updated to match I2C bus with ToF sensor
    config->sync_interval_hours = 24;
    config->display_refresh_hz = 2;  // Default 2Hz for low CPU usage
    config->wifi_enabled = true;
    config->ds3231_enabled = true;
    config->display_compact_mode = false;
    config->gps_logging_enabled = false;  // Default: disabled for ML training
    // OLED Display defaults
    config->oled_enabled = true;
    config->oled_refresh_hz = 60;  // 60 Hz with frame buffer - smooth and flicker-free!
    config->oled_brightness = 128;  // Medium brightness (0-255)
    config->oled_mode = 0;  // Dashboard mode
    config->oled_flip = false;
    // Near-Miss Detection defaults
    config->detection_mode = 0;  // 0=Hybrid (default)
    // GPS Dynamic Model defaults
    config->gps_dynamic_model = 10;  // 10=Bike (default for cycling safety device)
    // GPS Update Rate defaults
    config->gps_update_rate = 5;  // 5 Hz (optimal for cycling - smooth tracking, good battery life)
}

bool config_file_exists(void) {
    struct stat st;
    return (stat(CONFIG_FILE_PATH, &st) == 0);
}

esp_err_t config_load_from_sd(system_config_t *config) {
    if (!config) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // Load defaults first
    config_load_defaults(config);
    
    FILE *file = fopen(CONFIG_FILE_PATH, "r");
    if (!file) {
        ESP_LOGW(TAG, "Config file not found: %s", CONFIG_FILE_PATH);
        ESP_LOGI(TAG, "Using default configuration");
        
        // Store in global config
        memcpy(&g_config, config, sizeof(system_config_t));
        g_config_loaded = true;
        
        return ESP_ERR_NOT_FOUND;
    }
    
    ESP_LOGI(TAG, "Loading configuration from SD card...");
    
    char line[256];
    int line_num = 0;
    
    while (fgets(line, sizeof(line), file)) {
        line_num++;
        
        // Remove newline
        line[strcspn(line, "\r\n")] = 0;
        
        // Skip empty lines and comments
        if (line[0] == '\0' || line[0] == '#') {
            continue;
        }
        
        // Parse key=value pairs
        char *equals = strchr(line, '=');
        if (!equals) {
            ESP_LOGW(TAG, "Invalid line %d: %s", line_num, line);
            continue;
        }
        
        *equals = '\0';
        char *key = line;
        char *value = equals + 1;
        
        // Trim whitespace
        while (*key == ' ' || *key == '\t') key++;
        while (*value == ' ' || *value == '\t') value++;
        
        // Parse configuration values
        if (strcmp(key, "WIFI_SSID") == 0) {
            strncpy(config->wifi_ssid, value, CONFIG_MAX_STRING_LEN - 1);
        } else if (strcmp(key, "WIFI_PASSWORD") == 0) {
            strncpy(config->wifi_password, value, CONFIG_MAX_STRING_LEN - 1);
        } else if (strcmp(key, "TIMEZONE") == 0) {
            strncpy(config->timezone, value, CONFIG_MAX_STRING_LEN - 1);
        } else if (strcmp(key, "DS3231_SDA_PIN") == 0) {
            config->ds3231_sda_pin = atoi(value);
        } else if (strcmp(key, "DS3231_SCL_PIN") == 0) {
            config->ds3231_scl_pin = atoi(value);
        } else if (strcmp(key, "SYNC_INTERVAL_HOURS") == 0) {
            config->sync_interval_hours = atoi(value);
        } else if (strcmp(key, "WIFI_ENABLED") == 0) {
            config->wifi_enabled = (strcmp(value, "true") == 0 || strcmp(value, "1") == 0);
        } else if (strcmp(key, "DS3231_ENABLED") == 0) {
            config->ds3231_enabled = (strcmp(value, "true") == 0 || strcmp(value, "1") == 0);
        } else if (strcmp(key, "GPS_LOGGING_ENABLED") == 0) {
            config->gps_logging_enabled = (strcmp(value, "true") == 0 || strcmp(value, "1") == 0);
        } else if (strcmp(key, "DISPLAY_REFRESH_HZ") == 0) {
            int hz = atoi(value);
            if (hz >= 1 && hz <= 60) {
                config->display_refresh_hz = hz;
            } else {
                ESP_LOGW(TAG, "Invalid display refresh rate: %d (using default)", hz);
            }
        } else if (strcmp(key, "DISPLAY_COMPACT_MODE") == 0) {
            config->display_compact_mode = (strcmp(value, "true") == 0 || strcmp(value, "1") == 0);
        } else if (strcmp(key, "OLED_ENABLED") == 0) {
            config->oled_enabled = (strcmp(value, "true") == 0 || strcmp(value, "1") == 0);
        } else if (strcmp(key, "OLED_REFRESH_HZ") == 0) {
            int hz = atoi(value);
            if (hz >= 1 && hz <= 60) {
                config->oled_refresh_hz = hz;
            } else {
                ESP_LOGW(TAG, "Invalid OLED refresh rate: %d (using default)", hz);
            }
        } else if (strcmp(key, "OLED_BRIGHTNESS") == 0) {
            int brightness = atoi(value);
            if (brightness >= 0 && brightness <= 255) {
                config->oled_brightness = brightness;
            } else {
                ESP_LOGW(TAG, "Invalid OLED brightness: %d (using default)", brightness);
            }
        } else if (strcmp(key, "OLED_MODE") == 0) {
            int mode = atoi(value);
            if (mode >= 0 && mode <= 3) {
                config->oled_mode = mode;
            } else {
                ESP_LOGW(TAG, "Invalid OLED mode: %d (using default)", mode);
            }
        } else if (strcmp(key, "OLED_FLIP") == 0) {
            config->oled_flip = (strcmp(value, "true") == 0 || strcmp(value, "1") == 0);
        } else if (strcmp(key, "DETECTION_MODE") == 0) {
            int mode = atoi(value);
            if (mode >= 0 && mode <= 2) {
                config->detection_mode = mode;
            } else {
                ESP_LOGW(TAG, "Invalid detection mode: %d (using default)", mode);
            }
        } else if (strcmp(key, "GPS_DYNAMIC_MODEL") == 0) {
            int model = atoi(value);
            if (model >= 0 && model <= 10) {
                config->gps_dynamic_model = model;
            } else {
                ESP_LOGW(TAG, "Invalid GPS dynamic model: %d (using default)", model);
            }
        } else if (strcmp(key, "GPS_UPDATE_RATE") == 0) {
            int rate = atoi(value);
            if (rate >= 1 && rate <= 25) {
                config->gps_update_rate = rate;
            } else {
                ESP_LOGW(TAG, "Invalid GPS update rate: %d Hz (using default)", rate);
            }
        } else {
            ESP_LOGW(TAG, "Unknown config key: %s", key);
        }
    }
    
    fclose(file);
    
    // Store in global config
    memcpy(&g_config, config, sizeof(system_config_t));
    g_config_loaded = true;
    
    ESP_LOGI(TAG, "✓ Configuration loaded from SD card");
    return ESP_OK;
}

esp_err_t config_save_to_sd(const system_config_t *config) {
    if (!config) {
        return ESP_ERR_INVALID_ARG;
    }
    
    FILE *file = fopen(CONFIG_FILE_PATH, "w");
    if (!file) {
        ESP_LOGE(TAG, "Failed to create config file: %s", CONFIG_FILE_PATH);
        return ESP_ERR_NO_MEM;
    }
    
    ESP_LOGI(TAG, "Saving configuration to SD card...");
    
    fprintf(file, "# ProxiTof System Configuration\n");
    fprintf(file, "# Edit this file to change settings without recompiling\n");
    fprintf(file, "# Remove SD card, edit on computer, insert back\n");
    fprintf(file, "\n");
    
    fprintf(file, "# WiFi Configuration\n");
    fprintf(file, "WIFI_ENABLED=%s\n", config->wifi_enabled ? "true" : "false");
    fprintf(file, "WIFI_SSID=%s\n", config->wifi_ssid);
    fprintf(file, "WIFI_PASSWORD=%s\n", config->wifi_password);
    fprintf(file, "\n");
    
    fprintf(file, "# Time Configuration\n");
    fprintf(file, "TIMEZONE=%s\n", config->timezone);
    fprintf(file, "SYNC_INTERVAL_HOURS=%d\n", config->sync_interval_hours);
    fprintf(file, "\n");
    
    fprintf(file, "# DS3231 RTC Configuration\n");
    fprintf(file, "DS3231_ENABLED=%s\n", config->ds3231_enabled ? "true" : "false");
    fprintf(file, "DS3231_SDA_PIN=%d\n", config->ds3231_sda_pin);
    fprintf(file, "DS3231_SCL_PIN=%d\n", config->ds3231_scl_pin);
    fprintf(file, "\n");
    
    fprintf(file, "# GPS Logging Configuration\n");
    fprintf(file, "GPS_LOGGING_ENABLED=%s\n", config->gps_logging_enabled ? "true" : "false");
    fprintf(file, "\n");
    
    fprintf(file, "# Console Display Configuration\n");
    fprintf(file, "DISPLAY_REFRESH_HZ=%d\n", config->display_refresh_hz);
    fprintf(file, "DISPLAY_COMPACT_MODE=%s\n", config->display_compact_mode ? "true" : "false");
    fprintf(file, "\n");
    
    fprintf(file, "# OLED Display Configuration (0.96\" SSD1306 SPI)\n");
    fprintf(file, "OLED_ENABLED=%s\n", config->oled_enabled ? "true" : "false");
    fprintf(file, "OLED_REFRESH_HZ=%d  # 1-60 Hz (60 recommended for smooth display)\n", config->oled_refresh_hz);
    fprintf(file, "OLED_BRIGHTNESS=%d  # 0-255 (128 = medium)\n", config->oled_brightness);
    fprintf(file, "OLED_MODE=%d  # 0=Dashboard, 1=ToF Focus, 2=IMU Focus, 3=Status\n", config->oled_mode);
    fprintf(file, "OLED_FLIP=%s  # Rotate display 180 degrees\n", config->oled_flip ? "true" : "false");
    fprintf(file, "\n");
    
    fprintf(file, "# Near-Miss Detection Configuration\n");
    fprintf(file, "DETECTION_MODE=%d  # 0=Hybrid, 1=Algorithm, 2=Neural Network\n", config->detection_mode);
    fprintf(file, "\n");
    
    fprintf(file, "# GPS Dynamic Model Configuration\n");
    fprintf(file, "# Controls GPS tracking optimization for different movement types\n");
    fprintf(file, "# ALL models 0-10 are functional and can be set here\n");
    fprintf(file, "# 0=Portable, 1=Reserved, 2=Stationary, 3=Pedestrian, 4=Automotive\n");
    fprintf(file, "# 5=Sea, 6=Airborne<1g, 7=Airborne<2g, 8=Airborne<4g, 9=Wrist, 10=Bike\n");
    fprintf(file, "# Bike mode (10) is optimized for cycling: quick turns, moderate speed, stop-and-go\n");
    fprintf(file, "# Pedestrian mode (3) for walking data collection\n");
    fprintf(file, "# Press and HOLD ACTION button in GPS mode to cycle between modes\n");
    fprintf(file, "GPS_DYNAMIC_MODEL=%d\n", config->gps_dynamic_model);
    fprintf(file, "\n");
    
    fprintf(file, "# GPS Update Rate Configuration\n");
    fprintf(file, "# Controls how often GPS provides position updates\n");
    fprintf(file, "# 1 Hz  = 1 update/second (default, best battery life)\n");
    fprintf(file, "# 5 Hz  = 5 updates/second (optimal for cycling, smooth tracking) ← RECOMMENDED\n");
    fprintf(file, "# 10 Hz = 10 updates/second (high precision, more power)\n");
    fprintf(file, "# 25 Hz = 25 updates/second (maximum rate, research only)\n");
    fprintf(file, "# Valid range: 1-25 Hz\n");
    fprintf(file, "GPS_UPDATE_RATE=%d\n", config->gps_update_rate);
    fprintf(file, "\n");
    
    fprintf(file, "# Common Timezone Examples:\n");
    fprintf(file, "# UTC: UTC0\n");
    fprintf(file, "# US Eastern: EST5EDT,M3.2.0,M11.1.0\n");
    fprintf(file, "# US Pacific: PST8PDT,M3.2.0,M11.1.0\n");
    fprintf(file, "# Europe/Berlin: CET-1CEST,M3.5.0,M10.5.0/3\n");
    fprintf(file, "# Asia/Tokyo: JST-9\n");
    
    fclose(file);
    
    ESP_LOGI(TAG, "✓ Configuration saved to SD card");
    return ESP_OK;
}

void config_print(const system_config_t *config) {
    if (!config) return;
    
    ESP_LOGI(TAG, "System Configuration:");
    ESP_LOGI(TAG, "  WiFi: %s", config->wifi_enabled ? "Enabled" : "Disabled");
    if (config->wifi_enabled) {
        ESP_LOGI(TAG, "    SSID: %s", config->wifi_ssid);
        ESP_LOGI(TAG, "    Password: %s", strlen(config->wifi_password) > 0 ? "***" : "(empty)");
    }
    ESP_LOGI(TAG, "  DS3231: %s", config->ds3231_enabled ? "Enabled" : "Disabled");
    if (config->ds3231_enabled) {
        ESP_LOGI(TAG, "    SDA Pin: %d", config->ds3231_sda_pin);
        ESP_LOGI(TAG, "    SCL Pin: %d", config->ds3231_scl_pin);
    }
    ESP_LOGI(TAG, "  GPS Logging: %s", config->gps_logging_enabled ? "Enabled (production)" : "Disabled (ML training)");
    ESP_LOGI(TAG, "  Timezone: %s", config->timezone);
    ESP_LOGI(TAG, "  Sync Interval: %d hours", config->sync_interval_hours);
    ESP_LOGI(TAG, "  Console Display: %d Hz, %s mode", 
             config->display_refresh_hz, 
             config->display_compact_mode ? "compact" : "full");
    ESP_LOGI(TAG, "  OLED Display: %s", config->oled_enabled ? "Enabled" : "Disabled");
    if (config->oled_enabled) {
        ESP_LOGI(TAG, "    Refresh: %d Hz", config->oled_refresh_hz);
        ESP_LOGI(TAG, "    Brightness: %d/255", config->oled_brightness);
        ESP_LOGI(TAG, "    Mode: %d", config->oled_mode);
    }
}

const system_config_t* config_manager_get_config(void) {
    if (!g_config_loaded) {
        ESP_LOGW(TAG, "Config not loaded yet, returning defaults");
        config_load_defaults(&g_config);
        g_config_loaded = true;
    }
    return &g_config;
}

esp_err_t config_manager_reload(void) {
    ESP_LOGI(TAG, "Reloading configuration from SD card...");
    
    system_config_t temp_config;
    esp_err_t err = config_load_from_sd(&temp_config);
    
    if (err == ESP_OK) {
        // Update global config
        memcpy(&g_config, &temp_config, sizeof(system_config_t));
        g_config_loaded = true;
        ESP_LOGI(TAG, "✓ Configuration reloaded successfully");
    } else {
        ESP_LOGW(TAG, "Failed to reload config: %s", esp_err_to_name(err));
    }
    
    return err;
}