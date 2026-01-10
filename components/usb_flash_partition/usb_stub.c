/**
 * usb_stub.c
 * 
 * Temporary USB stub implementation
 * Provides interface compatibility while USB MSC is being developed
 */

#include "usb_flash_partition.h"
#include "esp_log.h"

static const char *TAG = "USB_STUB";

// Stub implementation - always returns not initialized
esp_err_t usb_msc_sdcard_init(void) {
    ESP_LOGW(TAG, "USB MSC stub - not implemented");
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t usb_msc_sdcard_pause_logging(void) {
    return ESP_OK;
}

esp_err_t usb_msc_sdcard_resume_logging(void) {
    return ESP_OK;
}

bool usb_msc_sdcard_is_active(void) {
    return false;
}

bool usb_msc_sdcard_is_initialized(void) {
    return false;
}

void usb_msc_sdcard_get_status(usb_sd_status_t *status) {
    if (status) {
        status->usb_msc_active = false;
        status->usb_host_connected = false;
        status->sd_card_available = true;
        status->logging_active = true;
        status->switch_count = 0;
    }
}

void usb_msc_sdcard_process(void) {
    // Do nothing
}

esp_err_t usb_msc_sdcard_deinit(void) {
    return ESP_OK;
}