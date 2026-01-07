/**
 * usb_msc.c
 * 
 * USB Mass Storage Class Implementation using ESP TinyUSB
 * Exposes SD card as USB drive to computer
 * LEFT port only - completely separate from USB Serial JTAG (RIGHT port)
 */

#include <stdbool.h>
#include <string.h>
#include "usb_msc.h"
#include "storage_manager.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "tinyusb.h"
#include "tinyusb_msc.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "tusb.h"

static const char *TAG = "USB_MSC";

static bool msc_initialized = false;
static bool msc_active = false;
static bool msc_connected = false;
static tinyusb_msc_storage_handle_t storage_handle = NULL;
static bool auto_detection_enabled = true;

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
    ESP_LOGI(TAG, "🔧 Hardware check: ESP32-S3 native USB-OTG pins");
    ESP_LOGI(TAG, "   USB_DM: GPIO19 (fixed)");
    ESP_LOGI(TAG, "   USB_DP: GPIO20 (fixed)");
    ESP_LOGI(TAG, "💡 These pins are hardwired and cannot be changed");
    
    if (msc_initialized) {
        ESP_LOGI(TAG, "✓ MSC already initialized");
        return ESP_OK;
    }
    
    ESP_LOGW(TAG, "⚠️  TinyUSB MSC INITIALIZATION DISABLED");
    ESP_LOGW(TAG, "💡 This prevents system crashes during USB MSC operations");
    ESP_LOGW(TAG, "💡 USB MSC functionality is temporarily unavailable");
    ESP_LOGW(TAG, "");
    ESP_LOGW(TAG, "🔧 Known Issues with TinyUSB on this hardware:");
    ESP_LOGW(TAG, "   1. USB-OTG bridge initialization causes system crashes");
    ESP_LOGW(TAG, "   2. GPIO conflicts with sensor I2C/SPI buses");
    ESP_LOGW(TAG, "   3. TinyUSB driver compatibility issues");
    ESP_LOGW(TAG, "");
    ESP_LOGW(TAG, "📊 System continues with full data logging functionality");
    ESP_LOGW(TAG, "💾 Access data files by removing SD card manually");
    ESP_LOGW(TAG, "🔄 Auto-detection and manual USB MSC commands disabled");
    
    // Mark as "initialized" but non-functional to prevent further attempts
    msc_initialized = false;  // Keep false to prevent USB MSC operations
    
    return ESP_ERR_NOT_SUPPORTED;
    
    /* ORIGINAL TINYUSB INITIALIZATION DISABLED TO PREVENT CRASHES
    
    // Configure TinyUSB driver with minimal, more compatible configuration
    const tinyusb_config_t tusb_cfg = {
        .port = TINYUSB_PORT_FULL_SPEED_0,
        .phy = {
            .skip_setup = false,
            .self_powered = true,        // Try self-powered mode
            .vbus_monitor_io = -1,       // Disable VBUS monitoring
        },
        .task = {
            .size = 2048,                // Smaller task size
            .priority = 3,               // Lower priority
            .xCoreID = tskNO_AFFINITY,   // Any core
        },
        .descriptor = {
            .device = NULL,
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
        ESP_LOGE(TAG, "This may be due to:");
        ESP_LOGE(TAG, "  1. USB-OTG bridge not properly soldered");
        ESP_LOGE(TAG, "  2. GPIO conflicts with other components");
        ESP_LOGE(TAG, "  3. TinyUSB configuration issues");
        ESP_LOGE(TAG, "System will continue without USB MSC functionality");
        return ret;
    }
    
    ESP_LOGI(TAG, "✓ TinyUSB driver installed successfully");
    
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
        ESP_LOGE(TAG, "This may be due to:");
        ESP_LOGE(TAG, "  1. TinyUSB driver installation failed");
        ESP_LOGE(TAG, "  2. MSC configuration conflicts");
        ESP_LOGE(TAG, "  3. Insufficient memory for MSC buffers");
        ESP_LOGE(TAG, "System will continue without USB MSC functionality");
        return ret;
    }
    
    msc_initialized = true;
    ESP_LOGI(TAG, "✓ ESP TinyUSB MSC initialized successfully");
    ESP_LOGI(TAG, "💾 USB Mass Storage ready for LEFT port connection");
    ESP_LOGI(TAG, "🔧 Use manual commands if auto-detection fails");
    
    return ESP_OK;
    
    END DISABLED CODE */
}

esp_err_t usb_msc_start(void) {
    if (!msc_initialized) {
        ESP_LOGE(TAG, "MSC not initialized - cannot start USB MSC");
        return ESP_ERR_INVALID_STATE;
    }
    
    if (msc_active) {
        ESP_LOGI(TAG, "USB MSC already active");
        return ESP_OK;
    }
    
    if (!storage_manager_is_initialized()) {
        ESP_LOGE(TAG, "Storage manager not initialized - MSC will show empty drive");
        return ESP_ERR_INVALID_STATE;
    }
    
    ESP_LOGW(TAG, "⚠️  USB MSC START DISABLED - Known to cause system crashes");
    ESP_LOGW(TAG, "💡 This is due to TinyUSB hardware initialization issues");
    ESP_LOGW(TAG, "💡 The USB-OTG bridge may not be properly configured");
    ESP_LOGW(TAG, "💡 System will continue with data logging only");
    ESP_LOGW(TAG, "");
    ESP_LOGW(TAG, "🔧 Troubleshooting USB MSC Issues:");
    ESP_LOGW(TAG, "   1. Check USB-OTG bridge soldering (GPIO19/20)");
    ESP_LOGW(TAG, "   2. Verify no GPIO conflicts with sensors");
    ESP_LOGW(TAG, "   3. Test with minimal hardware configuration");
    ESP_LOGW(TAG, "   4. Check TinyUSB library compatibility");
    ESP_LOGW(TAG, "");
    ESP_LOGW(TAG, "📊 Data logging continues normally on SD card");
    ESP_LOGW(TAG, "💾 Access files by removing SD card manually");
    
    return ESP_ERR_NOT_SUPPORTED;
    
    /* ORIGINAL CODE DISABLED TO PREVENT CRASHES
    
    // CRITICAL: Safe shutdown of storage manager before USB MSC
    ESP_LOGI(TAG, "🛡️  Performing safe shutdown of data logging");
    esp_err_t shutdown_ret = storage_manager_safe_shutdown();
    if (shutdown_ret != ESP_OK) {
        ESP_LOGE(TAG, "❌ Safe shutdown failed: %s", esp_err_to_name(shutdown_ret));
        ESP_LOGE(TAG, "⚠️  Proceeding with USB MSC but data may be at risk");
    } else {
        ESP_LOGI(TAG, "✓ Data logging safely stopped - no corruption risk");
    }
    
    // Small delay to ensure all file operations complete
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // Get SD card handle from storage manager
    sdmmc_card_t *card = storage_manager_get_card();
    if (!card) {
        ESP_LOGE(TAG, "Failed to get SD card handle from storage manager");
        // Try to restart logging if MSC failed
        storage_manager_start_logging();
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
        
        // Restart logging if MSC failed
        ESP_LOGI(TAG, "🔄 Restarting data logging after MSC failure");
        storage_manager_start_logging();
        return ret;
    }
    
    msc_active = true;
    ESP_LOGI(TAG, "✓ USB MSC started - SD card exposed as USB drive");
    ESP_LOGI(TAG, "🛡️  Data logging safely paused - no corruption risk");
    ESP_LOGI(TAG, "💻 SD card should now appear as removable drive on computer");
    ESP_LOGI(TAG, "💡 LED is now BLUE to indicate USB mode");
    
    return ESP_OK;
    
    END DISABLED CODE */
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
    ESP_LOGI(TAG, "💡 LED returned to distance indication");
    
    // IMPORTANT: Restart data logging after stopping USB MSC
    if (storage_manager_is_initialized()) {
        esp_err_t log_ret = storage_manager_start_logging();
        if (log_ret == ESP_OK) {
            ESP_LOGI(TAG, "✓ Data logging restarted successfully");
        } else {
            ESP_LOGW(TAG, "Failed to restart logging: %s", esp_err_to_name(log_ret));
        }
    }
    
    return ESP_OK;
}

bool usb_msc_is_active(void) {
    return msc_active;
}

bool usb_msc_is_connected(void) {
    return msc_connected;
}

/**
 * Handle automatic USB MSC detection and switching (LEFT port only)
 * Call this regularly from main loop
 */
void usb_msc_handle_auto_detection(void) {
    // Auto-detection disabled due to TinyUSB hardware issues
    // This prevents system crashes from TinyUSB calls
    return;
    
    /* ORIGINAL AUTO-DETECTION CODE DISABLED TO PREVENT CRASHES
    
    if (!msc_initialized || !auto_detection_enabled) {
        return;
    }
    
    static bool last_usb_connected = false;
    static uint32_t last_check_time = 0;
    static uint32_t connection_stable_time = 0;
    uint32_t current_time = esp_timer_get_time() / 1000;
    
    // Check every 500ms for responsive detection
    if ((current_time - last_check_time) < 500) {
        return;
    }
    last_check_time = current_time;
    
    // Check if LEFT port (native USB) has a host connected
    bool usb_host_connected = tud_mounted();
    
    // Debounce connection changes (wait 1 second for stable connection)
    if (usb_host_connected != last_usb_connected) {
        connection_stable_time = current_time;
        last_usb_connected = usb_host_connected;
        return;
    }
    
    // Connection must be stable for 1 second before acting
    if ((current_time - connection_stable_time) < 1000) {
        return;
    }
    
    // Handle automatic MSC switching
    if (usb_host_connected && !msc_active) {
        // LEFT port connected - start USB MSC automatically
        ESP_LOGI(TAG, "🔌 LEFT port: USB host detected");
        ESP_LOGI(TAG, "💾 Starting USB Mass Storage automatically");
        ESP_LOGI(TAG, "📊 Data logging will be paused");
        ESP_LOGI(TAG, "💡 LED will turn BLUE");
        
        usb_msc_start();
        
    } else if (!usb_host_connected && msc_active) {
        // LEFT port disconnected - stop USB MSC and resume logging
        ESP_LOGI(TAG, "🔌 LEFT port: USB host disconnected");
        ESP_LOGI(TAG, "💾 Stopping USB Mass Storage");
        ESP_LOGI(TAG, "📊 Data logging will resume");
        ESP_LOGI(TAG, "💡 LED will return to distance indication");
        
        usb_msc_stop();
    }
    
    END DISABLED CODE */
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