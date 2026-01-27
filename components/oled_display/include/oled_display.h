/**
 * oled_display.h
 * 
 * OLED Display Driver for SSD1306 (128x64, SPI)
 * Real-time sensor visualization at 60 FPS
 */

#pragma once

#include "esp_err.h"
#include "sensor_manager.h"
#include "storage_manager.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Display modes
typedef enum {
    OLED_MODE_DASHBOARD = 0,    // Full dashboard (ToF + IMU + Stats)
    OLED_MODE_TOF_FOCUS,        // Large ToF grid
    OLED_MODE_IMU_FOCUS,        // Large IMU display
    OLED_MODE_STATUS,           // System status
    OLED_MODE_NEAR_MISS,        // Near-miss alert details
    OLED_MODE_WIFI_SETTINGS,    // WiFi control and time sync
    OLED_MODE_GPS,              // GPS location and status
    OLED_MODE_SHUTDOWN,         // Safe shutdown confirmation
    OLED_MODE_MAX
} oled_display_mode_t;

// Display configuration
typedef struct {
    uint8_t brightness;         // 0-255
    bool flip_display;          // Rotate 180 degrees
    oled_display_mode_t mode;   // Display mode
} oled_config_t;

/**
 * Initialize OLED display
 * 
 * @return ESP_OK on success
 */
esp_err_t oled_display_init(void);

/**
 * Update display with current sensor data
 * Call at 60 Hz for smooth updates
 * 
 * @param tof_data ToF sensor data
 * @param imu_data IMU sensor data
 * @param min_distance Minimum distance from ToF grid
 * @return ESP_OK on success
 */
esp_err_t oled_display_update(const tof_data_t *tof_data, 
                              const imu_data_t *imu_data, 
                              uint16_t min_distance);

/**
 * Set display mode
 * 
 * @param mode Display mode
 * @return ESP_OK on success
 */
esp_err_t oled_display_set_mode(oled_display_mode_t mode);

/**
 * Set display brightness
 * 
 * @param brightness 0-255
 * @return ESP_OK on success
 */
esp_err_t oled_display_set_brightness(uint8_t brightness);

/**
 * Clear display
 * 
 * @return ESP_OK on success
 */
esp_err_t oled_display_clear(void);

/**
 * Get current display mode
 * 
 * @return Current display mode
 */
oled_display_mode_t oled_display_get_mode(void);

/**
 * Handle action button press (context-aware)
 * - Dashboard/Near-Miss mode: Flag event
 * - WiFi Settings mode: Toggle WiFi / Trigger sync
 * - Other modes: No action
 * 
 * @return ESP_OK if action was handled
 */
esp_err_t oled_display_handle_action_button(void);

/**
 * Show startup animation with Proxitof logo and loading progress
 * Displays progress bar in yellow zone while sensors initialize
 * 
 * @param progress Progress percentage (0-100)
 * @param status_text Status message to display (e.g., "Initializing ToF...")
 * @return ESP_OK on success
 */
esp_err_t oled_display_show_startup(uint8_t progress, const char *status_text);

/**
 * Check if shutdown has been confirmed
 * Returns true if user pressed action button twice in shutdown mode
 * 
 * @return true if shutdown confirmed, false otherwise
 */
bool oled_display_is_shutdown_confirmed(void);

#ifdef __cplusplus
}
#endif
