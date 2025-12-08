/**
 * SensorManager.h
 * 
 * Manages all sensor hardware for ProxiTof Near-Miss Detection System
 * - VL53L8CX ToF sensor (I2C)
 * - BNO08x IMU sensor (SPI)
 * - Button input with debouncing
 */

#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <DataModels.h>
#include <vl53l8cx.h>
#include <SparkFun_BNO08x_Arduino_Library.h>

// Pin Definitions
#define TOF_SDA_PIN 8
#define TOF_SCL_PIN 9
#define TOF_LPN_PIN 4

#define IMU_MOSI_PIN 11
#define IMU_MISO_PIN 13
#define IMU_SCK_PIN 12
#define IMU_CS_PIN 10
#define IMU_INT_PIN 5
#define IMU_RST_PIN 6

#define FLAG_BUTTON_PIN 3

// Debounce configuration
#define BUTTON_DEBOUNCE_MS 50

/**
 * SensorManager - Unified interface for all sensor hardware
 * 
 * Responsibilities:
 * - Initialize and configure ToF and IMU sensors
 * - Poll sensors at appropriate rates (ToF: 60Hz, IMU: 100Hz)
 * - Handle sensor errors and provide graceful degradation
 * - Manage button input with debouncing
 * - Generate consistent timestamps using millis()
 */
class SensorManager {
public:
    SensorManager();
    ~SensorManager();
    
    /**
     * Initialize all sensors
     * @return true if at least one sensor initialized successfully
     */
    bool initialize();
    
    /**
     * Read ToF sensor data
     * @param data Output ToFData structure
     * @return true if data read successfully
     */
    bool readToFData(ToFData& data);
    
    /**
     * Read IMU sensor data
     * @param data Output IMUData structure
     * @return true if data read successfully
     */
    bool readIMUData(IMUData& data);
    
    /**
     * Check if button is pressed (with debouncing)
     * @return true if button press detected
     */
    bool isButtonPressed();
    
    /**
     * Get current timestamp in milliseconds since boot
     * @return Timestamp in milliseconds
     */
    uint32_t getTimestamp();
    
    /**
     * Get sensor health status
     */
    bool isToFHealthy() const { return tofHealthy; }
    bool isIMUHealthy() const { return imuHealthy; }
    
    /**
     * Get last button press timestamp
     * @return Timestamp of last button press in milliseconds
     */
    uint32_t getLastButtonPressTime() const { return lastButtonPressTime; }
    
private:
    // Sensor objects
    VL53L8CX *tofSensor;
    BNO08x imuSensor;
    
    // Sensor health flags
    bool tofHealthy;
    bool imuHealthy;
    
    // Button state
    uint32_t lastButtonPressTime;
    uint32_t lastButtonReadTime;
    bool lastButtonState;
    
    // Helper methods
    bool initializeToF();
    bool initializeIMU();
    bool initializeButton();
    
    /**
     * Log error message to serial
     * @param component Component name (e.g., "TOF_SENSOR", "IMU_SENSOR")
     * @param errorCode Error code
     * @param description Error description
     */
    void logError(const char* component, uint8_t errorCode, const char* description);
    
    /**
     * Log warning message to serial
     * @param component Component name
     * @param warningCode Warning code
     * @param description Warning description
     */
    void logWarning(const char* component, uint8_t warningCode, const char* description);
};

#endif // SENSOR_MANAGER_H
