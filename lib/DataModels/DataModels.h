/**
 * DataModels.h
 * 
 * Core data structures for ProxiTof Near-Miss Detection System
 * Defines sensor data structures, feature vectors, configuration, and system state
 */

#ifndef DATA_MODELS_H
#define DATA_MODELS_H

#include <Arduino.h>

// =============================================================================
// Sensor Data Structures
// =============================================================================

/**
 * ToFData - Time-of-Flight sensor data structure
 * Contains 4x4 distance grid from VL53L8CX sensor
 */
struct ToFData {
    uint32_t timestamp_ms;      // Milliseconds since boot
    uint16_t distances[16];     // 4x4 grid in row-major order (mm)
    bool valid;                 // Data validity flag
    
    ToFData() : timestamp_ms(0), valid(false) {
        for (int i = 0; i < 16; i++) {
            distances[i] = 0;
        }
    }
};

/**
 * IMUData - Inertial Measurement Unit data structure
 * Contains orientation, linear acceleration, and angular velocity from BNO08x
 */
struct IMUData {
    uint32_t timestamp_ms;      // Milliseconds since boot
    float roll, pitch, yaw;     // Orientation in degrees
    float accel_x, accel_y, accel_z;  // Linear acceleration in m/s²
    float gyro_x, gyro_y, gyro_z;     // Angular velocity in deg/s
    bool valid;                 // Data validity flag
    
    IMUData() : timestamp_ms(0), roll(0), pitch(0), yaw(0),
                accel_x(0), accel_y(0), accel_z(0),
                gyro_x(0), gyro_y(0), gyro_z(0), valid(false) {}
};

/**
 * SyncedData - Synchronized sensor data structure
 * Contains ToF and IMU data synchronized within 10ms window
 */
struct SyncedData {
    uint32_t timestamp_ms;      // Common timestamp for synchronized data
    ToFData tof;                // ToF sensor data
    IMUData imu;                // IMU sensor data
    bool synchronized;          // Synchronization success flag
    
    SyncedData() : timestamp_ms(0), synchronized(false) {}
};

// =============================================================================
// Feature Vector and Logging Structures
// =============================================================================

/**
 * FeatureVector - Complete feature set for ML inference and logging
 * Contains raw sensor data (25 features) and derived features (4 features)
 */
struct FeatureVector {
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
    
    FeatureVector() : roll(0), pitch(0), yaw(0),
                      accel_x(0), accel_y(0), accel_z(0),
                      gyro_x(0), gyro_y(0), gyro_z(0),
                      min_distance_mm(0), approach_rate_mm_s(0),
                      jerk_magnitude_m_s3(0), angular_velocity_deg_s(0),
                      flag_manual(false), flag_auto(false), confidence(0.0f) {
        for (int i = 0; i < 16; i++) {
            tof_distances[i] = 0;
        }
    }
};

/**
 * LogEntry - Complete CSV log entry structure
 * Contains all sensor data, derived features, and flags for CSV output
 */
struct LogEntry {
    uint32_t timestamp_ms;      // Milliseconds since boot
    
    // ToF sensor data (16 zones)
    uint16_t tof[16];
    
    // IMU sensor data (9 values)
    float roll, pitch, yaw;
    float accel_x, accel_y, accel_z;
    float gyro_x, gyro_y, gyro_z;
    
    // Derived features
    uint16_t min_distance_mm;
    float approach_rate_mm_s;
    float jerk_magnitude_m_s3;
    float angular_velocity_deg_s;
    
    // Flags and ML output
    uint8_t flag_manual;        // 0 or 1
    uint8_t flag_auto;          // 0 or 1
    float confidence;           // 0.00 to 1.00
    
    LogEntry() : timestamp_ms(0), roll(0), pitch(0), yaw(0),
                 accel_x(0), accel_y(0), accel_z(0),
                 gyro_x(0), gyro_y(0), gyro_z(0),
                 min_distance_mm(0), approach_rate_mm_s(0),
                 jerk_magnitude_m_s3(0), angular_velocity_deg_s(0),
                 flag_manual(0), flag_auto(0), confidence(0.0f) {
        for (int i = 0; i < 16; i++) {
            tof[i] = 0;
        }
    }
    
    /**
     * Convert LogEntry to CSV string format
     * Format: timestamp_ms,tof_00,...,tof_33,roll,pitch,yaw,accel_x,accel_y,accel_z,
     *         gyro_x,gyro_y,gyro_z,min_distance_mm,approach_rate_mm_s,
     *         jerk_magnitude_m_s3,angular_velocity_deg_s,flag_manual,flag_auto,confidence
     */
    String toCSV() const {
        String csv = String(timestamp_ms);
        
        // ToF distances (16 zones)
        for (int i = 0; i < 16; i++) {
            csv += "," + String(tof[i]);
        }
        
        // IMU orientation (3 values)
        csv += "," + String(roll, 2);
        csv += "," + String(pitch, 2);
        csv += "," + String(yaw, 2);
        
        // IMU acceleration (3 values)
        csv += "," + String(accel_x, 2);
        csv += "," + String(accel_y, 2);
        csv += "," + String(accel_z, 2);
        
        // IMU gyroscope (3 values)
        csv += "," + String(gyro_x, 2);
        csv += "," + String(gyro_y, 2);
        csv += "," + String(gyro_z, 2);
        
        // Derived features (4 values)
        csv += "," + String(min_distance_mm);
        csv += "," + String(approach_rate_mm_s, 2);
        csv += "," + String(jerk_magnitude_m_s3, 2);
        csv += "," + String(angular_velocity_deg_s, 2);
        
        // Flags and confidence
        csv += "," + String(flag_manual);
        csv += "," + String(flag_auto);
        csv += "," + String(confidence, 2);
        
        return csv;
    }
    
    /**
     * Get CSV header string
     * Returns the column names for CSV file header
     */
    static String getCSVHeader() {
        String header = "timestamp_ms";
        
        // ToF column names (tof_00 to tof_33 in row-major order)
        for (int row = 0; row < 4; row++) {
            for (int col = 0; col < 4; col++) {
                header += ",tof_" + String(row) + String(col);
            }
        }
        
        // IMU column names
        header += ",roll,pitch,yaw";
        header += ",accel_x,accel_y,accel_z";
        header += ",gyro_x,gyro_y,gyro_z";
        
        // Derived feature column names
        header += ",min_distance_mm,approach_rate_mm_s";
        header += ",jerk_magnitude_m_s3,angular_velocity_deg_s";
        
        // Flag and confidence column names
        header += ",flag_manual,flag_auto,confidence";
        
        return header;
    }
};

// =============================================================================
// Configuration Structure
// =============================================================================

/**
 * SystemConfig - Runtime configuration parameters
 * Can be loaded from config.txt on SD card or use defaults
 */
struct SystemConfig {
    uint16_t log_interval_ms;           // Logging interval (~60Hz = 16ms)
    uint16_t flag_window_ms;            // Time window for flag marking
    float ml_threshold;                 // ML confidence threshold for alerts
    float approach_threshold_mm_s;      // Approach rate threshold for near-miss
    float jerk_threshold_m_s3;          // Jerk magnitude threshold for near-miss
    
    // Constructor with default values
    SystemConfig() {
        applyDefaults();
    }
    
    /**
     * Apply default configuration values
     */
    void applyDefaults() {
        log_interval_ms = 16;           // ~60Hz
        flag_window_ms = 2000;          // 2 seconds
        ml_threshold = 0.70f;           // 70% confidence
        approach_threshold_mm_s = 500.0f;   // 500 mm/s
        jerk_threshold_m_s3 = 2.0f;     // 2 m/s³
    }
    
    /**
     * Load configuration from SD card file
     * @param filename Path to config file (e.g., "/config.txt")
     * @return true if loaded successfully, false otherwise
     */
    bool loadFromSD(const char* filename);
    bool parseFromString(const String& content);
    bool setParameter(const String& key, const String& value);
    bool validateParameters() const;
    
    /**
     * Print current configuration to serial
     */
    void printConfig() const {
        Serial.println("\n=== System Configuration ===");
        Serial.print("Log Interval: "); Serial.print(log_interval_ms); Serial.println(" ms");
        Serial.print("Flag Window: "); Serial.print(flag_window_ms); Serial.println(" ms");
        Serial.print("ML Threshold: "); Serial.println(ml_threshold, 2);
        Serial.print("Approach Threshold: "); Serial.print(approach_threshold_mm_s); Serial.println(" mm/s");
        Serial.print("Jerk Threshold: "); Serial.print(jerk_threshold_m_s3); Serial.println(" m/s³");
        Serial.println("============================\n");
    }
    
private:
    String trim(const String& str) const;
};

// =============================================================================
// System State Enumerations
// =============================================================================

/**
 * SystemState - Overall system operational state
 */
enum class SystemState {
    INIT,              // Initializing sensors and SD card
    READY,             // Sensors ready, waiting to start logging
    LOGGING,           // Active data collection
    LOGGING_WITH_ML,   // Data collection + ML inference
    ERROR_SD,          // SD card error, sensors only
    ERROR_SENSOR       // Sensor error, cannot operate
};

/**
 * LEDState - Status LED visual feedback states
 */
enum class LEDState {
    INITIALIZING,       // Blue solid - system starting up
    SD_ERROR,           // Red blinking 2Hz - SD card error
    LOGGING_ACTIVE,     // Green breathing - data logging active
    BUTTON_PRESSED,     // Yellow flash 500ms - button press feedback
    ML_NEAR_MISS,       // Orange alert 1s - ML detected near-miss
    CRITICAL_PROXIMITY, // Red solid - critical proximity alert (highest priority)
    BUFFER_WARNING      // Purple tint overlay - buffer near full
};

/**
 * Convert SystemState to string for debugging
 */
inline const char* systemStateToString(SystemState state) {
    switch (state) {
        case SystemState::INIT: return "INIT";
        case SystemState::READY: return "READY";
        case SystemState::LOGGING: return "LOGGING";
        case SystemState::LOGGING_WITH_ML: return "LOGGING_WITH_ML";
        case SystemState::ERROR_SD: return "ERROR_SD";
        case SystemState::ERROR_SENSOR: return "ERROR_SENSOR";
        default: return "UNKNOWN";
    }
}

/**
 * Convert LEDState to string for debugging
 */
inline const char* ledStateToString(LEDState state) {
    switch (state) {
        case LEDState::INITIALIZING: return "INITIALIZING";
        case LEDState::SD_ERROR: return "SD_ERROR";
        case LEDState::LOGGING_ACTIVE: return "LOGGING_ACTIVE";
        case LEDState::BUTTON_PRESSED: return "BUTTON_PRESSED";
        case LEDState::ML_NEAR_MISS: return "ML_NEAR_MISS";
        case LEDState::CRITICAL_PROXIMITY: return "CRITICAL_PROXIMITY";
        case LEDState::BUFFER_WARNING: return "BUFFER_WARNING";
        default: return "UNKNOWN";
    }
}

#endif // DATA_MODELS_H
