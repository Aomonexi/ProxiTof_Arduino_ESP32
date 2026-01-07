/**
 * storage_manager.h
 * 
 * ESP-IDF Storage Manager Component
 * Handles SD card operations with circular buffering for non-blocking writes
 */

#ifndef STORAGE_MANAGER_H
#define STORAGE_MANAGER_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "sdmmc_cmd.h"  // For sdmmc_card_t

#ifdef __cplusplus
extern "C" {
#endif

// =============================================================================
// Configuration Constants
// =============================================================================

#define STORAGE_BUFFER_SIZE 120         // 2 seconds at 60Hz
#define STORAGE_MAX_FILE_SIZE (100 * 1024 * 1024)  // 100MB
#define STORAGE_FLUSH_THRESHOLD 80      // Flush at 80% capacity

// =============================================================================
// Data Structures
// =============================================================================

/**
 * Simple log entry for raw sensor data
 * Simplified version without derived features (those come from Task 4)
 */
typedef struct {
    uint32_t timestamp_ms;
    
    // ToF data (16 zones)
    uint16_t tof[16];
    
    // IMU data (9 values)
    float roll, pitch, yaw;
    float accel_x, accel_y, accel_z;
    float gyro_x, gyro_y, gyro_z;
    
    // Flags
    uint8_t flag_manual;
    
} log_entry_t;

/**
 * Storage statistics
 */
typedef struct {
    uint32_t entries_logged;
    uint32_t entries_dropped;
    uint32_t flush_count;
    uint32_t current_file_size;
    float buffer_fill_percent;
    bool sd_card_present;
    bool logging_active;
} storage_stats_t;

// =============================================================================
// Public API
// =============================================================================

/**
 * Initialize storage manager and SD card
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t storage_manager_init(void);

/**
 * Start a new logging session
 * Creates new log file with timestamp
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t storage_manager_start_logging(void);

/**
 * Stop current logging session
 * Flushes buffer and closes file
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t storage_manager_stop_logging(void);

/**
 * Write sensor data to buffer
 * Non-blocking - data is buffered and written asynchronously
 * @param entry Log entry to write
 * @return ESP_OK on success, ESP_ERR_NO_MEM if buffer full
 */
esp_err_t storage_manager_write(const log_entry_t *entry);

/**
 * Force flush buffer to SD card
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t storage_manager_flush(void);

/**
 * Get storage statistics
 * @param stats Pointer to stats structure to fill
 */
void storage_manager_get_stats(storage_stats_t *stats);

/**
 * Check if logging is active
 * @return true if logging, false otherwise
 */
bool storage_manager_is_logging(void);

/**
 * Check if storage manager is initialized
 * @return true if initialized, false otherwise
 */
bool storage_manager_is_initialized(void);

/**
 * Get SD card handle for USB MSC integration
 * @return Pointer to SD card structure, NULL if not initialized
 */
sdmmc_card_t* storage_manager_get_card(void);

/**
 * Safe shutdown - prepare for USB MSC or system shutdown
 * Ensures all data is safely written before releasing SD card
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t storage_manager_safe_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif // STORAGE_MANAGER_H
