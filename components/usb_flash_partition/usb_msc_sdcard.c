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
static bool suppress_callback_messages = false;  // Suppress messages during init

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
    
    // Suppress messages during initialization
    if (suppress_callback_messages) {
        return;
    }
    
    switch (event->id) {
        case TINYUSB_MSC_EVENT_MOUNT_START:
            if (event->mount_point == TINYUSB_MSC_STORAGE_MOUNT_USB) {
                ESP_LOGI(TAG, "💾 USB mode activated - SD card accessible via USB");
                msc_active = true;
                current_status.usb_msc_active = true;
                current_status.logging_active = false;
                switch_count++;
                current_status.switch_count = switch_count;
            } else {
                ESP_LOGI(TAG, "📊 APP mode activated - data logging resumed");
                msc_active = false;
                current_status.usb_msc_active = false;
                current_status.logging_active = true;
            }
            break;
            
        case TINYUSB_MSC_EVENT_MOUNT_COMPLETE:
            if (event->mount_point == TINYUSB_MSC_STORAGE_MOUNT_USB) {
                ESP_LOGI(TAG, "✓ USB Transfer mode ready");
                ESP_LOGI(TAG, "🔌 Connect LEFT port (USB-OTG) to computer");
            } else {
                ESP_LOGI(TAG, "✓ Logging mode ready");
            }
            break;
            
        case TINYUSB_MSC_EVENT_MOUNT_FAILED:
            ESP_LOGW(TAG, "❌ Mode switch failed");
            // Reset state on failure
            msc_active = false;
            current_status.usb_msc_active = false;
            current_status.logging_active = true;
            break;
            
        case TINYUSB_MSC_EVENT_FORMAT_REQUIRED:
            ESP_LOGW(TAG, "⚠️  SD card requires formatting");
            break;
            
        case TINYUSB_MSC_EVENT_FORMAT_FAILED:
            ESP_LOGE(TAG, "❌ SD card format failed");
            break;
            
        default:
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
                .xCoreID = 1,  // Run on core 1 to avoid interfering with main loop on core 0
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
        .mount_point = TINYUSB_MSC_STORAGE_MOUNT_APP,  // Start in APP mode (logging active, USB inactive)
    };
    
    // Suppress callback messages during initialization
    suppress_callback_messages = true;
    
    ret = tinyusb_msc_new_storage_sdmmc(&storage_config, &msc_storage_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create MSC storage: %s", esp_err_to_name(ret));
        suppress_callback_messages = false;
        return ret;
    }
    
    // Wait for initialization to complete
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // Re-enable callback messages for user-triggered switches
    suppress_callback_messages = false;
    
    msc_initialized = true;
    msc_active = false;  // Start in APP mode (USB inactive)
    current_status.usb_msc_active = false;
    current_status.logging_active = true;  // Logging active at startup
    
    ESP_LOGI(TAG, "✓ USB MSC initialized successfully");
    ESP_LOGI(TAG, "📊 Starting in APP mode - data logging active");
    ESP_LOGI(TAG, "💡 Navigate to USB Transfer mode (Mode 7) to activate USB access");
    
    return ESP_OK;
}

esp_err_t usb_msc_sdcard_pause_logging(void)
{
    if (!msc_initialized) {
        ESP_LOGW(TAG, "USB MSC not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    
    // Check if already in USB mode
    if (msc_active) {
        ESP_LOGW(TAG, "USB Transfer mode already active");
        return ESP_OK;
    }
    
    ESP_LOGI(TAG, "📤 Activating USB Transfer mode...");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "╔════════════════════════════════════════════════╗");
    ESP_LOGI(TAG, "║  DEDICATED USB MODE - Performance Optimized   ║");
    ESP_LOGI(TAG, "╠════════════════════════════════════════════════╣");
    ESP_LOGI(TAG, "║  Suspending non-essential tasks:               ║");
    ESP_LOGI(TAG, "║  ⏸️  Sensor reading (ToF, IMU)                 ║");
    ESP_LOGI(TAG, "║  ⏸️  GPS processing                            ║");
    ESP_LOGI(TAG, "║  ⏸️  Data processing                           ║");
    ESP_LOGI(TAG, "║  ⏸️  High-rate display updates                 ║");
    ESP_LOGI(TAG, "║                                                ║");
    ESP_LOGI(TAG, "║  Active tasks:                                 ║");
    ESP_LOGI(TAG, "║  ✓ USB transfer (maximum bandwidth)            ║");
    ESP_LOGI(TAG, "║  ✓ Display status (1Hz)                        ║");
    ESP_LOGI(TAG, "║  ✓ Button monitoring                           ║");
    ESP_LOGI(TAG, "╚════════════════════════════════════════════════╝");
    ESP_LOGI(TAG, "");
    
    // CRITICAL: Stop logging BEFORE switching to USB mode
    // This prevents SD card access conflicts during the switch
    if (storage_manager_is_logging()) {
        ESP_LOGI(TAG, "⏸️  Pausing data logging...");
        storage_manager_stop_logging();
        vTaskDelay(pdMS_TO_TICKS(100));  // Give time for final flush
    }
    
    // Pause GPS processing to free CPU resources
    extern void gps_pause(void);
    gps_pause();
    
    // Switch to USB mode (unmount from app, mount to USB)
    ESP_LOGI(TAG, "🔄 Switching SD card to USB mode...");
    esp_err_t ret = tinyusb_msc_set_storage_mount_point(msc_storage_handle, 
                                                        TINYUSB_MSC_STORAGE_MOUNT_USB);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to switch to USB mode: %s", esp_err_to_name(ret));
        // Try to restart logging and GPS if switch failed
        storage_manager_start_logging();
        extern void gps_resume(void);
        gps_resume();
        return ret;
    }
    
    // Manually update state (callback might not fire immediately)
    msc_active = true;
    current_status.usb_msc_active = true;
    current_status.logging_active = false;
    
    ESP_LOGI(TAG, "✓ USB Transfer mode activated");
    ESP_LOGI(TAG, "🔌 Connect LEFT port (USB-OTG) to computer");
    ESP_LOGI(TAG, "💡 Press ACTION again to return to logging mode");
    ESP_LOGI(TAG, "");
    
    return ESP_OK;
}

esp_err_t usb_msc_sdcard_resume_logging(void)
{
    if (!msc_initialized) {
        ESP_LOGW(TAG, "USB MSC not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    
    // Check if already in APP mode
    if (!msc_active) {
        ESP_LOGW(TAG, "Already in logging mode");
        return ESP_OK;
    }
    
    ESP_LOGI(TAG, "📥 Deactivating USB Transfer mode...");
    ESP_LOGI(TAG, "💡 User workflow: Eject on PC → Disconnect cable → Press ACTION");
    ESP_LOGI(TAG, "");
    
    // Switch to APP mode (unmount from USB, mount to app)
    // No need to check USB connection - user is responsible for ejecting first
    ESP_LOGI(TAG, "🔄 Switching SD card to APP mode...");
    esp_err_t ret = tinyusb_msc_set_storage_mount_point(msc_storage_handle, 
                                                        TINYUSB_MSC_STORAGE_MOUNT_APP);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to switch to APP mode: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Manually update state (callback might not fire immediately)
    msc_active = false;
    current_status.usb_msc_active = false;
    current_status.logging_active = true;
    
    // Wait for mount to complete and SD card to settle
    vTaskDelay(pdMS_TO_TICKS(500));
    
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "╔════════════════════════════════════════════════╗");
    ESP_LOGI(TAG, "║  RESUMING NORMAL OPERATION                     ║");
    ESP_LOGI(TAG, "╠════════════════════════════════════════════════╣");
    ESP_LOGI(TAG, "║  Resuming all tasks:                           ║");
    ESP_LOGI(TAG, "║  ▶️  Sensor reading (ToF 60Hz, IMU 100Hz)      ║");
    ESP_LOGI(TAG, "║  ▶️  GPS processing                            ║");
    ESP_LOGI(TAG, "║  ▶️  Data processing & near-miss detection     ║");
    ESP_LOGI(TAG, "║  ▶️  Display updates (high refresh rate)       ║");
    ESP_LOGI(TAG, "║  ▶️  Data logging to SD card                   ║");
    ESP_LOGI(TAG, "╚════════════════════════════════════════════════╝");
    ESP_LOGI(TAG, "");
    
    // Resume GPS processing
    extern void gps_resume(void);
    gps_resume();
    
    // Restart logging
    ESP_LOGI(TAG, "▶️  Resuming data logging...");
    ret = storage_manager_start_logging();
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to restart logging: %s", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "✓ Logging mode activated");
    }
    
    ESP_LOGI(TAG, "✓ All systems resumed - back to normal operation");
    ESP_LOGI(TAG, "");
    
    return ESP_OK;
}

bool usb_msc_sdcard_is_host_connected(void)
{
    return tud_mounted();
}

bool usb_msc_sdcard_is_ready(void)
{
    // Device is ready when mounted and not suspended
    return tud_mounted() && !tud_suspended();
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