/**
 * data_processor_c_wrapper.h
 * 
 * C wrapper for C++ DataProcessor class
 * Allows C code to use the DataProcessor functionality
 */

#ifndef DATA_PROCESSOR_C_WRAPPER_H
#define DATA_PROCESSOR_C_WRAPPER_H

#include "data_processor_types.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Opaque handle to DataProcessor object
typedef struct data_processor_handle data_processor_handle_t;

/**
 * Create a new DataProcessor instance
 * @return Handle to DataProcessor, NULL on failure
 */
data_processor_handle_t* data_processor_create(void);

/**
 * Destroy DataProcessor instance
 * @param handle DataProcessor handle
 */
void data_processor_destroy(data_processor_handle_t *handle);

/**
 * Set synchronization window
 * @param handle DataProcessor handle
 * @param window_ms Sync window in milliseconds
 */
void data_processor_set_sync_window(data_processor_handle_t *handle, uint32_t window_ms);

/**
 * Get synchronization window
 * @param handle DataProcessor handle
 * @return Sync window in milliseconds
 */
uint32_t data_processor_get_sync_window(data_processor_handle_t *handle);

/**
 * Synchronize ToF and IMU data
 * @param handle DataProcessor handle
 * @param tof ToF sensor data
 * @param imu IMU sensor data
 * @param output Synchronized output data
 * @return true if successful, false otherwise
 */
bool data_processor_synchronize_data(data_processor_handle_t *handle,
                                     const tof_data_t *tof,
                                     const imu_data_t *imu,
                                     synced_data_t *output);

/**
 * Extract features from synchronized data
 * @param handle DataProcessor handle
 * @param data Synchronized sensor data
 * @param features Output feature vector
 */
void data_processor_extract_features(data_processor_handle_t *handle,
                                     const synced_data_t *data,
                                     feature_vector_t *features);

/**
 * Detect near-miss conditions
 * @param handle DataProcessor handle
 * @param features Feature vector to analyze
 * @return true if near-miss detected, false otherwise
 */
bool data_processor_detect_near_miss(data_processor_handle_t *handle,
                                     const feature_vector_t *features);

#ifdef __cplusplus
}
#endif

#endif // DATA_PROCESSOR_C_WRAPPER_H

