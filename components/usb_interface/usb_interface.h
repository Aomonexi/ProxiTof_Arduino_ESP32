/**
 * usb_interface.h
 * 
 * USB Interface for Configuration and Time Sync
 * Provides serial commands when connected to computer
 */

#pragma once

#include "esp_err.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize USB interface
 * Sets up USB CDC (serial) for computer communication
 * 
 * @return ESP_OK on success
 */
esp_err_t usb_interface_init(void);

/**
 * Process USB commands (call periodically)
 * Handles configuration and time sync commands from computer
 */
void usb_interface_process(void);

/**
 * Check if USB is connected to computer (not just power)
 * Detects actual data connection vs power-only
 * 
 * @return true if USB cable connected and computer detected
 */
bool usb_interface_is_connected(void);

/**
 * Check if USB MSC mode is currently active
 * Returns true if device is in flash drive mode
 * 
 * @return true if USB Mass Storage is active
 */
bool usb_interface_is_msc_active(void);

/**
 * Handle USB connection changes and auto-MSC switching
 * Call this regularly from main loop to detect connection changes
 * Automatically starts/stops USB MSC based on computer connection
 */
void usb_interface_handle_connection_changes(void);

/**
 * Sync time from computer via USB
 * Computer sends current time via serial command
 * 
 * @return ESP_OK on success
 */
esp_err_t usb_interface_sync_time(void);

#ifdef __cplusplus
}
#endif