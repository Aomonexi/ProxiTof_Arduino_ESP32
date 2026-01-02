/**
 * time_sync.h
 * 
 * ESP-IDF System Time Management
 * Using official ESP-IDF SNTP and system time APIs
 */

#pragma once

#include "esp_err.h"
#include <stdbool.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize WiFi and sync time with NTP server using ESP-IDF official APIs
 * 
 * @param wifi_ssid WiFi network name
 * @param wifi_password WiFi password
 * @param timezone Timezone string (e.g., "CST-8" for China, "EST5EDT,M3.2.0,M11.1.0" for US Eastern)
 * @return ESP_OK on success
 */
esp_err_t time_sync_init(const char* wifi_ssid, const char* wifi_password, const char* timezone);

/**
 * Disconnect WiFi after time sync (to save power)
 * @return ESP_OK on success
 */
esp_err_t time_sync_disconnect_wifi(void);

/**
 * Check if time is synchronized
 * @return true if time has been synchronized
 */
bool time_sync_is_synced(void);

/**
 * Get current system time
 * @param timeinfo Pointer to tm structure to fill
 * @return ESP_OK on success, ESP_ERR_INVALID_STATE if time not valid
 */
esp_err_t time_sync_get_current_time(struct tm *timeinfo);

/**
 * Set system time manually
 * @param time_str Time string in format "YYYY-MM-DD HH:MM:SS"
 * @return ESP_OK on success
 */
esp_err_t time_sync_set_manual_time(const char* time_str);

#ifdef __cplusplus
}
#endif