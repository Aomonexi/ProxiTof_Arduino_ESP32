#ifndef MODEL_DATA_H
#define MODEL_DATA_H

#include <stdint.h>

/**
 * Model Data Header
 * 
 * This file contains the TFLite model data embedded in firmware.
 * 
 * BEFORE TRAINING:
 *   - model_data_len = 0 (no model)
 *   - ML inference will be disabled
 *   - System uses algorithm-only detection
 * 
 * AFTER TRAINING:
 *   - Run: python tools/quick_start_ml.py
 *   - This file will be regenerated with actual model weights
 *   - Rebuild firmware to include trained model
 * 
 * Model Architecture (after training):
 *   - Input: 25 features (16 ToF + 9 IMU)
 *   - Hidden: 16 neurons (ReLU activation)
 *   - Output: 2 classes (safe, near-miss)
 *   - Total parameters: ~450
 *   - Model size: ~2.4 KB
 */

// Model data length (0 = no model available)
const unsigned int model_data_len = 0;

// Model data array (empty until training)
// After training, this will contain the TFLite flatbuffer
const unsigned char model_data[] = {
    // Placeholder - will be replaced by quick_start_ml.py
    // The training script generates this file with actual model weights
    0x00  // Single byte to prevent empty array warning
};

// Scaler parameters for feature normalization
// These will be populated by quick_start_ml.py after training
// 
// Usage: normalized[i] = (raw[i] - scaler_mean[i]) / scaler_std[i]
//
// Feature order:
//   [0-15]:  ToF distances (tof_00 to tof_33)
//   [16-18]: Orientation (roll, pitch, yaw)
//   [19-21]: Acceleration (accel_x, accel_y, accel_z)
//   [22-24]: Gyroscope (gyro_x, gyro_y, gyro_z)

// Placeholder scaler parameters (will be replaced after training)
const float scaler_mean[25] = {
    // ToF features (16) - placeholder
    500.0f, 500.0f, 500.0f, 500.0f, 500.0f, 500.0f, 500.0f, 500.0f,
    500.0f, 500.0f, 500.0f, 500.0f, 500.0f, 500.0f, 500.0f, 500.0f,
    // IMU features (9) - placeholder
    0.0f, 0.0f, 0.0f,      // roll, pitch, yaw
    0.0f, 0.0f, 9.81f,     // accel_x, accel_y, accel_z
    0.0f, 0.0f, 0.0f       // gyro_x, gyro_y, gyro_z
};

const float scaler_std[25] = {
    // ToF features (16) - placeholder
    200.0f, 200.0f, 200.0f, 200.0f, 200.0f, 200.0f, 200.0f, 200.0f,
    200.0f, 200.0f, 200.0f, 200.0f, 200.0f, 200.0f, 200.0f, 200.0f,
    // IMU features (9) - placeholder
    15.0f, 15.0f, 180.0f,  // roll, pitch, yaw (degrees)
    2.0f, 2.0f, 1.0f,      // accel_x, accel_y, accel_z (m/s²)
    50.0f, 50.0f, 50.0f    // gyro_x, gyro_y, gyro_z (deg/s)
};

#endif // MODEL_DATA_H
