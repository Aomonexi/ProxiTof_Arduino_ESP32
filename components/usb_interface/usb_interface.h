/**
 * usb_interface.h
 * 
 * USB Interface for Configuration and Time Sync
 * Provides serial commands when connected to computer
 */

#pragma once

#include "esp_err.h"

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
 * Check if USB is connected to computer
 * 
 * @return true if USB cable connected and computer detected
 */
bool usb_interface_is_connected(void);

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