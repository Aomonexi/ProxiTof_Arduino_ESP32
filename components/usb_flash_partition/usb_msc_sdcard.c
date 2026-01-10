/**
 * usb_msc_sdcard.c
 * 
 * USB MSC SD Card Implementation using ESP-IDF TinyUSB
 * Direct SD card access via USB Mass Storage Class
 */

#include "usb_flash_partition.h"
#include "storage_manager.h"
#include "esp_log.h"
#include "esp_err.h"
#include "tinyusb.h"
#include "tinyusb_msc.h"
#include "sdmmc_cmd.h"

static const char *TAG = "USB_MSC_SD";

// USB MSC state
static tinyusb_msc_storage_handle_t msc_storage_handle = NULL;
static bool msc_initialized = false;
static bool msc_active = false;
static uint32_t switch_count = 0;

// Status tracking
static usb_sd_status_t current_status = {
    .usb_msc_active = false,
    .usb_host_connected = false,
    .sd_card_available = false,
    .logging_active = true,
    .switch_count = 0
};

/**
 * MSC event callback - handles mount/unmount events
 */
static void msc_event_callback(tinyusb_msc_storage_handle_t handle, tinyusb_msc_event_t *event, void *arg)
{
    (void)handle;
    (void)arg;
    
    switch (event->id) {
        case TINYUSB_MSC_EVENT_MOUNT_START:
            ESP_LOGI(TAG, "🔄 MSC mount operation starting...");
            if (event->mount_point == TINYUSB_MSC_STORAGE_MOUNT_USB) {
                ESP_LOGI(TAG, "📤 Switching to USB mode - pausing data logging");
                msc_active = true;
                current_status.usb_msc_active = true;
                current_status.logging_active = false;
                switch_count++;
                current_status.switch_count = switch_count;
            } else {
                ESP_LOGI(TAG, "📥 Switching to APP mode - resuming data logging");
                msc_active = false;
                current_status.usb_msc_active = false;
                current_status.logging_active = true;
            }
            break;
            
        case TINYUSB_MSC_EVENT_MOUNT_COMPLETE:
            ESP_LOGI(TAG, "✓ MSC mount operation completed");
            if (event->mount_point == TINYUSB_MSC_STORAGE_MOUNT_USB) {
                ESP_LOGI(TAG, "💾 SD card now accessible via USB");
                ESP_LOGI(TAG, "🔌 Connect LEFT port to computer to access files");
            } else {
                ESP_LOGI(TAG, "📊 Data logging resumed");
            }
            break;
            
        case TINYUSB_MSC_EVENT_MOUNT_FAILED:
            ESP_LOGW(TAG, "❌ MSC mount operation failed");
            // Reset state on failure
            msc_active = false;
            current_status.usb_msc_active = false;
            current_status.logging_active = true;
            break;
            
        case TINYUSB_MSC_EVENT_FORMAT_REQUIRED:
            ESP_LOGW(TAG, "⚠️  SD card requires formatting");
            ESP_LOGW(TAG, "💡 Use tinyusb_msc_format_storage() to format");
            break;
            
        case TINYUSB_MSC_EVENT_FORMAT_FAILED:
            ESP_LOGE(TAG, "❌ SD card format failed");
            break;
            
        default:
            ESP_LOGW(TAG, "Unknown MSC event: %d", event->id);
            break;
    }
}

esp_err_t usb_msc_sdcard_init(void)
{
    if (msc_initialized) {
        ESP_LOGW(TAG, "USB MSC already initialized");
        return ESP_OK;
    }
    
    ESP_LOGI(TAG, "Initializing USB MSC with SD card...");
    
    // Check if storage manager is initialized and get SD card handle
    if (!storage_manager_is_initialized()) {
        ESP_LOGE(TAG, "Storage manager not initialized - SD card not available");
        current_status.sd_card_available = false;
        return ESP_ERR_INVALID_STATE;
    }
    
    // Get SD card handle from storage manager
    sdmmc_card_t *card = storage_manager_get_card();
    if (card == NULL) {
        ESP_LOGE(TAG, "SD card not available from storage manager");
        current_status.sd_card_available = false;
        return ESP_ERR_INVALID_STATE;
    }
    
    current_status.sd_card_available = true;
    ESP_LOGI(TAG, "✓ SD card available: %s", card->cid.name);
    
    // Initialize TinyUSB if not already done
    if (!tusb_inited()) {
        ESP_LOGI(TAG, "Initializing TinyUSB...");
        
        tinyusb_config_t tusb_cfg = {
            .port = TINYUSB_PORT_FULL_SPEED_0,
            .phy = {
                .skip_setup = false,
                .self_powered = false,
                .vbus_monitor_io = -1,
            },
            .task = {
                .size = 4096,
                .priority = 5,
                .xCoreID = tskNO_AFFINITY,
            },
            .descriptor = {0}, // Use default descriptors
            .event_cb = NULL,
            .event_arg = NULL,
        };
        
        esp_err_t ret = tinyusb_driver_install(&tusb_cfg);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to install TinyUSB driver: %s", esp_err_to_name(ret));
            return ret;
        }
        ESP_LOGI(TAG, "✓ TinyUSB driver installed");
    }
    
    // Configure MSC driver
    tinyusb_msc_driver_config_t msc_driver_config = {
        .callback = msc_event_callback,
        .callback_arg = NULL,
        .user_flags = {
            .auto_mount_off = false,  // Allow automatic mounting
        }
    };
    
    esp_err_t ret = tinyusb_msc_install_driver(&msc_driver_config);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "Failed to install MSC driver: %s", esp_err_to_name(ret));
        return ret;
    }
    
    if (ret == ESP_ERR_INVALID_STATE) {
        ESP_LOGI(TAG, "MSC driver already installed");
    } else {
        ESP_LOGI(TAG, "✓ MSC driver installed");
    }
    
    // Configure MSC storage
    tinyusb_msc_storage_config_t storage_config = {
        .medium = {
            .card = card,  // Use SD card from storage manager
        },
        .fat_fs = {
            .base_path = "/usb",  // Different from SD card mount point
            .config = {
                .max_files = 5,
                .allocation_unit_size = 16 * 1024,
                .format_if_mount_failed = false,  // Don't auto-format
            },
            .do_not_format = true,  // Don't format automatically
            .format_flags = FM_ANY,
        },
        .mount_point = TINYUSB_MSC_STORAGE_MOUNT_USB,  // Start in USB mode
    };
    
    ret = tinyusb_msc_new_storage_sdmmc(&storage_config, &msc_storage_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create MSC storage: %s", esp_err_to_name(ret));
        return ret;
    }
    
    msc_initialized = true;
    msc_active = true;  // Start in USB mode
    current_status.usb_msc_active = true;
    current_status.logging_active = false;  // Logging paused in USB mode
    
    ESP_LOGI(TAG, "✓ USB MSC initialized successfully");
    ESP_LOGI(TAG, "💾 SD card accessible via USB (LEFT port)");
    ESP_LOGI(TAG, "⚠️  Data logging paused while in USB mode");
    
    return ESP_OK;
}

esp_err_t usb_msc_sdcard_pause_logging(void)
{
    if (!msc_initialized) {
        ESP_LOGW(TAG, "USB MSC not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    
    ESP_LOGI(TAG, "📤 Switching to USB mode...");
    
    // Switch to USB mode (unmount from app, mount to USB)
    esp_err_t ret = tinyusb_msc_set_storage_mount_point(msc_storage_handle, 
                                                        TINYUSB_MSC_STORAGE_MOUNT_USB);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to switch to USB mode: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // The callback will handle state updates
    return ESP_OK;
}

esp_err_t usb_msc_sdcard_resume_logging(void)
{
    if (!msc_initialized) {
        ESP_LOGW(TAG, "USB MSC not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    
    ESP_LOGI(TAG, "📥 Switching to APP mode...");
    
    // Switch to APP mode (unmount from USB, mount to app)
    esp_err_t ret = tinyusb_msc_set_storage_mount_point(msc_storage_handle, 
                                                        TINYUSB_MSC_STORAGE_MOUNT_APP);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to switch to APP mode: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // The callback will handle state updates
    return ESP_OK;
}

bool usb_msc_sdcard_is_active(void)
{
    return msc_active;
}

bool usb_msc_sdcard_is_initialized(void)
{
    return msc_initialized;
}

void usb_msc_sdcard_get_status(usb_sd_status_t *status)
{
    if (status) {
        // Update USB host connection status
        current_status.usb_host_connected = tud_mounted();
        
        // Copy current status
        *status = current_status;
    }
}

void usb_msc_sdcard_process(void)
{
    if (!msc_initialized) {
        return;
    }
    
    // TinyUSB handles processing automatically
    // We just need to track connection status
    static bool last_connected = false;
    bool currently_connected = tud_mounted();
    
    if (currently_connected != last_connected) {
        if (currently_connected) {
            ESP_LOGI(TAG, "🔌 USB host connected");
        } else {
            ESP_LOGI(TAG, "🔌 USB host disconnected");
        }
        last_connected = currently_connected;
        current_status.usb_host_connected = currently_connected;
    }
}

esp_err_t usb_msc_sdcard_deinit(void)
{
    if (!msc_initialized) {
        return ESP_OK;
    }
    
    ESP_LOGI(TAG, "Deinitializing USB MSC...");
    
    // Delete MSC storage
    if (msc_storage_handle) {
        esp_err_t ret = tinyusb_msc_delete_storage(msc_storage_handle);
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "Failed to delete MSC storage: %s", esp_err_to_name(ret));
        }
        msc_storage_handle = NULL;
    }
    
    // Uninstall MSC driver
    esp_err_t ret = tinyusb_msc_uninstall_driver();
    if (ret != ESP_OK && ret != ESP_ERR_NOT_SUPPORTED) {
        ESP_LOGW(TAG, "Failed to uninstall MSC driver: %s", esp_err_to_name(ret));
    }
    
    msc_initialized = false;
    msc_active = false;
    
    // Reset status
    current_status.usb_msc_active = false;
    current_status.usb_host_connected = false;
    current_status.logging_active = true;
    
    ESP_LOGI(TAG, "✓ USB MSC deinitialized");
    return ESP_OK;
}