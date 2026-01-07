/**
 * usb_msc.h
 * 
 * USB Mass Storage Class Implementation
 * Exposes SD card as USB drive for easy configuration
 */

#pragma once

#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize USB Mass Storage Device
 * Exposes SD card as USB drive when connected to computer
 * 
 * @return ESP_OK on success
 */
esp_err_t usb_msc_init(void);

/**
 * Start USB MSC mode
 * SD card becomes accessible from computer
 * Note: Stops data logging while active
 * 
 * @return ESP_OK on success
 */
esp_err_t usb_msc_start(void);

/**
 * Stop USB MSC mode
 * Returns SD card control to ESP32
 * Resumes data logging
 * 
 * @return ESP_OK on success
 */
esp_err_t usb_msc_stop(void);

/**
 * Check if USB MSC is active
 * 
 * @return true if USB MSC mode is active
 */
bool usb_msc_is_active(void);

/**
 * Check if USB cable is connected
 * 
 * @return true if USB cable detected
 */
bool usb_msc_is_connected(void);

/**
 * Handle automatic USB MSC detection and switching (LEFT port only)
 * Call this regularly from main loop
 */
void usb_msc_handle_auto_detection(void);

/**
 * Deinitialize USB MSC
 * Cleans up TinyUSB resources
 * 
 * @return ESP_OK on success
 */
esp_err_t usb_msc_deinit(void);

#ifdef __cplusplus
}
#endif