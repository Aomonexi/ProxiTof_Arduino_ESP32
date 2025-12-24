/**
 * smart_time.h
 * 
 * Smart Time Management System
 * Multi-tier time keeping with automatic fallback
 */

#pragma once

#include "esp_err.h"
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    TIME_SOURCE_NONE = 0,
    TIME_SOURCE_INTERNAL_RTC,
    TIME_SOURCE_DS3231,
    TIME_SOURCE_NTP
} time_source_t;

typedef struct {
    bool ds3231_available;
    bool time_synced;
    time_source_t current_source;
    time_t last_sync_time;
    uint32_t sync_count;
    uint32_t ds3231_error_count;
} time_status_t;

/**
 * Initialize smart time management system
 * 
 * @param ds3231_sda DS3231 SDA pin (or -1 to disable)
 * @param ds3231_scl DS3231 SCL pin (or -1 to disable)
 * @param wifi_ssid WiFi SSID (or NULL to disable)
 * @param wifi_password WiFi password (or NULL to disable)
 * @param timezone Timezone string (or NULL for UTC)
 * @param sync_interval_hours Hours between NTP syncs (default 24)
 * @return ESP_OK on success
 */
esp_err_t smart_time_init(int ds3231_sda, int ds3231_scl, 
                         const char* wifi_ssid, const char* wifi_password, 
                         const char* timezone, int sync_interval_hours);

/**
 * Get current time from best available source
 * Automatically handles fallback between sources
 * 
 * @param timeinfo Output time structure
 * @return ESP_OK on success
 */
esp_err_t smart_time_get(struct tm *timeinfo);

/**
 * Check if time is valid and reliable
 * 
 * @return true if time is trustworthy
 */
bool smart_time_is_valid(void);

/**
 * Get time management status
 * 
 * @param status Output status structure
 */
void smart_time_get_status(time_status_t *status);

/**
 * Force time synchronization (WiFi + NTP)
 * Updates both DS3231 and internal RTC
 * 
 * @return ESP_OK on success
 */
esp_err_t smart_time_sync_now(void);

/**
 * Set time manually (emergency fallback)
 * Updates all available time sources
 * 
 * @param time_str Time string "YYYY-MM-DD HH:MM:SS"
 * @return ESP_OK on success
 */
esp_err_t smart_time_set_manual(const char* time_str);

/**
 * Periodic maintenance (call every few seconds)
 * Handles automatic sync, health checks, fallback
 */
void smart_time_maintenance(void);

/**
 * Get human-readable time source name
 */
const char* smart_time_source_name(time_source_t source);

#ifdef __cplusplus
}
#endif