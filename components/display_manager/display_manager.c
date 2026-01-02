/**
 * display_manager.c
 * 
 * High-Performance Display Manager Implementation
 * Optimized for screen output with configurable refresh rates
 */

#include "display_manager.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>
#include <string.h>

static const char *TAG = "DISPLAY";

// Display state
static bool display_initialized = false;
static display_config_t current_config = {0};
static display_stats_t stats = {0};
static uint32_t last_update_time = 0;
static uint32_t frame_interval_us = 0;

// Performance tracking
static uint32_t cpu_start_time = 0;
static uint32_t total_cpu_time = 0;
static uint32_t fps_counter = 0;
static uint32_t fps_last_time = 0;

// Data buffers for efficient updates
static tof_data_t last_tof_data = {0};
static imu_data_t last_imu_data = {0};
static uint16_t last_min_distance = 9999;
static bool data_changed = false;

/**
 * Check if display update is needed based on refresh rate
 */
static bool should_update_display(void) {
    uint32_t current_time = esp_timer_get_time();
    
    if (frame_interval_us == 0) {
        return true; // First frame
    }
    
    return (current_time - last_update_time) >= frame_interval_us;
}

/**
 * Compact ToF grid display (optimized for performance)
 */
static void display_tof_compact(const tof_data_t *tof_data, uint16_t min_dist) {
    printf("ToF: ");
    for (int i = 0; i < 16; i++) {
        if (i % 4 == 0 && i > 0) printf(" | ");
        printf("%4u", tof_data->distances[i]);
    }
    printf(" Min:%4u\n", min_dist);
}

/**
 * Full ToF grid display with color coding
 */
static void display_tof_full(const tof_data_t *tof_data, uint16_t min_dist) {
    printf("┌─────────────────────────────────────┐\n");
    printf("│     ToF 4x4 Grid (mm)              │\n");
    printf("├─────────────────────────────────────┤\n");
    
    for (int row = 0; row < 4; row++) {
        printf("│ ");
        for (int col = 0; col < 4; col++) {
            int idx = row * 4 + col;
            uint16_t dist = tof_data->distances[idx];
            
            // Color coding
            if (dist < 100) {
                printf("\033[31m%4u\033[0m ", dist);      // Red
            } else if (dist < 500) {
                printf("\033[33m%4u\033[0m ", dist);      // Yellow
            } else {
                printf("\033[32m%4u\033[0m ", dist);      // Green
            }
        }
        printf("│\n");
    }
    
    printf("├─────────────────────────────────────┤\n");
    printf("│ Min Distance: %4u mm              │\n", min_dist);
    printf("└─────────────────────────────────────┘\n");
}

/**
 * Compact IMU display
 */
static void display_imu_compact(const imu_data_t *imu_data) {
    printf("IMU: R:%6.1f P:%6.1f Y:%6.1f | A:%5.1f,%5.1f,%5.1f\n",
           imu_data->roll, imu_data->pitch, imu_data->yaw,
           imu_data->accel_x, imu_data->accel_y, imu_data->accel_z);
}

/**
 * Full IMU display
 */
static void display_imu_full(const imu_data_t *imu_data) {
    printf("┌─────────────────────────────────────┐\n");
    printf("│         IMU Sensor Data             │\n");
    printf("├─────────────────────────────────────┤\n");
    printf("│ Roll:  %8.2f°                     │\n", imu_data->roll);
    printf("│ Pitch: %8.2f°                     │\n", imu_data->pitch);
    printf("│ Yaw:   %8.2f°                     │\n", imu_data->yaw);
    printf("├─────────────────────────────────────┤\n");
    printf("│ Accel: %6.2f %6.2f %6.2f m/s²    │\n", 
           imu_data->accel_x, imu_data->accel_y, imu_data->accel_z);
    printf("│ Gyro:  %6.2f %6.2f %6.2f deg/s   │\n", 
           imu_data->gyro_x, imu_data->gyro_y, imu_data->gyro_z);
    printf("└─────────────────────────────────────┘\n");
}

/**
 * Display system statistics
 */
static void display_system_stats(void) {
    storage_stats_t storage_stats;
    storage_manager_get_stats(&storage_stats);
    
    printf("Stats: FPS:%2lu CPU:%2lu%% Log:%lu Buf:%3.0f%% Drop:%lu\n",
           stats.fps_actual,
           stats.cpu_usage_percent,
           storage_stats.entries_logged,
           storage_stats.buffer_fill_percent,
           storage_stats.entries_dropped);
}

esp_err_t display_manager_init(const display_config_t *config) {
    if (!config) {
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGI(TAG, "Initializing display manager...");
    ESP_LOGI(TAG, "Refresh rate: %lu Hz", config->refresh_rate_hz);
    ESP_LOGI(TAG, "Mode: %s", config->compact_mode ? "Compact" : "Full");
    
    // Validate refresh rate
    if (config->refresh_rate_hz < 1 || config->refresh_rate_hz > 60) {
        ESP_LOGE(TAG, "Invalid refresh rate: %lu Hz (must be 1-60)", config->refresh_rate_hz);
        return ESP_ERR_INVALID_ARG;
    }
    
    // Copy configuration
    memcpy(&current_config, config, sizeof(display_config_t));
    
    // Calculate frame interval
    frame_interval_us = 1000000 / config->refresh_rate_hz;
    
    // Initialize statistics
    memset(&stats, 0, sizeof(display_stats_t));
    
    // Reset timing
    last_update_time = esp_timer_get_time();
    fps_last_time = last_update_time;
    
    display_initialized = true;
    
    ESP_LOGI(TAG, "✓ Display manager initialized");
    ESP_LOGI(TAG, "Frame interval: %lu µs", frame_interval_us);
    
    return ESP_OK;
}

esp_err_t display_manager_update(const tof_data_t *tof_data, 
                                const imu_data_t *imu_data, 
                                uint16_t min_distance) {
    if (!display_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    if (!tof_data || !imu_data) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // Check if we need to update based on refresh rate
    if (!should_update_display()) {
        return ESP_OK; // Skip this frame
    }
    
    // Start CPU usage measurement
    cpu_start_time = esp_timer_get_time();
    
    // Check if data actually changed (optimization)
    bool tof_changed = memcmp(tof_data, &last_tof_data, sizeof(tof_data_t)) != 0;
    bool imu_changed = memcmp(imu_data, &last_imu_data, sizeof(imu_data_t)) != 0;
    bool dist_changed = (min_distance != last_min_distance);
    
    data_changed = tof_changed || imu_changed || dist_changed;
    
    // Only update if data changed or forced refresh
    if (data_changed || (stats.frames_rendered % (current_config.refresh_rate_hz * 5)) == 0) {
        
        // Clear screen for full refresh (optional - can be disabled for performance)
        if (!current_config.compact_mode) {
            printf("\033[2J\033[H"); // Clear screen and move cursor to top
        }
        
        // Display ToF data
        if (current_config.show_tof_grid) {
            if (current_config.compact_mode) {
                display_tof_compact(tof_data, min_distance);
            } else {
                display_tof_full(tof_data, min_distance);
            }
        }
        
        // Display IMU data
        if (current_config.show_imu_data) {
            if (current_config.compact_mode) {
                display_imu_compact(imu_data);
            } else {
                display_imu_full(imu_data);
            }
        }
        
        // Display system statistics
        if (current_config.show_system_stats) {
            display_system_stats();
        }
        
        // Store last data for change detection
        memcpy(&last_tof_data, tof_data, sizeof(tof_data_t));
        memcpy(&last_imu_data, imu_data, sizeof(imu_data_t));
        last_min_distance = min_distance;
    }
    
    // Update timing and statistics
    uint32_t current_time = esp_timer_get_time();
    last_update_time = current_time;
    stats.frames_rendered++;
    fps_counter++;
    
    // Calculate CPU usage
    uint32_t frame_cpu_time = current_time - cpu_start_time;
    total_cpu_time += frame_cpu_time;
    
    // Update FPS every second
    if ((current_time - fps_last_time) >= 1000000) { // 1 second
        stats.fps_actual = fps_counter;
        stats.cpu_usage_percent = (total_cpu_time * 100) / (current_time - fps_last_time);
        
        // Reset counters
        fps_counter = 0;
        total_cpu_time = 0;
        fps_last_time = current_time;
    }
    
    stats.last_update_time = current_time;
    
    return ESP_OK;
}

esp_err_t display_manager_set_refresh_rate(uint32_t refresh_rate_hz) {
    if (!display_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    if (refresh_rate_hz < 1 || refresh_rate_hz > 60) {
        return ESP_ERR_INVALID_ARG;
    }
    
    current_config.refresh_rate_hz = refresh_rate_hz;
    frame_interval_us = 1000000 / refresh_rate_hz;
    
    ESP_LOGI(TAG, "Display refresh rate changed to %lu Hz", refresh_rate_hz);
    
    return ESP_OK;
}

esp_err_t display_manager_get_stats(display_stats_t *out_stats) {
    if (!display_initialized || !out_stats) {
        return ESP_ERR_INVALID_ARG;
    }
    
    memcpy(out_stats, &stats, sizeof(display_stats_t));
    return ESP_OK;
}

esp_err_t display_manager_configure(bool show_tof, bool show_imu, bool show_stats) {
    if (!display_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    current_config.show_tof_grid = show_tof;
    current_config.show_imu_data = show_imu;
    current_config.show_system_stats = show_stats;
    
    ESP_LOGI(TAG, "Display configuration updated: ToF=%s IMU=%s Stats=%s",
             show_tof ? "ON" : "OFF",
             show_imu ? "ON" : "OFF", 
             show_stats ? "ON" : "OFF");
    
    return ESP_OK;
}

void display_manager_process(void) {
    // This function can be used for additional processing if needed
    // Currently, all processing is done in display_manager_update()
}