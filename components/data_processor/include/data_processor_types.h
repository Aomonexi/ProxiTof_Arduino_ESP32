/**
 * data_processor_types.h
 * 
 * Data types for Data Processor Component (ESP-IDF compatible)
 * Pure C types without Arduino dependencies
 */

#ifndef DATA_PROCESSOR_TYPES_H
#define DATA_PROCESSOR_TYPES_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// =============================================================================
// Sensor Data Structures
// =============================================================================

/**
 * ToFData - Time-of-Flight sensor data structure
 * Contains 4x4 distance grid from VL53L8CX sensor
 */
typedef struct {
    uint32_t timestamp_ms;      // Milliseconds since boot
    uint16_t distances[16];     // 4x4 grid in row-major order (mm)
    bool valid;                 // Data validity flag
} tof_data_t;

/**
 * IMUData - Inertial Measurement Unit data structure
 * Contains orientation, linear acceleration, and angular velocity from BNO08x
 */
typedef struct {
    uint32_t timestamp_ms;      // Milliseconds since boot
    float roll, pitch, yaw;     // Orientation in degrees
    float accel_x, accel_y, accel_z;  // Linear acceleration in m/s²
    float gyro_x, gyro_y, gyro_z;     // Angular velocity in deg/s
    bool valid;                 // Data validity flag
} imu_data_t;

/**
 * SyncedData - Synchronized sensor data structure
 * Contains ToF and IMU data synchronized within 10ms window
 */
typedef struct {
    uint32_t timestamp_ms;      // Common timestamp for synchronized data
    tof_data_t tof;             // ToF sensor data
    imu_data_t imu;             // IMU sensor data
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
