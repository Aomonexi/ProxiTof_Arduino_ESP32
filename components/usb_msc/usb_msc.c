/**
 * usb_msc.c
 * 
 * USB Mass Storage Class Implementation using ESP TinyUSB
 * Exposes SD card as USB drive to computer
 * Based on ESP-IDF esp_tinyusb component
 */

#include <stdbool.h>
#include <string.h>
#include "usb_msc.h"
#include "storage_manager.h"
#include "esp_log.h"
#include "tinyusb.h"
#include "tinyusb_msc.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

static const char *TAG = "USB_MSC";

static bool msc_initialized = false;
static bool msc_active = false;
static bool msc_connected = false;
static tinyusb_msc_storage_handle_t storage_handle = NULL;

// Event callback for MSC mount/unmount events
static void msc_event_callback(tinyusb_msc_storage_handle_t handle, tinyusb_msc_event_t *event, void *arg) {
    switch (event->id) {
        case TINYUSB_MSC_EVENT_MOUNT_START:
            ESP_LOGI(TAG, "MSC mount operation starting...");
            if (event->mount_point == TINYUSB_MSC_STORAGE_MOUNT_USB) {
                msc_connected = true;
                ESP_LOGI(TAG, "💻 SD card exposed to USB host");
            } else {
                msc_connected = false;
                ESP_LOGI(TAG, "📱 SD card returned to application");
            }
            break;
            
        case TINYUSB_MSC_EVENT_MOUNT_COMPLETE:
            ESP_LOGI(TAG, "MSC mount operation completed");
            break;
            
        case TINYUSB_MSC_EVENT_MOUNT_FAILED:
            ESP_LOGW(TAG, "MSC mount operation failed");
            break;
            
        case TINYUSB_MSC_EVENT_FORMAT_REQUIRED:
            ESP_LOGW(TAG, "Storage requires formatting");
            break;
            
        case TINYUSB_MSC_EVENT_FORMAT_FAILED:
            ESP_LOGE(TAG, "Storage format failed");
            break;
            
        default:
            ESP_LOGW(TAG, "Unknown MSC event: %d", event->id);
            break;
    }
}

esp_err_t usb_msc_init(void) {
    ESP_LOGI(TAG, "Initializing ESP TinyUSB MSC...");
    
    if (msc_initialized) {
        return ESP_OK;
    }
    
    // Configure TinyUSB driver with minimal configuration
    const tinyusb_config_t tusb_cfg = {
        .port = TINYUSB_PORT_FULL_SPEED_0,
        .phy = {
            .skip_setup = false,
            .self_powered = false,
            .vbus_monitor_io = -1,
        },
        .task = {
            .size = 4096,
            .priority = 5,
            .xCoreID = 1,  // Use core 1 for TinyUSB task (core 0 is used by main app)
        },
        .descriptor = {
            .device = NULL,        // Use default device descriptor
            .qualifier = NULL,
            .string = NULL,
            .string_count = 0,
            .full_speed_config = NULL,
            .high_speed_config = NULL,
        },
        .event_cb = NULL,
        .event_arg = NULL,
    };
    
    // Install TinyUSB driver
    esp_err_t ret = tinyusb_driver_install(&tusb_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to install TinyUSB driver: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "✓ TinyUSB driver installed");
    
    // Configure MSC driver
    const tinyusb_msc_driver_config_t msc_driver_cfg = {
        .user_flags = {
            .auto_mount_off = 0,  // Allow automatic mounting
        },
        .callback = msc_event_callback,
        .callback_arg = NULL,
    };
    
    // Install MSC driver
    ret = tinyusb_msc_install_driver(&msc_driver_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to install MSC driver: %s", esp_err_to_name(ret));
        return ret;
    }
    
    msc_initialized = true;
    ESP_LOGI(TAG, "✓ ESP TinyUSB MSC initialized");
    ESP_LOGI(TAG, "💾 Connect USB cable to access SD card as drive");
    
    return ESP_OK;
}

esp_err_t usb_msc_start(void) {
    if (!msc_initialized) {
        ESP_LOGE(TAG, "MSC not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    
    if (!storage_manager_is_initialized()) {
        ESP_LOGE(TAG, "Storage manager not initialized - MSC will show empty drive");
        return ESP_ERR_INVALID_STATE;
    }
    
    // Get SD card handle from storage manager
    sdmmc_card_t *card = storage_manager_get_card();
    if (!card) {
        ESP_LOGE(TAG, "Failed to get SD card handle from storage manager");
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "SD card info: Name=%s, Speed=%d kHz", 
             card->cid.name, 
             card->real_freq_khz);
    
    // Configure MSC storage for SD/MMC
    const tinyusb_msc_storage_config_t storage_cfg = {
        .medium = {
            .card = card,
        },
        .fat_fs = {
            .base_path = "/sdcard",
            .config = {
                .format_if_mount_failed = false,  // Don't format if mount fails
                .max_files = 5,
                .allocation_unit_size = 16 * 1024,
                .disk_status_check_enable = false,
            },
            .do_not_format = true,  // Never format the card
            .format_flags = 0,  // Use default (FM_ANY)
        },
        .mount_point = TINYUSB_MSC_STORAGE_MOUNT_USB,  // Start in USB mode
    };
    
    ESP_LOGI(TAG, "Creating MSC storage with base_path: %s", storage_cfg.fat_fs.base_path);
    
    // Create MSC storage
    esp_err_t ret = tinyusb_msc_new_storage_sdmmc(&storage_cfg, &storage_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create MSC storage: %s", esp_err_to_name(ret));
        ESP_LOGE(TAG, "This may be due to SD card filesystem issues");
        ESP_LOGE(TAG, "Try formatting SD card as FAT32 on computer first");
        return ret;
    }
    
    msc_active = true;
    ESP_LOGI(TAG, "✓ USB MSC started - SD card exposed as USB drive");
    ESP_LOGI(TAG, "⚠️  Data logging paused while USB MSC active");
    ESP_LOGI(TAG, "💻 SD card should now appear as removable drive on computer");
    
    return ESP_OK;
}

esp_err_t usb_msc_stop(void) {
    if (!msc_active || !storage_handle) {
        return ESP_OK;
    }
    
    // Switch storage back to application
    esp_err_t ret = tinyusb_msc_set_storage_mount_point(storage_handle, TINYUSB_MSC_STORAGE_MOUNT_APP);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to switch storage to app: %s", esp_err_to_name(ret));
    }
    
    // Delete MSC storage
    ret = tinyusb_msc_delete_storage(storage_handle);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to delete MSC storage: %s", esp_err_to_name(ret));
    }
    
    storage_handle = NULL;
    msc_active = false;
    msc_connected = false;
    
    ESP_LOGI(TAG, "✓ USB MSC stopped - SD card returned to application");
    ESP_LOGI(TAG, "📊 Data logging resumed");
    
    return ESP_OK;
}

bool usb_msc_is_active(void) {
    return msc_active;
}

bool usb_msc_is_connected(void) {
    return msc_connected;
}

esp_err_t usb_msc_deinit(void) {
    if (msc_initialized) {
        usb_msc_stop();
        
        // Uninstall MSC driver
        esp_err_t ret = tinyusb_msc_uninstall_driver();
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "Failed to uninstall MSC driver: %s", esp_err_to_name(ret));
        }
        
        // Uninstall TinyUSB driver
        ret = tinyusb_driver_uninstall();
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "Failed to uninstall TinyUSB driver: %s", esp_err_to_name(ret));
        }
        
        msc_initialized = false;
        ESP_LOGI(TAG, "USB MSC deinitialized");
    }
    return ESP_OK;
}