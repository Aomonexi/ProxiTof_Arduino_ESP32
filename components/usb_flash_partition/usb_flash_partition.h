/**
 * usb_flash_partition.h
 * 
 * USB MSC SD Card Implementation
 * Direct sector access to SD card via USB Mass Storage Class
 */

#pragma once

#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize USB MSC with SD card
 * Sets up TinyUSB MSC for direct SD card sector access
 */
esp_err_t usb_msc_sdcard_init(void);

/**
 * Pause data logging safely before USB access
 */
esp_err_t usb_msc_sdcard_pause_logging(void);

/**
 * Resume data logging after USB access
 */
esp_err_t usb_msc_sdcard_resume_logging(void);

/**
 * Check if USB MSC is currently active (computer accessing SD card)
 */
bool usb_msc_sdcard_is_active(void);

/**
 * Check if USB MSC is initialized
 */
bool usb_msc_sdcard_is_initialized(void);

/**
 * Check if USB host (PC) is currently connected
 */
bool usb_msc_sdcard_is_host_connected(void);

/**
 * Check if USB device is ready for data transfer (mounted and not suspended)
 */
bool usb_msc_sdcard_is_ready(void);

/**
 * Get current status
 */
typedef struct {
    bool usb_msc_active;
    bool usb_host_connected;
    bool sd_card_available;
    bool logging_active;
    uint32_t switch_count;
} usb_sd_status_t;

void usb_msc_sdcard_get_status(usb_sd_status_t *status);

/**
 * Process USB MSC events and handle logging pause/resume
 */
void usb_msc_sdcard_process(void);

/**
 * Deinitialize USB MSC
 */
esp_err_t usb_msc_sdcard_deinit(void);

#ifdef __cplusplus
}
#endif