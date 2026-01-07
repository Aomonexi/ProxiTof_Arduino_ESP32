/**
 * smart_time.c
 * 
 * Smart Time Management Implementation
 * Handles DS3231, Internal RTC, and WiFi+NTP with automatic fallback
 */

#include "smart_time.h"
#include "ds3231.h"
#include "time_sync.h"
#include "sensor_manager.h"
#include "esp_log.h"
#include <sys/time.h>
#include <string.h>

static const char *TAG = "SMART_TIME";

// DS3231 device structure
static i2c_dev_t ds3231_dev;
static bool ds3231_available = false;

// Configuration
static bool ds3231_enabled = false;
static bool wifi_enabled = false;
static char wifi_ssid[64] = {0};
static char wifi_password[64] = {0};
static char timezone_str[64] = {0};

// Status tracking
static time_status_t status = {0};
static time_t last_maintenance = 0;
static time_t last_sync_attempt = 0;

// Time validation
#define MIN_VALID_TIME 1577836800  // Jan 1, 2020
static int max_sync_interval = 24 * 3600;  // Default 24 hours, configurable
#define MAINTENANCE_INTERVAL 60  // 60 seconds

static bool is_time_valid(time_t t) {
    return (t > MIN_VALID_TIME && t < (MIN_VALID_TIME + 20LL * 365 * 24 * 3600)); // Before 2040
}

static bool check_ds3231_available(void) {
    if (!ds3231_enabled) {
        return false;
    }
    
    // Try to read time from DS3231 to test connectivity
    struct tm test_time;
    esp_err_t ret = ds3231_get_time(&ds3231_dev, &test_time);
    return (ret == ESP_OK);
}

static esp_err_t update_internal_rtc(struct tm *timeinfo) {
    time_t t = mktime(timeinfo);
    if (!is_time_valid(t)) {
        return ESP_ERR_INVALID_ARG;
    }
    
    struct timeval tv = { .tv_sec = t };
    settimeofday(&tv, NULL);
    
    ESP_LOGI(TAG, "✓ Internal RTC updated: %04d-%02d-%02d %02d:%02d:%02d",
             timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday,
             timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
    
    return ESP_OK;
}

static esp_err_t get_ds3231_time(struct tm *timeinfo) {
    if (!ds3231_enabled) {
        return ESP_ERR_NOT_SUPPORTED;
    }
    
    esp_err_t ret = ds3231_get_time(&ds3231_dev, timeinfo);
    if (ret != ESP_OK) {
        status.ds3231_error_count++;
        if (status.ds3231_error_count > 3) {
            status.ds3231_available = false;
            ESP_LOGW(TAG, "DS3231 marked as unavailable after %lu errors", status.ds3231_error_count);
        }
        return ret;
    }
    
    // Reset error count on success
    if (status.ds3231_error_count > 0) {
        ESP_LOGI(TAG, "DS3231 recovered after %lu errors", status.ds3231_error_count);
        status.ds3231_error_count = 0;
    }
    
    status.ds3231_available = true;
    return ESP_OK;
}

static esp_err_t sync_from_ntp(void) {
    if (!wifi_enabled) {
        return ESP_ERR_NOT_SUPPORTED;
    }
    
    ESP_LOGI(TAG, "Attempting NTP sync...");
    esp_err_t ret = time_sync_init(wifi_ssid, wifi_password, timezone_str);
    
    if (ret == ESP_OK) {
        time_t now;
        struct tm timeinfo;
        time(&now);
        localtime_r(&now, &timeinfo);
        
        // Update DS3231 if available
        if (ds3231_enabled && status.ds3231_available) {
            esp_err_t ds_ret = ds3231_set_time(&ds3231_dev, &timeinfo);
            if (ds_ret == ESP_OK) {
                ESP_LOGI(TAG, "✓ DS3231 updated from NTP");
            } else {
                ESP_LOGW(TAG, "Failed to update DS3231: %s", esp_err_to_name(ds_ret));
            }
        }
        
        status.time_synced = true;
        status.current_source = TIME_SOURCE_NTP;
        status.last_sync_time = now;
        status.sync_count++;
        
        // Disconnect WiFi to save power
        time_sync_disconnect_wifi();
        
        ESP_LOGI(TAG, "✓ NTP sync successful, WiFi disconnected");
        return ESP_OK;
    } else {
        ESP_LOGW(TAG, "NTP sync failed: %s", esp_err_to_name(ret));
        return ret;
    }
}

esp_err_t smart_time_init(int ds3231_sda, int ds3231_scl, 
                         const char* wifi_ssid_param, const char* wifi_password_param, 
                         const char* timezone, int sync_interval_hours) {
    ESP_LOGI(TAG, "Initializing Smart Time Management...");
    
    // Initialize DS3231 if pins provided
    if (ds3231_sda >= 0 && ds3231_scl >= 0) {
        // Try to get shared I2C bus from sensor manager first
        i2c_master_bus_handle_t shared_bus = sensor_manager_get_i2c_bus();
        
        esp_err_t ret;
        if (shared_bus != NULL) {
            ESP_LOGI(TAG, "Using shared I2C bus from sensor manager");
            ret = ds3231_init_desc_shared(&ds3231_dev, shared_bus);
        } else {
            ESP_LOGI(TAG, "Creating dedicated I2C bus for DS3231");
            ret = ds3231_init_desc(&ds3231_dev, I2C_NUM_0, ds3231_sda, ds3231_scl);
        }
        
        if (ret == ESP_OK) {
            ds3231_enabled = true;
            ds3231_available = true;
            status.ds3231_available = true;
            ESP_LOGI(TAG, "✓ DS3231 RTC initialized (SDA=%d, SCL=%d)", ds3231_sda, ds3231_scl);
        } else {
            ESP_LOGW(TAG, "DS3231 initialization failed: %s", esp_err_to_name(ret));
            ds3231_available = false;
        }
    }
    
    // Store WiFi credentials if provided
    if (wifi_ssid_param && wifi_password_param) {
        strncpy(wifi_ssid, wifi_ssid_param, sizeof(wifi_ssid) - 1);
        strncpy(wifi_password, wifi_password_param, sizeof(wifi_password) - 1);
        wifi_enabled = true;
        ESP_LOGI(TAG, "✓ WiFi credentials configured");
    }
    
    // Store timezone
    if (timezone) {
        strncpy(timezone_str, timezone, sizeof(timezone_str) - 1);
    } else {
        strcpy(timezone_str, "UTC0");
    }
    
    // Try to get time from best available source
    struct tm timeinfo;
    esp_err_t ret = smart_time_get(&timeinfo);
    
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "✓ Time available from %s", smart_time_source_name(status.current_source));
        ESP_LOGI(TAG, "Current time: %04d-%02d-%02d %02d:%02d:%02d",
                 timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
                 timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    } else {
        ESP_LOGW(TAG, "No valid time source available");
        
        // Try NTP sync if WiFi is available
        if (wifi_enabled) {
            ESP_LOGI(TAG, "Attempting initial NTP sync...");
            sync_from_ntp();
        }
    }
    
    // Store sync interval
    if (sync_interval_hours > 0) {
        max_sync_interval = sync_interval_hours * 3600;
        ESP_LOGI(TAG, "Sync interval: %d hours", sync_interval_hours);
    }
    
    last_maintenance = time(NULL);
    
    ESP_LOGI(TAG, "✓ Smart Time Management initialized");
    ESP_LOGI(TAG, "Sources: DS3231=%s, WiFi=%s", 
             ds3231_enabled ? "enabled" : "disabled",
             wifi_enabled ? "enabled" : "disabled");
    
    return ESP_OK;
}

esp_err_t smart_time_get(struct tm *timeinfo) {
    if (!timeinfo) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // Try DS3231 first (most reliable)
    if (ds3231_enabled && status.ds3231_available) {
        esp_err_t ret = get_ds3231_time(timeinfo);
        if (ret == ESP_OK) {
            time_t t = mktime(timeinfo);
            if (is_time_valid(t)) {
                status.current_source = TIME_SOURCE_DS3231;
                // Sync internal RTC with DS3231
                update_internal_rtc(timeinfo);
                return ESP_OK;
            }
        }
    }
    
    // Fallback to internal RTC
    time_t now;
    time(&now);
    if (is_time_valid(now)) {
        localtime_r(&now, timeinfo);
        status.current_source = TIME_SOURCE_INTERNAL_RTC;
        return ESP_OK;
    }
    
    // No valid time available
    status.current_source = TIME_SOURCE_NONE;
    return ESP_ERR_NOT_FOUND;
}

bool smart_time_is_valid(void) {
    struct tm timeinfo;
    return (smart_time_get(&timeinfo) == ESP_OK);
}

void smart_time_get_status(time_status_t *out_status) {
    if (out_status) {
        memcpy(out_status, &status, sizeof(time_status_t));
    }
}

esp_err_t smart_time_sync_now(void) {
    if (!wifi_enabled) {
        return ESP_ERR_NOT_SUPPORTED;
    }
    
    last_sync_attempt = time(NULL);
    return sync_from_ntp();
}

esp_err_t smart_time_set_manual(const char* time_str) {
    if (!time_str) {
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGI(TAG, "Setting time manually: %s", time_str);
    
    // Use the time_sync function to set time
    esp_err_t ret = time_sync_set_manual_time(time_str);
    if (ret == ESP_OK) {
        status.time_synced = true;
        status.current_source = TIME_SOURCE_USB;  // Assume USB if called from USB interface
        status.last_sync_time = time(NULL);
        status.sync_count++;
        
        // Try to update DS3231 if available
        if (ds3231_enabled && status.ds3231_available) {
            struct tm timeinfo;
            time_t now = time(NULL);
            localtime_r(&now, &timeinfo);
            
            esp_err_t ds_ret = ds3231_set_time(&ds3231_dev, &timeinfo);
            if (ds_ret == ESP_OK) {
                ESP_LOGI(TAG, "✓ DS3231 updated with synchronized time");
            } else {
                ESP_LOGW(TAG, "Failed to update DS3231: %s", esp_err_to_name(ds_ret));
            }
        }
        
        ESP_LOGI(TAG, "✓ Time synchronized successfully");
    }
    
    return ret;
}

void smart_time_maintenance(void) {
    time_t now = time(NULL);
    
    // Skip if not enough time passed
    if (now - last_maintenance < MAINTENANCE_INTERVAL) {
        return;
    }
    
    last_maintenance = now;
    
    // Check if DS3231 is still available (if it was working before)
    if (ds3231_enabled) {
        bool currently_available = check_ds3231_available();
        if (currently_available != status.ds3231_available) {
            if (currently_available) {
                ESP_LOGI(TAG, "DS3231 recovered!");
                status.ds3231_available = true;
                status.ds3231_error_count = 0;
            } else {
                ESP_LOGW(TAG, "DS3231 connection lost");
                status.ds3231_available = false;
                status.ds3231_error_count++;
            }
        }
    }
    
    // Auto-sync if needed and WiFi available
    if (wifi_enabled && status.time_synced) {
        time_t time_since_sync = now - status.last_sync_time;
        time_t time_since_attempt = now - last_sync_attempt;
        
        // Try sync if it's been more than configured interval and at least 1 hour since last attempt
        if (time_since_sync > max_sync_interval && time_since_attempt > 3600) {
            ESP_LOGI(TAG, "Auto-sync triggered (last sync: %ld hours ago)", time_since_sync / 3600);
            sync_from_ntp();
        }
    }
}

const char* smart_time_source_name(time_source_t source) {
    switch (source) {
        case TIME_SOURCE_DS3231: return "DS3231 RTC";
        case TIME_SOURCE_INTERNAL_RTC: return "Internal RTC";
        case TIME_SOURCE_NTP: return "NTP";
        case TIME_SOURCE_USB: return "USB";
        case TIME_SOURCE_MANUAL: return "Manual";
        case TIME_SOURCE_NONE: return "None";
        default: return "Unknown";
    }
}