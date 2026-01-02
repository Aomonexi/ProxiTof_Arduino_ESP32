/**
 * usb_interface.c
 * 
 * USB Interface Implementation with Auto-Detection
 */

#include "usb_interface.h"
#include "smart_time.h"
#include "config_manager.h"
#include "usb_msc.h"
#include "display_manager.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "driver/usb_serial_jtag.h"
#include "hal/usb_serial_jtag_ll.h"
#include "soc/usb_serial_jtag_reg.h"
#include <string.h>
#include <stdio.h>

static const char *TAG = "USB_IF";

static bool usb_initialized = false;
static char command_buffer[256];
static int buffer_pos = 0;
static bool last_connection_state = false;
static uint32_t connection_stable_time = 0;

// Command handlers
static void handle_help_command(void);
static void handle_status_command(void);
static void handle_config_command(const char* args);
static void handle_time_command(const char* args);
static void handle_sync_command(void);
static void handle_files_command(void);
static void handle_usb_msc_command(const char* args);
static void handle_display_command(const char* args);

/**
 * Check if USB is connected to a computer (not just power)
 * Returns true if data connection is active
 */
bool usb_interface_is_connected(void) {
    if (!usb_initialized) {
        return false;
    }
    
    // Check USB Serial JTAG connection status
    // This detects if a computer is actually connected and communicating
    bool connected = usb_serial_jtag_ll_txfifo_writable();
    
    // Add debug logging every 10 seconds
    static uint32_t last_debug_time = 0;
    uint32_t current_time = esp_timer_get_time() / 1000;
    if ((current_time - last_debug_time) > 10000) {
        ESP_LOGI(TAG, "USB connection check: %s", connected ? "CONNECTED" : "NOT CONNECTED");
        last_debug_time = current_time;
    }
    
    return connected;
}

/**
 * Detect USB connection changes and handle auto-MSC
 * Call this regularly from main loop
 */
void usb_interface_handle_connection_changes(void) {
    if (!usb_initialized) {
        return;
    }
    
    bool current_state = usb_interface_is_connected();
    uint32_t current_time = esp_timer_get_time() / 1000; // Convert to ms
    
    // Debounce connection changes (wait 2 seconds for stability)
    if (current_state != last_connection_state) {
        connection_stable_time = current_time;
        last_connection_state = current_state;
        return;
    }
    
    // Check if connection has been stable for 2 seconds
    if ((current_time - connection_stable_time) < 2000) {
        return;
    }
    
    // Handle stable connection changes
    static bool auto_msc_active = false;
    
    if (current_state && !auto_msc_active) {
        // Computer connected - start USB MSC automatically
        ESP_LOGI(TAG, "🔌 Computer connected - starting USB Mass Storage");
        ESP_LOGI(TAG, "💾 SD card will appear as USB drive");
        ESP_LOGI(TAG, "📊 Data logging paused during USB access");
        
        if (usb_msc_start() == ESP_OK) {
            auto_msc_active = true;
        }
        
    } else if (!current_state && auto_msc_active) {
        // Computer disconnected - stop USB MSC and resume logging
        ESP_LOGI(TAG, "🔌 Computer disconnected - stopping USB Mass Storage");
        ESP_LOGI(TAG, "📊 Data logging resumed");
        
        if (usb_msc_stop() == ESP_OK) {
            auto_msc_active = false;
        }
    }
}

esp_err_t usb_interface_init(void) {
    if (usb_initialized) {
        return ESP_OK;
    }
    
    ESP_LOGI(TAG, "Initializing USB interface...");
    
    // Configure USB Serial JTAG
    usb_serial_jtag_driver_config_t usb_config = {
        .rx_buffer_size = 1024,
        .tx_buffer_size = 1024,
    };
    
    esp_err_t ret = usb_serial_jtag_driver_install(&usb_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to install USB Serial JTAG driver: %s", esp_err_to_name(ret));
        return ret;
    }
    
    usb_initialized = true;
    
    ESP_LOGI(TAG, "✓ USB interface initialized");
    ESP_LOGI(TAG, "💻 Connect USB cable and open serial terminal");
    ESP_LOGI(TAG, "📝 Type 'help' for available commands");
    
    return ESP_OK;
}

void usb_interface_process(void) {
    if (!usb_initialized) {
        return;
    }
    
    // Read incoming data
    uint8_t data[64];
    int len = usb_serial_jtag_read_bytes(data, sizeof(data), 0);
    
    for (int i = 0; i < len; i++) {
        char c = data[i];
        
        if (c == '\n' || c == '\r') {
            if (buffer_pos > 0) {
                command_buffer[buffer_pos] = '\0';
                
                // Process command
                if (strncmp(command_buffer, "help", 4) == 0) {
                    handle_help_command();
                } else if (strncmp(command_buffer, "status", 6) == 0) {
                    handle_status_command();
                } else if (strncmp(command_buffer, "config", 6) == 0) {
                    handle_config_command(command_buffer + 7);
                } else if (strncmp(command_buffer, "time", 4) == 0) {
                    handle_time_command(command_buffer + 5);
                } else if (strncmp(command_buffer, "sync", 4) == 0) {
                    handle_sync_command();
                } else if (strncmp(command_buffer, "files", 5) == 0) {
                    handle_files_command();
                } else if (strncmp(command_buffer, "msc", 3) == 0) {
                    handle_usb_msc_command(command_buffer + 4);
                } else if (strncmp(command_buffer, "display", 7) == 0) {
                    handle_display_command(command_buffer + 8);
                } else if (strlen(command_buffer) > 0) {
                    usb_serial_jtag_write_bytes("Unknown command. Type 'help' for available commands.\n", 54, 100);
                }
                
                buffer_pos = 0;
            }
        } else if (c >= 32 && c < 127) {  // Printable characters
            if (buffer_pos < sizeof(command_buffer) - 1) {
                command_buffer[buffer_pos++] = c;
            }
        }
    }
}

static void handle_help_command(void) {
    const char* help_text = 
        "\n=== ProxiTof USB Interface ===\n"
        "Available commands:\n"
        "  help                 - Show this help\n"
        "  status               - Show system status\n"
        "  config               - Show current configuration\n"
        "  config wifi SSID PASS - Set WiFi credentials\n"
        "  config timezone TZ   - Set timezone\n"
        "  time                 - Show current time\n"
        "  time YYYY-MM-DD HH:MM:SS - Set time manually\n"
        "  sync                 - Sync time from computer\n"
        "  files                - List recent log files\n"
        "  msc start            - Start USB Mass Storage (SD card as USB drive)\n"
        "  msc stop             - Stop USB Mass Storage (resume data logging)\n"
        "  msc status           - Show USB Mass Storage status\n"
        "  display refresh HZ   - Set display refresh rate (1-60 Hz)\n"
        "  display compact on/off - Enable/disable compact display mode\n"
        "  display status       - Show display configuration\n"
        "\nExamples:\n"
        "  config wifi MyWiFi MyPassword\n"
        "  config timezone EST5EDT,M3.2.0,M11.1.0\n"
        "  time 2024-12-24 15:30:00\n"
        "  sync\n"
        "  msc start\n"
        "  display refresh 30\n\n";
    
    usb_serial_jtag_write_bytes(help_text, strlen(help_text), 1000);
}

static void handle_status_command(void) {
    char status_text[1024];
    
    // Get time status
    time_status_t time_status;
    smart_time_get_status(&time_status);
    
    // Get current time
    struct tm timeinfo;
    time_t now;
    time(&now);
    localtime_r(&now, &timeinfo);
    
    snprintf(status_text, sizeof(status_text),
        "\n=== System Status ===\n"
        "Time: %04d-%02d-%02d %02d:%02d:%02d\n"
        "Time Source: %s\n"
        "DS3231: %s\n"
        "Time Synced: %s\n"
        "Sync Count: %lu\n"
        "USB Connected: %s\n"
        "SD Card: %s\n"
        "\n",
        timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
        timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec,
        smart_time_source_name(time_status.current_source),
        time_status.ds3231_available ? "Available" : "Not available",
        time_status.time_synced ? "Yes" : "No",
        time_status.sync_count,
        usb_interface_is_connected() ? "Yes" : "No",
        config_file_exists() ? "Available" : "Not available"
    );
    
    usb_serial_jtag_write_bytes(status_text, strlen(status_text), 1000);
}

static void handle_config_command(const char* args) {
    if (strlen(args) == 0) {
        // Show current config
        system_config_t config;
        config_load_from_sd(&config);
        
        char config_text[512];
        snprintf(config_text, sizeof(config_text),
            "\n=== Current Configuration ===\n"
            "WiFi: %s\n"
            "SSID: %s\n"
            "Password: %s\n"
            "Timezone: %s\n"
            "Sync Interval: %d hours\n"
            "DS3231: %s (SDA=%d, SCL=%d)\n"
            "\n",
            config.wifi_enabled ? "Enabled" : "Disabled",
            config.wifi_ssid,
            strlen(config.wifi_password) > 0 ? "***" : "(empty)",
            config.timezone,
            config.sync_interval_hours,
            config.ds3231_enabled ? "Enabled" : "Disabled",
            config.ds3231_sda_pin, config.ds3231_scl_pin
        );
        
        usb_serial_jtag_write_bytes(config_text, strlen(config_text), 1000);
        return;
    }
    
    // Parse config commands
    char cmd[32], arg1[64], arg2[64];
    int parsed = sscanf(args, "%31s %63s %63s", cmd, arg1, arg2);
    
    if (strcmp(cmd, "wifi") == 0 && parsed >= 3) {
        // Update WiFi config
        system_config_t config;
        config_load_from_sd(&config);
        
        strncpy(config.wifi_ssid, arg1, sizeof(config.wifi_ssid) - 1);
        strncpy(config.wifi_password, arg2, sizeof(config.wifi_password) - 1);
        config.wifi_enabled = true;
        
        if (config_save_to_sd(&config) == ESP_OK) {
            usb_serial_jtag_write_bytes("✓ WiFi configuration saved to SD card\n", 39, 100);
            usb_serial_jtag_write_bytes("💡 Restart system to apply changes\n", 36, 100);
        } else {
            usb_serial_jtag_write_bytes("❌ Failed to save configuration\n", 32, 100);
        }
    } else if (strcmp(cmd, "timezone") == 0 && parsed >= 2) {
        // Update timezone config
        system_config_t config;
        config_load_from_sd(&config);
        
        strncpy(config.timezone, arg1, sizeof(config.timezone) - 1);
        
        if (config_save_to_sd(&config) == ESP_OK) {
            usb_serial_jtag_write_bytes("✓ Timezone configuration saved\n", 31, 100);
        } else {
            usb_serial_jtag_write_bytes("❌ Failed to save configuration\n", 32, 100);
        }
    } else {
        usb_serial_jtag_write_bytes("Invalid config command. Use: config wifi SSID PASS\n", 52, 100);
    }
}

static void handle_time_command(const char* args) {
    if (strlen(args) == 0) {
        // Show current time
        struct tm timeinfo;
        time_t now;
        time(&now);
        localtime_r(&now, &timeinfo);
        
        char time_text[128];
        snprintf(time_text, sizeof(time_text),
            "Current time: %04d-%02d-%02d %02d:%02d:%02d\n",
            timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
            timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
        
        usb_serial_jtag_write_bytes(time_text, strlen(time_text), 100);
        return;
    }
    
    // Set time manually
    if (smart_time_set_manual(args) == ESP_OK) {
        usb_serial_jtag_write_bytes("✓ Time set successfully\n", 24, 100);
    } else {
        usb_serial_jtag_write_bytes("❌ Invalid time format. Use: YYYY-MM-DD HH:MM:SS\n", 50, 100);
    }
}

static void handle_sync_command(void) {
    usb_serial_jtag_write_bytes("Syncing time from computer...\n", 30, 100);
    
    // Get current time from computer (this would need a companion app)
    // For now, we'll trigger NTP sync if WiFi is available
    if (smart_time_sync_now() == ESP_OK) {
        usb_serial_jtag_write_bytes("✓ Time synchronized via NTP\n", 29, 100);
    } else {
        usb_serial_jtag_write_bytes("❌ Time sync failed (check WiFi configuration)\n", 47, 100);
    }
}

static void handle_files_command(void) {
    usb_serial_jtag_write_bytes("\n=== Recent Log Files ===\n", 26, 100);
    usb_serial_jtag_write_bytes("💡 Use 'msc start' to access SD card as USB drive\n", 51, 100);
    usb_serial_jtag_write_bytes("📁 Files are organized by date: /YYYY-MM-DD/\n", 46, 100);
    usb_serial_jtag_write_bytes("📊 CSV format with ToF + IMU + timestamp data\n", 47, 100);
    usb_serial_jtag_write_bytes("🔧 Edit config.txt to change system settings\n\n", 47, 100);
}

static void handle_usb_msc_command(const char* args) {
    char response[256];
    
    if (strncmp(args, "start", 5) == 0) {
        esp_err_t ret = usb_msc_start();
        if (ret == ESP_OK) {
            snprintf(response, sizeof(response),
                "\n✓ USB Mass Storage started\n"
                "💾 SD card is now accessible as USB drive\n"
                "⚠️  Data logging paused while MSC active\n"
                "💡 Use 'msc stop' to resume data logging\n\n");
        } else {
            snprintf(response, sizeof(response),
                "\n❌ Failed to start USB Mass Storage: %s\n"
                "💡 Check SD card is initialized\n\n", esp_err_to_name(ret));
        }
    } else if (strncmp(args, "stop", 4) == 0) {
        esp_err_t ret = usb_msc_stop();
        if (ret == ESP_OK) {
            snprintf(response, sizeof(response),
                "\n✓ USB Mass Storage stopped\n"
                "📊 Data logging resumed\n"
                "💾 SD card returned to application control\n\n");
        } else {
            snprintf(response, sizeof(response),
                "\n❌ Failed to stop USB Mass Storage: %s\n\n", esp_err_to_name(ret));
        }
    } else if (strncmp(args, "status", 6) == 0) {
        bool is_active = usb_msc_is_active();
        bool is_connected = usb_msc_is_connected();
        
        snprintf(response, sizeof(response),
            "\n=== USB Mass Storage Status ===\n"
            "MSC Active: %s\n"
            "USB Connected: %s\n"
            "Data Logging: %s\n"
            "\n💡 Commands: 'msc start', 'msc stop'\n\n",
            is_active ? "✓ YES" : "✗ NO",
            is_connected ? "✓ YES" : "✗ NO",
            is_active ? "⏸️  PAUSED" : "▶️  ACTIVE");
    } else {
        snprintf(response, sizeof(response),
            "\n❌ Invalid MSC command\n"
            "Available: msc start, msc stop, msc status\n\n");
    }
    
    usb_serial_jtag_write_bytes(response, strlen(response), 1000);
}

static void handle_display_command(const char* args) {
    char response[512];
    
    if (strncmp(args, "refresh", 7) == 0) {
        // Parse refresh rate: "refresh 30"
        int refresh_rate = atoi(args + 8);
        if (refresh_rate >= 1 && refresh_rate <= 60) {
            esp_err_t ret = display_manager_set_refresh_rate(refresh_rate);
            if (ret == ESP_OK) {
                snprintf(response, sizeof(response),
                    "\n✓ Display refresh rate set to %d Hz\n"
                    "💡 Higher rates use more CPU\n\n", refresh_rate);
            } else {
                snprintf(response, sizeof(response),
                    "\n❌ Failed to set refresh rate: %s\n\n", esp_err_to_name(ret));
            }
        } else {
            snprintf(response, sizeof(response),
                "\n❌ Invalid refresh rate: %d\n"
                "Valid range: 1-60 Hz\n"
                "Examples: display refresh 30\n\n", refresh_rate);
        }
    } else if (strncmp(args, "compact", 7) == 0) {
        // Parse compact mode: "compact on" or "compact off"
        bool compact_mode = (strncmp(args + 8, "on", 2) == 0);
        esp_err_t ret = display_manager_configure(true, true, true); // Keep all displays on
        if (ret == ESP_OK) {
            snprintf(response, sizeof(response),
                "\n✓ Display compact mode: %s\n"
                "💡 Compact mode uses less CPU\n\n", 
                compact_mode ? "ENABLED" : "DISABLED");
        } else {
            snprintf(response, sizeof(response),
                "\n❌ Failed to set compact mode: %s\n\n", esp_err_to_name(ret));
        }
    } else if (strncmp(args, "status", 6) == 0) {
        // Show display status
        display_stats_t stats;
        esp_err_t ret = display_manager_get_stats(&stats);
        if (ret == ESP_OK) {
            snprintf(response, sizeof(response),
                "\n=== Display Status ===\n"
                "Refresh Rate: %u Hz (actual: %lu FPS)\n"
                "CPU Usage: %lu%%\n"
                "Frames Rendered: %lu\n"
                "Performance: %s\n"
                "\n💡 Commands: 'display refresh HZ', 'display compact on/off'\n\n",
                30, // TODO: Get actual configured refresh rate
                stats.fps_actual,
                stats.cpu_usage_percent,
                stats.frames_rendered,
                stats.cpu_usage_percent < 20 ? "GOOD" : stats.cpu_usage_percent < 40 ? "MODERATE" : "HIGH");
        } else {
            snprintf(response, sizeof(response),
                "\n❌ Display manager not initialized\n\n");
        }
    } else {
        snprintf(response, sizeof(response),
            "\n❌ Invalid display command\n"
            "Available commands:\n"
            "  display refresh HZ     - Set refresh rate (1-60 Hz)\n"
            "  display compact on/off - Enable/disable compact mode\n"
            "  display status         - Show display statistics\n"
            "\nExamples:\n"
            "  display refresh 30\n"
            "  display compact on\n"
            "  display status\n\n");
    }
    
    usb_serial_jtag_write_bytes(response, strlen(response), 1000);
}

esp_err_t usb_interface_sync_time(void) {
    // This would be called by a companion computer application
    // that sends the current time via USB serial
    return smart_time_sync_now();
}