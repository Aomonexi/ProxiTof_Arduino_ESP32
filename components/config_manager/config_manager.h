/**
 * config_manager.h
 * 
 * SD Card Configuration Manager
 * Allows field configuration without recompiling
 */

#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CONFIG_MAX_STRING_LEN 64

typedef struct {
    char wifi_ssid[CONFIG_MAX_STRING_LEN];
    char wifi_password[CONFIG_MAX_STRING_LEN];
    char timezone[CONFIG_MAX_STRING_LEN];
    int ds3231_sda_pin;
    int ds3231_scl_pin;
    int sync_interval_hours;
    bool wifi_enabled;
    bool ds3231_enabled;
} system_config_t;

/**
 * Load configuration from SD card
 * Looks for /sdcard/config.txt
 * 
 * @param config Output configuration structure
 * @return ESP_OK on success
 */
esp_err_t config_load_from_sd(system_config_t *config);

/**
 * Save configuration to SD card
 * Creates /sdcard/config.txt
 * 
 * @param config Configuration to save
 * @return ESP_OK on success
 */
esp_err_t config_save_to_sd(const system_config_t *config);

/**
 * Load default configuration
 * 
 * @param config Output configuration structure
 */
void config_load_defaults(system_config_t *config);

/**
 * Print configuration (for debugging)
 * 
 * @param config Configuration to print
 */
void config_print(const system_config_t *config);

/**
 * Check if configuration file exists on SD card
 * 
 * @return true if config file exists
 */
bool config_file_exists(void);

#ifdef __cplusplus
}
#endif