/**
 * data_processor_types.h
 * 
 * Data types for Data Processor Component (ESP-IDF compatible)
 * Uses types from sensor_manager to avoid conflicts
 */

#ifndef DATA_PROCESSOR_TYPES_H
#define DATA_PROCESSOR_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include "sensor_manager.h"  // Use existing tof_data_t and imu_data_t

#ifdef __cplusplus
extern "C" {
#endif

// =============================================================================
// Synchronized Data Structure
// =============================================================================

/**
 * SyncedData - Synchronized sensor data structure
 * Contains ToF and IMU data synchronized within 10ms window
 */
typedef struct {
    uint32_t timestamp_ms;      // Common timestamp for synchronized data
    tof_data_t tof;             // ToF sensor data (from sensor_manager)
    imu_data_t imu;             // IMU sensor data (from sensor_manager)
    bool synchronized;          // Synchronization success flag
} synced_data_t;

/**
 * FeatureVector - Complete feature set for ML inference and logging
 * Contains raw sensor data (25 features) and derived features (4 features)
 */
typedef struct {
    // Raw sensor data (25 features for ML input)
    uint16_t tof_distances[16]; // ToF 4x4 grid (mm)
    float roll, pitch, yaw;     // Orientation (degrees)
    float accel_x, accel_y, accel_z;  // Linear acceleration (m/s²)
    float gyro_x, gyro_y, gyro_z;     // Angular velocity (deg/s)
    
    // Derived features (4 features)
    uint16_t min_distance_mm;         // Minimum distance across ToF grid
    float approach_rate_mm_s;         // Rate of distance change (mm/s)
    float jerk_magnitude_m_s3;        // Magnitude of acceleration change (m/s³)
    float angular_velocity_deg_s;     // Magnitude of angular velocity (deg/s)
    
    // Event flags
    bool flag_manual;           // Manual flag from button press
    bool flag_auto;             // Automatic flag from ML detection
    float confidence;           // ML confidence score (0.0 to 1.0)
} feature_vector_t;

#ifdef __cplusplus
}
#endif

#endif // DATA_PROCESSOR_TYPES_H
