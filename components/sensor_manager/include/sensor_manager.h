/**
 * sensor_manager.h
 * 
 * ESP-IDF Sensor Manager Component for ProxiTof Near-Miss Detection System
 * - VL53L8CX ToF sensor (I2C) using RJRP44 library
 * - BNO08x IMU sensor (SPI)
 * - Button input with debouncing (GPIO)
 */

#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "driver/i2c_master.h"

#ifdef __cplusplus
extern "C" {
#endif

// =============================================================================
// Pin Definitions
// =============================================================================

#define TOF_SDA_PIN         GPIO_NUM_8
#define TOF_SCL_PIN         GPIO_NUM_9
#define TOF_LPN_PIN         GPIO_NUM_4

#define IMU_MOSI_PIN        GPIO_NUM_11
#define IMU_MISO_PIN        GPIO_NUM_13
#define IMU_SCK_PIN         GPIO_NUM_12
#define IMU_CS_PIN          GPIO_NUM_10
#define IMU_INT_PIN         GPIO_NUM_5
#define IMU_RST_PIN         GPIO_NUM_6

#define FLAG_BUTTON_PIN     GPIO_NUM_3

// =============================================================================
// Configuration Constants
// =============================================================================

#define BUTTON_DEBOUNCE_MS  50   // Reduced for better responsiveness (was 200ms)
#define I2C_MASTER_FREQ_HZ  100000  // Reduced from 400kHz to 100kHz for stability
#define SPI_MASTER_FREQ_HZ  1000000

// =============================================================================
// Data Structures
// =============================================================================

/**
 * ToF sensor data structure
 * Contains 4x4 distance grid from VL53L8CX sensor
 */
typedef struct {
    uint32_t timestamp_ms;      // Milliseconds since boot
    uint16_t distances[16];     // 4x4 grid in row-major order (mm)
    bool valid;                 // Data validity flag
} tof_data_t;

/**
 * IMU sensor data structure
 * Contains orientation, linear acceleration, and angular velocity
 */
typedef struct {
    uint32_t timestamp_ms;      // Milliseconds since boot
    float roll;                 // Roll angle (degrees)
    float pitch;                // Pitch angle (degrees)
    float yaw;                  // Yaw angle (degrees)
    float accel_x;              // Linear acceleration X (m/s²)
    float accel_y;              // Linear acceleration Y (m/s²)
    float accel_z;              // Linear acceleration Z (m/s²)
    float gyro_x;               // Angular velocity X (deg/s)
    float gyro_y;               // Angular velocity Y (deg/s)
    float gyro_z;               // Angular velocity Z (deg/s)
    bool valid;                 // Data validity flag
} imu_data_t;

/**
 * Sensor health status structure
 */
typedef struct {
    bool tof_healthy;
    bool imu_healthy;
    uint32_t tof_error_count;
    uint32_t imu_error_count;
} sensor_health_t;

// =============================================================================
// Public API Functions
// =============================================================================

/**
 * Initialize the sensor manager and all sensors
 * 
 * Initializes:
 * - I2C bus for ToF sensor
 * - SPI bus for IMU sensor
 * - GPIO for button input
 * - VL53L8CX ToF sensor
 * - BNO08x IMU sensor
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t sensor_manager_init(void);

/**
 * Read ToF sensor data
 * 
 * Reads 4x4 distance grid from VL53L8CX sensor at 60Hz
 * 
 * @param data Pointer to tof_data_t structure to fill
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t sensor_manager_read_tof(tof_data_t *data);

/**
 * Read IMU sensor data
 * 
 * Reads orientation, acceleration, and gyroscope from BNO08x at 100Hz
 * 
 * @param data Pointer to imu_data_t structure to fill
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t sensor_manager_read_imu(imu_data_t *data);

/**
 * Check if button is pressed (with debouncing)
 * 
 * @return true if button press detected, false otherwise
 */
bool sensor_manager_is_button_pressed(void);

/**
 * Get current timestamp in milliseconds since boot
 * 
 * @return Timestamp in milliseconds
 */
uint32_t sensor_manager_get_timestamp(void);

/**
 * Get sensor health status
 * 
 * @param health Pointer to sensor_health_t structure to fill
 */
void sensor_manager_get_health(sensor_health_t *health);

/**
 * Reinitialize IMU sensor
 * 
 * Attempts to reinitialize the BNO08x IMU sensor.
 * Useful when IMU fails to initialize at startup or stops responding.
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t sensor_manager_reinit_imu(void);

/**
 * Get last button press timestamp
 * 
 * @return Timestamp of last button press in milliseconds
 */
uint32_t sensor_manager_get_last_button_press_time(void);

/**
 * Check if button is currently being held down
 * 
 * @return true if button is currently pressed (LOW), false otherwise
 */
bool sensor_manager_is_button_held(void);

/**
 * Get button press duration in milliseconds
 * Returns 0 if button is not currently pressed
 * 
 * @return Duration in milliseconds that button has been held
 */
uint32_t sensor_manager_get_button_duration(void);

/**
 * Get shared I2C bus handle for other components (e.g., DS3231 RTC)
 * 
 * @return I2C master bus handle, or NULL if not initialized
 */
i2c_master_bus_handle_t sensor_manager_get_i2c_bus(void);

#ifdef __cplusplus
}
#endif

#endif // SENSOR_MANAGER_H
