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

#ifdef __cplusplus
}
#endif
