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
        } else if (strcmp(key, "DISPLAY_REFRESH_HZ") == 0) {
            int hz = atoi(value);
            if (hz >= 1 && hz <= 60) {
                config->display_refresh_hz = hz;
            } else {
                ESP_LOGW(TAG, "Invalid display refresh rate: %d (using default)", hz);
            }
        } else if (strcmp(key, "DISPLAY_COMPACT_MODE") == 0) {
            config->display_compact_mode = (strcmp(value, "true") == 0 || strcmp(value, "1") == 0);
        } else {
            ESP_LOGW(TAG, "Unknown config key: %s", key);
        }
    }
    
    fclose(file);
    
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
    
    fprintf(file, "# Display Configuration\n");
    fprintf(file, "DISPLAY_REFRESH_HZ=%d\n", config->display_refresh_hz);
    fprintf(file, "DISPLAY_COMPACT_MODE=%s\n", config->display_compact_mode ? "true" : "false");
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
    ESP_LOGI(TAG, "  Timezone: %s", config->timezone);
    ESP_LOGI(TAG, "  Sync Interval: %d hours", config->sync_interval_hours);
    ESP_LOGI(TAG, "  Display: %d Hz, %s mode", 
             config->display_refresh_hz, 
             config->display_compact_mode ? "compact" : "full");
}