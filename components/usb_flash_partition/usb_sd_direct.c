/**
 * usb_sd_direct.c
 * 
 * USB SD Card Direct Access Implementation
 * Safe switching between app logging and USB MSC access
 */

#include "usb_flash_partition.h"
#include "storage_manager.h"
#include "esp_log.h"
#include "tinyusb_msc.h"
#include "esp_timer.h"
#include "sdmmc_cmd.h"
#include <string.h>

static const char *TAG = "USB_SD_DIRECT";

// SD card direct access state
static bool usb_sd_initialized = false;
static bool usb_msc_active = false;
static bool usb_host_connected = false;
static tinyusb_msc_storage_handle_t msc_handle = NULL;
static sdmmc_card_t *sd_card = NULL;

// Statistics
static usb_sd_status_t status = {0};

/**
 * TinyUSB MSC event callback for SD card switching
 */
static void sd_msc_event_callback(tinyusb_msc_storage_handle_t handle, tinyusb_msc_event_t *event, void *arg) {
    (void)arg;
    
    switch (event->id) {
        case TINYUSB_MSC_EVENT_MOUNT_START:
            if (event->mount_point == TINYUSB_MSC_STORAGE_MOUNT_USB) {
                ESP_LOGI(TAG, "🔌 USB host requesting SD card access");
                ESP_LOGI(TAG, "⚠️  Data logging will be paused");
                usb_msc_active = true;
                status.usb_msc_active = true;
                status.logging_active = false;
            } else {
                ESP_LOGI(TAG, "📱 Application regaining SD card access");
                ESP_LOGI(TAG, "✓ Data logging will resume");
                usb_msc_active = false;
                status.usb_msc_active = false;
                status.logging_active = true;
            }
            break;
            
        case TINYUSB_MSC_EVENT_MOUNT_COMPLETE:
            if (event->mount_point == TINYUSB_MSC_STORAGE_MOUNT_USB) {
                ESP_LOGI(TAG, "✓ USB MSC active - SD card exposed as USB drive");
                ESP_LOGI(TAG, "💻 Computer can now access all SD card files");
                ESP_LOGI(TAG, "💡 LED should turn BLUE (USB mode)");
                usb_host_connected = true;
                status.usb_host_connected = true;
            } else {
                ESP_LOGI(TAG, "✓ Application access restored - USB MSC inactive");
                ESP_LOGI(TAG, "📊 Data logging resumed");
                ESP_LOGI(TAG, "💡 LED returns to distance indication");
                usb_host_connected = false;
                status.usb_host_connected = false;
            }
            break;
            
        case TINYUSB_MSC_EVENT_MOUNT_FAILED:
            ESP_LOGE(TAG, "❌ MSC mount failed");
            usb_msc_active = false;
            usb_host_connected = false;
            status.usb_msc_active = false;
            status.usb_host_connected = false;
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
/**
 * Initialize USB SD card direct access system
 */
esp_err_t usb_sd_direct_init(void) {
    if (usb_sd_initialized) {
        ESP_LOGI(TAG, "✓ USB SD direct access already initialized");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Initializing USB SD card direct access...");
    ESP_LOGI(TAG, "🔄 Safe switching between logging and USB access");
    ESP_LOGI(TAG, "💾 Direct access to complete SD card via USB");

    // Get SD card handle from storage manager
    sd_card = storage_manager_get_card();
    if (!sd_card) {
        ESP_LOGE(TAG, "❌ No SD card available from storage manager");
        ESP_LOGE(TAG, "💡 Initialize storage manager first");
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "✓ SD card found: %s", sd_card->cid.name);
    ESP_LOGI(TAG, "  Capacity: %lu sectors (%lu MB)", 
             sd_card->csd.capacity, 
             (sd_card->csd.capacity * sd_card->csd.sector_size) / (1024*1024));

    // Initialize TinyUSB MSC driver
    ESP_LOGI(TAG, "Initializing TinyUSB MSC driver for SD card...");
    
    tinyusb_msc_driver_config_t driver_config = {
        .user_flags.auto_mount_off = 0,  // Enable automatic mounting
        .callback = sd_msc_event_callback,
        .callback_arg = NULL
    };
    
    esp_err_t ret = tinyusb_msc_install_driver(&driver_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "❌ Failed to install TinyUSB MSC driver: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "✓ TinyUSB MSC driver installed");
    
    // Create TinyUSB MSC storage for SD card
    tinyusb_msc_storage_config_t storage_config = {
        .medium.card = sd_card,  // Use SD card directly
        .fat_fs = {
            .base_path = "/sdcard",  // Use existing SD card mount point
            .config = {
                .max_files = 10,
                .format_if_mount_failed = false,  // Don't format SD card!
                .allocation_unit_size = 0  // Use default
            },
            .do_not_format = true,  // NEVER format the SD card
            .format_flags = 0
        },
        .mount_point = TINYUSB_MSC_STORAGE_MOUNT_APP  // Start with app access
    };
    
    ret = tinyusb_msc_new_storage_sdmmc(&storage_config, &msc_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "❌ Failed to create TinyUSB MSC storage: %s", esp_err_to_name(ret));
        ESP_LOGE(TAG, "💡 This is likely due to SD card filesystem conflicts");
        ESP_LOGE(TAG, "💡 System will continue with SD card logging only");
        
        // Clean up and reset state
        tinyusb_msc_uninstall_driver();
        usb_msc_active = false;
        usb_host_connected = false;
        status.usb_msc_active = false;
        status.usb_host_connected = false;
        msc_handle = NULL;
        
        // Don't return error - allow system to continue without USB MSC
        ESP_LOGW(TAG, "⚠️  USB MSC disabled - SD card logging will work normally");
        usb_sd_initialized = false;  // Mark as not initialized
        return ret;
    }
    
    ESP_LOGI(TAG, "✓ TinyUSB MSC storage created for SD card");
    ESP_LOGI(TAG, "💾 SD card ready for safe USB switching");
    ESP_LOGI(TAG, "🔄 Automatic USB detection enabled");
    
    // Initialize status
    status.sd_card_available = true;
    status.logging_active = true;
    status.usb_msc_active = false;
    status.usb_host_connected = false;
    status.switch_count = 0;
    
    usb_sd_initialized = true;
    ESP_LOGI(TAG, "✓ USB SD direct access initialized successfully");
    
    return ESP_OK;
}

/**
 * Start USB MSC using direct SD card access
 */
esp_err_t usb_sd_direct_start(void) {
    if (!usb_sd_initialized) {
        ESP_LOGE(TAG, "USB SD direct access not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    if (usb_msc_active) {
        ESP_LOGI(TAG, "USB MSC already active");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "🚀 Starting USB MSC with direct SD card access...");
    ESP_LOGI(TAG, "⚠️  IMPORTANT: Data logging will be paused");
    ESP_LOGI(TAG, "💾 SD card will be exposed directly to USB host");

    if (!msc_handle) {
        ESP_LOGE(TAG, "MSC storage not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    // CRITICAL: Safe shutdown of storage manager before USB MSC
    ESP_LOGI(TAG, "🛡️  Performing safe shutdown of data logging");
    esp_err_t shutdown_ret = storage_manager_safe_shutdown();
    if (shutdown_ret != ESP_OK) {
        ESP_LOGE(TAG, "❌ Safe shutdown failed: %s", esp_err_to_name(shutdown_ret));
        ESP_LOGE(TAG, "⚠️  Cannot safely switch to USB mode");
        return shutdown_ret;
    } else {
        ESP_LOGI(TAG, "✓ Data logging safely stopped");
    }

    // Small delay to ensure all file operations complete
    vTaskDelay(pdMS_TO_TICKS(500));

    // Switch storage to USB mode
    esp_err_t ret = tinyusb_msc_set_storage_mount_point(msc_handle, TINYUSB_MSC_STORAGE_MOUNT_USB);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "❌ Failed to switch to USB mode: %s", esp_err_to_name(ret));
        
        // Try to restart logging if USB switch failed
        ESP_LOGW(TAG, "Attempting to restart logging after failed USB switch");
        storage_manager_start_logging();
        return ret;
    }

    status.switch_count++;
    ESP_LOGI(TAG, "✓ USB MSC started successfully");
    ESP_LOGI(TAG, "💻 Complete SD card should appear as USB drive");
    ESP_LOGI(TAG, "🔄 All your CSV files are directly accessible");

    return ESP_OK;
}
/**
 * Stop USB MSC and return SD card to application
 */
esp_err_t usb_sd_direct_stop(void) {
    if (!usb_msc_active) {
        return ESP_OK;
    }

    ESP_LOGI(TAG, "🛑 Stopping USB MSC and returning SD card to app...");

    if (!msc_handle) {
        return ESP_OK;
    }

    // Switch storage back to application mode
    esp_err_t ret = tinyusb_msc_set_storage_mount_point(msc_handle, TINYUSB_MSC_STORAGE_MOUNT_APP);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "❌ Failed to switch to app mode: %s", esp_err_to_name(ret));
        return ret;
    }

    // Small delay to ensure mount point switch completes
    vTaskDelay(pdMS_TO_TICKS(200));

    ESP_LOGI(TAG, "✓ USB MSC stopped - SD card returned to application");

    // IMPORTANT: Restart data logging after stopping USB MSC
    if (storage_manager_is_initialized()) {
        esp_err_t log_ret = storage_manager_start_logging();
        if (log_ret == ESP_OK) {
            ESP_LOGI(TAG, "✓ Data logging restarted successfully");
            ESP_LOGI(TAG, "📊 Sensor data collection resumed");
        } else {
            ESP_LOGW(TAG, "Failed to restart logging: %s", esp_err_to_name(log_ret));
        }
    }

    return ESP_OK;
}

/**
 * Check if USB MSC is currently active
 */
bool usb_sd_direct_is_active(void) {
    return usb_sd_initialized && usb_msc_active;
}

/**
 * Check if USB MSC system is initialized
 */
bool usb_sd_direct_is_initialized(void) {
    return usb_sd_initialized;
}

/**
 * Check if USB host is connected
 */
bool usb_sd_direct_is_connected(void) {
    if (!msc_handle) {
        return false;
    }
    
    tinyusb_msc_mount_point_t mount_point;
    esp_err_t ret = tinyusb_msc_get_storage_mount_point(msc_handle, &mount_point);
    if (ret != ESP_OK) {
        return false;
    }
    
    return (mount_point == TINYUSB_MSC_STORAGE_MOUNT_USB) && usb_host_connected;
}

/**
 * Get current status
 */
void usb_sd_direct_get_status(usb_sd_status_t *out_status) {
    if (out_status) {
        status.usb_msc_active = usb_msc_active;
        status.usb_host_connected = usb_host_connected;
        status.sd_card_available = (sd_card != NULL);
        status.logging_active = storage_manager_is_logging();
        memcpy(out_status, &status, sizeof(usb_sd_status_t));
    }
}

/**
 * Handle automatic USB detection and SD card switching
 */
void usb_sd_direct_process(void) {
    if (!usb_sd_initialized || !msc_handle) {
        // USB MSC not initialized - do nothing
        return;
    }

    static bool last_usb_connected = false;
    static uint32_t last_check_time = 0;
    static uint32_t connection_stable_time = 0;
    
    uint32_t current_time = esp_timer_get_time() / 1000;

    // Check USB connection every 1 second
    if ((current_time - last_check_time) < 1000) {
        return;
    }
    last_check_time = current_time;

    // Check if USB host is connected (via TinyUSB)
    bool usb_connected = usb_sd_direct_is_connected();

    // Debounce connection changes (wait 2 seconds for stable connection)
    if (usb_connected != last_usb_connected) {
        connection_stable_time = current_time;
        last_usb_connected = usb_connected;
        ESP_LOGI(TAG, "USB connection change detected, waiting for stability...");
        return;
    }

    // Connection must be stable for 2 seconds before acting
    if ((current_time - connection_stable_time) < 2000) {
        return;
    }

    // Handle automatic SD card switching
    if (usb_connected && !usb_msc_active) {
        // USB host connected - switch SD card to USB MSC
        ESP_LOGI(TAG, "🔌 USB host detected - switching SD card to USB mode");
        ESP_LOGI(TAG, "⚠️  Data logging will be paused");
        ESP_LOGI(TAG, "💾 Complete SD card will be accessible");
        ESP_LOGI(TAG, "💡 LED will turn BLUE");

        usb_sd_direct_start();

    } else if (!usb_connected && usb_msc_active) {
        // USB host disconnected - return SD card to application
        ESP_LOGI(TAG, "🔌 USB host disconnected - returning SD card to application");
        ESP_LOGI(TAG, "📊 Data logging will resume");
        ESP_LOGI(TAG, "💡 LED will return to distance indication");

        usb_sd_direct_stop();
    }
}

/**
 * Deinitialize USB SD direct access
 */
esp_err_t usb_sd_direct_deinit(void) {
    if (usb_sd_initialized) {
        usb_sd_direct_stop();
        
        if (msc_handle) {
            tinyusb_msc_delete_storage(msc_handle);
            msc_handle = NULL;
        }
        
        tinyusb_msc_uninstall_driver();
        
        usb_sd_initialized = false;
        sd_card = NULL;
        ESP_LOGI(TAG, "USB SD direct access deinitialized");
    }
    return ESP_OK;
}