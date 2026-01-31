#ifndef ML_INFERENCE_H
#define ML_INFERENCE_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize ML inference engine with ESP-NN acceleration
 * 
 * ESP-NN provides SIMD acceleration for:
 * - FullyConnected layers (matrix multiply)
 * - ReLU activation
 * 
 * Expected performance on ESP32-S3 @ 240MHz:
 * - Inference time: 5-15ms
 * - Memory usage: ~8KB tensor arena
 * 
 * @return true if successful (or no model available), false on error
 */
bool ml_inference_init(void);

/**
 * Run inference on sensor features
 * 
 * Features must be normalized using scaler parameters from training:
 *   normalized[i] = (raw[i] - mean[i]) / std[i]
 * 
 * Feature order (25 total):
 *   [0-15]:  ToF distances (tof_00 to tof_33, 4x4 grid)
 *   [16-18]: Orientation (roll, pitch, yaw)
 *   [19-21]: Acceleration (accel_x, accel_y, accel_z)
 *   [22-24]: Gyroscope (gyro_x, gyro_y, gyro_z)
 * 
 * @param features Array of 25 normalized features
 * @return Probability of near-miss (0.0 to 1.0), 0.0 if no model
 */
float ml_inference_predict(const float features[25]);

/**
 * Check if ML inference is ready (model loaded and initialized)
 * @return true if ready for inference
 */
bool ml_inference_is_ready(void);

/**
 * Get last inference latency in milliseconds
 * @return Inference time in ms (rounded)
 */
uint32_t ml_inference_get_latency_ms(void);

/**
 * Get last inference latency in microseconds (more precise)
 * @return Inference time in µs
 */
uint32_t ml_inference_get_latency_us(void);

/**
 * Get average inference latency in microseconds
 * @return Average inference time in µs
 */
uint32_t ml_inference_get_avg_latency_us(void);

/**
 * Get total inference count
 * @return Number of inferences performed
 */
uint32_t ml_inference_get_count(void);

#ifdef __cplusplus
}
#endif

#endif // ML_INFERENCE_H
