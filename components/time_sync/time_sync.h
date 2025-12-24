/**
 * time_sync.h
 * 
 * WiFi + NTP Time Synchronization for ESP32
 * Syncs time once at startup, then uses internal RTC
 */

#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize WiFi and sync time with NTP server
 * 
 * @param wifi_ssid WiFi network name
 * @param wifi_password WiFi password
 * @param timezone Timezone string (e.g., "CET-1CEST,M3.5.0,M10.5.0/3" for Europe)
 * @return ESP_OK on success
 */
esp_err_t time_sync_init(const char* wifi_ssid, const char* wifi_password, const char* timezone);

/**
 * Check if time is synchronized
 * @return true if time is valid (after year 2020)
 */
bool time_sync_is_valid(void);

/**
 * Get current time as string
 * @param buffer Output buffer
 * @param size Buffer size
 * @return ESP_OK on success
 */
esp_err_t time_sync_get_string(char* buffer, size_t size);

/**
 * Disconnect WiFi after time sync (to save power)
 */
void time_sync_disconnect_wifi(void);

#ifdef __cplusplus
}
#endif