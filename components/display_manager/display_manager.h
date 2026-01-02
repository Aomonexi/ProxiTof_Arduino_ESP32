/**
 * display_manager.h
 * 
 * High-Performance Display Manager for Screen Output
 * Optimized for high refresh rates with minimal performance impact
 */

#pragma once

#include "esp_err.h"
#include "sensor_manager.h"
#include "storage_manager.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Display configuration
 */
typedef struct {
    uint32_t refresh_rate_hz;      // Display refresh rate (10-60 Hz)
    bool show_tof_grid;            // Show ToF 4x4 grid
    bool show_imu_data;            // Show IMU orientation/acceleration
    bool show_system_stats;        // Show logging stats and health
    bool compact_mode;             // Compact display for small screens
} display_config_t;

/**
 * Display statistics
 */
typedef struct {
    uint32_t frames_rendered;      // Total frames rendered
    uint32_t fps_actual;           // Actual FPS achieved
    uint32_t cpu_usage_percent;    // CPU usage for display
    uint32_t last_update_time;     // Last update timestamp
} display_stats_t;

/**
 * Initialize display manager
 * 
 * @param config Display configuration
 * @return ESP_OK on success
 */
esp_err_t display_manager_init(const display_config_t *config);

/**
 * Update display with current sensor data
 * Call this at desired refresh rate (non-blocking)
 * 
 * @param tof_data Current ToF sensor data
 * @param imu_data Current IMU sensor data
 * @param min_distance Minimum distance from ToF grid
 * @return ESP_OK on success
 */
esp_err_t display_manager_update(const tof_data_t *tof_data, 
                                const imu_data_t *imu_data, 
                                uint16_t min_distance);

/**
 * Set display refresh rate
 * 
 * @param refresh_rate_hz New refresh rate (10-60 Hz)
 * @return ESP_OK on success
 */
esp_err_t display_manager_set_refresh_rate(uint32_t refresh_rate_hz);

/**
 * Get display statistics
 * 
 * @param stats Output statistics
 * @return ESP_OK on success
 */
esp_err_t display_manager_get_stats(display_stats_t *stats);

/**
 * Enable/disable display components
 * 
 * @param show_tof Show ToF grid
 * @param show_imu Show IMU data
 * @param show_stats Show system statistics
 * @return ESP_OK on success
 */
esp_err_t display_manager_configure(bool show_tof, bool show_imu, bool show_stats);

/**
 * Process display updates (call from main loop)
 * Handles timing and performance optimization
 */
void display_manager_process(void);

#ifdef __cplusplus
}
#endif