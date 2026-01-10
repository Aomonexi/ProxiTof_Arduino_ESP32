/**
 * usb_interface.c
 * 
 * USB Interface Implementation - Dedicated Port System
 * RIGHT port: USB Serial JTAG (programming, monitoring, time sync)
 * LEFT port: TinyUSB MSC (flash drive mode only)
 */

#include "usb_interface.h"
#include "smart_time.h"
#include "config_manager.h"
#include "usb_flash_partition.h"
#include "display_manager.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "driver/usb_serial_jtag.h"
#include "hal/usb_serial_jtag_ll.h"
#include <string.h>
#include <stdio.h>
#include <time.h>

static const char *TAG = "USB_IF";

static bool usb_initialized = false;
static char usb_rx_buffer[256];
static int usb_rx_pos = 0;

/**
 * Process USB time sync command
 * Expected format: "TIME:2024-12-24 15:30:00"
 */
static esp_err_t process_time_sync_command(const char* command) {
    if (strncmp(command, "TIME:", 5) != 0) {
        return ESP_ERR_INVALID_ARG;
    }
    
    const char* time_str = command + 5;  // Skip "TIME:"
    
    ESP_LOGI(TAG, "📅 Received time sync command: %s", time_str);
    
    // Set time using smart time system
    esp_err_t ret = smart_time_set_manual(time_str);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "✓ Time synchronized via USB Serial JTAG");
        ESP_LOGI(TAG, "💾 Time saved to DS3231 RTC (if available)");
        
        // Send confirmation back to computer via USB Serial JTAG
        const char* response = "TIME_SYNC_OK\r\n";
        usb_serial_jtag_write_bytes((const uint8_t*)response, strlen(response), 100);
        return ESP_OK;
    } else {
        ESP_LOGE(TAG, "❌ Time sync failed: %s", esp_err_to_name(ret));
        
        // Send error back to computer
        char response[64];
        snprintf(response, sizeof(response), "TIME_SYNC_ERROR:%s\r\n", esp_err_to_name(ret));
        usb_serial_jtag_write_bytes((const uint8_t*)response, strlen(response), 100);
        return ret;
    }
}

/**
 * Process USB commands from computer via USB Serial JTAG (RIGHT port)
 * This is completely separate from TinyUSB MSC (LEFT port)
 */
static void process_usb_serial_jtag_commands(void) {
    uint8_t buffer[64];
    int bytes_read = usb_serial_jtag_read_bytes(buffer, sizeof(buffer) - 1, 0);
    
    if (bytes_read <= 0) {
        return;
    }
    
    // Process received bytes
    for (int i = 0; i < bytes_read; i++) {
        char c = buffer[i];
        
        if (c == '\r' || c == '\n') {
            // End of command
            if (usb_rx_pos > 0) {
                usb_rx_buffer[usb_rx_pos] = '\0';
                
                ESP_LOGI(TAG, "📨 USB Serial JTAG command: %s", usb_rx_buffer);
                
                // Process different command types
                if (strncmp(usb_rx_buffer, "TIME:", 5) == 0) {
                    process_time_sync_command(usb_rx_buffer);
                } else if (strcmp(usb_rx_buffer, "STATUS") == 0) {
                    // Send system status
                    time_status_t time_status;
                    smart_time_get_status(&time_status);
                    
                    char status_msg[256];
                    snprintf(status_msg, sizeof(status_msg),
                            "STATUS:DS3231=%s,TIME_SOURCE=%s,SYNC_COUNT=%lu,MSC_ACTIVE=%s\r\n",
                            time_status.ds3231_available ? "OK" : "FAIL",
                            smart_time_source_name(time_status.current_source),
                            time_status.sync_count,
                            usb_msc_sdcard_is_active() ? "YES" : "NO");
                    
                    usb_serial_jtag_write_bytes((const uint8_t*)status_msg, strlen(status_msg), 100);
                } else if (strcmp(usb_rx_buffer, "USB_MSC_START") == 0) {
                    // Manually start USB MSC for testing
                    ESP_LOGI(TAG, "📨 Manual USB MSC start command");
                    
                    // Try direct SD card USB MSC approach
                    esp_err_t ret = usb_msc_sdcard_pause_logging();
                    if (ret == ESP_OK) {
                        const char* response = "USB_MSC_SD_STARTED\r\n";
                        usb_serial_jtag_write_bytes((const uint8_t*)response, strlen(response), 100);
                        ESP_LOGI(TAG, "✓ USB MSC started with direct SD card access");
                    } else {
                        char response[64];
                        snprintf(response, sizeof(response), "USB_MSC_SD_ERROR:%s\r\n", esp_err_to_name(ret));
                        usb_serial_jtag_write_bytes((const uint8_t*)response, strlen(response), 100);
                        ESP_LOGW(TAG, "❌ USB MSC direct SD card approach failed: %s", esp_err_to_name(ret));
                    }
                    
                } else if (strcmp(usb_rx_buffer, "USB_MSC_STOP") == 0) {
                    // Manually stop USB MSC for testing
                    ESP_LOGI(TAG, "📨 Manual USB MSC stop command");
                    
                    esp_err_t ret = usb_msc_sdcard_resume_logging();
                    if (ret == ESP_OK) {
                        const char* response = "USB_MSC_SD_STOPPED\r\n";
                        usb_serial_jtag_write_bytes((const uint8_t*)response, strlen(response), 100);
                        ESP_LOGI(TAG, "✓ USB MSC stopped");
                    } else {
                        char response[64];
                        snprintf(response, sizeof(response), "USB_MSC_SD_STOP_ERROR:%s\r\n", esp_err_to_name(ret));
                        usb_serial_jtag_write_bytes((const uint8_t*)response, strlen(response), 100);
                    }
                    
                } else if (strcmp(usb_rx_buffer, "USB_STATUS") == 0) {
                    // Get detailed USB status
                    ESP_LOGI(TAG, "📨 USB status command");
                    
                    usb_sd_status_t usb_status;
                    usb_msc_sdcard_get_status(&usb_status);
                    
                    char status_msg[256];
                    snprintf(status_msg, sizeof(status_msg),
                            "USB_STATUS:MSC=%s,HOST=%s,SD=%s,LOG=%s,SWITCHES=%lu\r\n",
                            usb_status.usb_msc_active ? "ACTIVE" : "INACTIVE",
                            usb_status.usb_host_connected ? "CONNECTED" : "DISCONNECTED",
                            usb_status.sd_card_available ? "AVAILABLE" : "UNAVAILABLE",
                            usb_status.logging_active ? "ACTIVE" : "PAUSED",
                            usb_status.switch_count);
                    
                    usb_serial_jtag_write_bytes((const uint8_t*)status_msg, strlen(status_msg), 100);
                    
                } else if (strcmp(usb_rx_buffer, "USB_HARDWARE_TEST") == 0) {
                    // Test USB-OTG hardware
                    ESP_LOGI(TAG, "📨 USB hardware test command");
                    
                    // Try to initialize SD direct USB MSC
                    esp_err_t ret = usb_msc_sdcard_init();
                    if (ret == ESP_OK) {
                        const char* response = "USB_HARDWARE_TEST_OK\r\n";
                        usb_serial_jtag_write_bytes((const uint8_t*)response, strlen(response), 100);
                    } else {
                        char response[64];
                        snprintf(response, sizeof(response), "USB_HARDWARE_TEST_ERROR:%s\r\n", esp_err_to_name(ret));
                        usb_serial_jtag_write_bytes((const uint8_t*)response, strlen(response), 100);
                    }
                } else if (strcmp(usb_rx_buffer, "TEST_FILE_WRITE") == 0) {
                    // Test file writing to SD card
                    ESP_LOGI(TAG, "📨 Test file write command");
                    FILE *test_file = fopen("/sdcard/test_write.txt", "w");
                    if (test_file) {
                        fprintf(test_file, "Test write from USB command at timestamp: %ld\n", (long)time(NULL));
                        fflush(test_file);
                        fclose(test_file);
                        const char* response = "TEST_FILE_WRITE_OK\r\n";
                        usb_serial_jtag_write_bytes((const uint8_t*)response, strlen(response), 100);
                    } else {
                        const char* response = "TEST_FILE_WRITE_ERROR\r\n";
                        usb_serial_jtag_write_bytes((const uint8_t*)response, strlen(response), 100);
                    }
                } else if (strcmp(usb_rx_buffer, "HELP") == 0) {
                    // Send help information
                    const char* help_msg = 
                        "USB Serial JTAG Commands (RIGHT port):\r\n"
                        "TIME:YYYY-MM-DD HH:MM:SS - Sync time\r\n"
                        "STATUS - Get system status\r\n"
                        "USB_MSC_START - Start USB Mass Storage (direct SD card)\r\n"
                        "USB_MSC_STOP - Stop USB Mass Storage\r\n"
                        "USB_STATUS - Get detailed USB status\r\n"
                        "TEST_FILE_WRITE - Test SD card file writing\r\n"
                        "USB_HARDWARE_TEST - Test USB-OTG hardware\r\n"
                        "HELP - Show this help\r\n"
                        "Note: USB MSC uses direct SD card access (logging pauses)\r\n";
                    usb_serial_jtag_write_bytes((const uint8_t*)help_msg, strlen(help_msg), 100);
                } else {
                    // Unknown command
                    const char* error_msg = "ERROR:Unknown command. Send HELP for commands.\r\n";
                    usb_serial_jtag_write_bytes((const uint8_t*)error_msg, strlen(error_msg), 100);
                }
                
                usb_rx_pos = 0;  // Reset buffer
            }
        } else if (c >= 32 && c <= 126) {  // Printable characters only
            if (usb_rx_pos < sizeof(usb_rx_buffer) - 1) {
                usb_rx_buffer[usb_rx_pos++] = c;
            }
        }
    }
    
    // Reset buffer if it gets too full (prevent overflow)
    if (usb_rx_pos >= sizeof(usb_rx_buffer) - 1) {
        usb_rx_pos = 0;
        ESP_LOGW(TAG, "USB command buffer overflow, resetting");
    }
}

/**
 * Check if USB Serial JTAG is connected (RIGHT port)
 * This is separate from TinyUSB MSC detection
 */
bool usb_interface_is_connected(void) {
    if (!usb_initialized) {
        return false;
    }
    
    // Check if USB Serial JTAG is connected
    return usb_serial_jtag_ll_txfifo_writable();
}

/**
 * Check if USB MSC mode is currently active (direct SD card)
 * Returns true if device is in flash drive mode
 */
bool usb_interface_is_msc_active(void) {
    return false;  // USB MSC disabled for now
}

/**
 * Handle USB MSC connection changes (direct SD card)
 * This handles the direct SD card USB MSC system
 */
void usb_interface_handle_connection_changes(void) {
    // Handle direct SD card USB MSC state changes
    static bool last_msc_active = false;
    bool current_msc_active = usb_msc_sdcard_is_active();
    
    // Log MSC state changes for debugging
    if (current_msc_active != last_msc_active) {
        if (current_msc_active) {
            ESP_LOGI(TAG, "🔌 USB MSC activated (direct SD card mode)");
            ESP_LOGI(TAG, "⚠️  Data logging paused");
            ESP_LOGI(TAG, "💡 LED should be BLUE");
        } else {
            ESP_LOGI(TAG, "🔌 USB MSC deactivated (normal mode)");
            ESP_LOGI(TAG, "📊 Data logging resumed");
            ESP_LOGI(TAG, "💡 LED returns to distance indication");
        }
        last_msc_active = current_msc_active;
    }
}

esp_err_t usb_interface_init(void) {
    if (usb_initialized) {
        return ESP_OK;
    }
    
    ESP_LOGI(TAG, "Initializing USB Serial JTAG interface (RIGHT port)...");
    
    // Configure USB Serial JTAG for RIGHT port communication
    usb_serial_jtag_driver_config_t usb_serial_jtag_config = {
        .rx_buffer_size = 512,
        .tx_buffer_size = 512,
    };
    
    esp_err_t ret = usb_serial_jtag_driver_install(&usb_serial_jtag_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to install USB Serial JTAG driver: %s", esp_err_to_name(ret));
        return ret;
    }
    
    usb_initialized = true;
    
    ESP_LOGI(TAG, "✓ USB Serial JTAG interface initialized");
    ESP_LOGI(TAG, "🔧 RIGHT port: Programming, monitoring, time sync");
    ESP_LOGI(TAG, "💾 LEFT port: Flash drive (handled by USB MSC)");
    ESP_LOGI(TAG, "📝 Send TIME:YYYY-MM-DD HH:MM:SS for time sync");
    ESP_LOGI(TAG, "📊 Send STATUS for system information");
    
    return ESP_OK;
}

void usb_interface_process(void) {
    if (!usb_initialized) {
        return;
    }
    
    // Process USB Serial JTAG commands from RIGHT port
    process_usb_serial_jtag_commands();
    
    // Handle USB MSC state changes (LEFT port)
    usb_interface_handle_connection_changes();
}

esp_err_t usb_interface_sync_time(void) {
    // This function can be called programmatically to request time sync
    // The actual sync happens when computer sends TIME: command via USB Serial JTAG
    ESP_LOGI(TAG, "📅 Time sync requested via USB Serial JTAG");
    ESP_LOGI(TAG, "💡 Computer should send: TIME:YYYY-MM-DD HH:MM:SS");
    
    if (usb_interface_is_connected()) {
        const char* request_msg = "TIME_SYNC_REQUEST\r\n";
        usb_serial_jtag_write_bytes((const uint8_t*)request_msg, strlen(request_msg), 100);
        return ESP_OK;
    } else {
        ESP_LOGW(TAG, "USB Serial JTAG not connected - cannot request time sync");
        return ESP_ERR_NOT_FOUND;
    }
}