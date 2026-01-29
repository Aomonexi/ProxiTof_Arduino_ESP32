/**
 * data_processor.h
 * 
 * Data Processor Component for ProxiTof Near-Miss Detection System
 * Handles sensor synchronization, feature extraction, and near-miss detection
 * 
 * Requirements: 2.1, 2.4, 2.5, 2.6, 7.1-7.6, 14.1-14.4
 */

#ifndef DATA_PROCESSOR_H
#define DATA_PROCESSOR_H

#include "data_processor_types.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus

/**
 * DataProcessor - Synchronizes sensors, extracts features, and detects near-miss events
 * 
 * This class provides:
 * - Sensor synchronization within 10ms window
 * - IMU interpolation for ToF frame alignment
 * - Timestamp drift detection
 * - Feature extraction (min distance, approach rate, jerk, angular velocity)
 * - Rule-based near-miss detection
 */
class DataProcessor {
public:
    /**
     * Constructor
     * Initializes the data processor with default sync window
     */
    DataProcessor();
    
    /**
     * Destructor
     */
    ~DataProcessor();
    
    // =========================================================================
    // Sensor Synchronization (Requirements 2.1, 2.4, 2.5, 2.6)
    // =========================================================================
    
    /**
     * Synchronize ToF and IMU data within time window
     * 
     * @param tof ToF sensor data
     * @param imu IMU sensor data
     * @param output Synchronized data output
     * @return true if synchronization successful, false otherwise
     * 
     * Property 9: Sensor synchronization within window
     * Property 10: IMU interpolation correctness
     */
    bool synchronizeData(const tof_data_t& tof, const imu_data_t& imu, synced_data_t& output);
    
    /**
     * Set synchronization window in milliseconds
     * Default is 10ms as per requirements
     * 
     * @param window_ms Synchronization window in milliseconds
     */
    void setSyncWindow(uint32_t window_ms);
    
    /**
     * Get current synchronization window
     * 
     * @return Synchronization window in milliseconds
     */
    uint32_t getSyncWindow() const;
    
    /**
     * Check if timestamp drift has been detected
     * 
     * @return true if drift detected, false otherwise
     * 
     * Property 11: Timestamp drift detection
     */
    bool isDriftDetected() const;
    
    /**
     * Get magnitude of detected timestamp drift
     * 
     * @return Drift magnitude in milliseconds
     */
    uint32_t getDriftMagnitude() const;
    
    // =========================================================================
    // Feature Extraction (Requirements 14.1-14.4)
    // =========================================================================
    
    /**
     * Extract all features from synchronized sensor data
     * 
     * @param data Synchronized sensor data
     * @param features Output feature vector
     * 
     * Property 55: Minimum distance calculation
     * Property 56: Approach rate derivation
     * Property 57: Jerk magnitude derivation
     * Property 58: Angular velocity derivation
     */
    void extractFeatures(const synced_data_t& data, feature_vector_t& features);
    
    /**
     * Calculate minimum distance across ToF 4x4 grid
     * 
     * @param distances Array of 16 distance values
     * @return Minimum distance in millimeters
     * 
     * Property 55: Minimum distance calculation
     * Requirement 14.1
     */
    uint16_t calculateMinDistance(const uint16_t distances[16]) const;
    
    /**
     * Calculate approach rate from consecutive ToF frames
     * 
     * @param current_min Current minimum distance
     * @param previous_min Previous minimum distance
     * @param delta_time_ms Time difference in milliseconds
     * @return Approach rate in mm/s (negative = approaching)
     * 
     * Property 27: Approach rate calculation correctness
     * Property 56: Approach rate derivation
     * Requirement 14.2
     */
    float calculateApproachRate(uint16_t current_min, uint16_t previous_min, 
                                uint32_t delta_time_ms) const;
    
    /**
     * Calculate jerk magnitude from consecutive IMU acceleration readings
     * 
     * @param current_accel Current acceleration (x, y, z)
     * @param previous_accel Previous acceleration (x, y, z)
     * @param delta_time_ms Time difference in milliseconds
     * @return Jerk magnitude in m/s³
     * 
     * Property 29: Jerk magnitude calculation correctness
     * Property 57: Jerk magnitude derivation
     * Requirement 14.3
     */
    float calculateJerkMagnitude(float current_accel[3], float previous_accel[3],
                                 uint32_t delta_time_ms) const;
    
    /**
     * Calculate angular velocity magnitude from gyroscope data
     * 
     * @param gyro_x Gyroscope X in deg/s
     * @param gyro_y Gyroscope Y in deg/s
     * @param gyro_z Gyroscope Z in deg/s
     * @return Angular velocity magnitude in deg/s
     * 
     * Property 58: Angular velocity derivation
     * Requirement 14.4
     */
    float calculateAngularVelocity(float gyro_x, float gyro_y, float gyro_z) const;
    
    // =========================================================================
    // Near-Miss Detection (Requirements 7.1-7.6)
    // =========================================================================
    
    /**
     * Detect near-miss conditions using rule-based logic
     * 
     * @param features Feature vector to analyze
     * @return true if near-miss detected, false otherwise
     * 
     * Property 28: Approach rate threshold detection
     * Property 30: Jerk threshold detection
     * Property 31: Combined near-miss detection
     * Property 32: Critical proximity alert
     */
    bool detectNearMiss(const feature_vector_t& features);
    
    /**
     * Check if rapid approach condition is met
     * 
     * @param approach_rate Approach rate in mm/s
     * @param threshold Threshold in mm/s (default 500)
     * @return true if rapid approach detected
     * 
     * Property 28: Approach rate threshold detection
     * Requirement 7.2
     */
    bool isRapidApproach(float approach_rate, float threshold = 500.0f) const;
    
    /**
     * Check if high jerk condition is met
     * 
     * @param jerk_magnitude Jerk magnitude in m/s³
     * @param threshold Threshold in m/s³ (default 2.0)
     * @return true if high jerk detected
     * 
     * Property 30: Jerk threshold detection
     * Requirement 7.4
     */
    bool isHighJerk(float jerk_magnitude, float threshold = 2.0f) const;
    
    /**
     * Check if critical proximity condition is met
     * 
     * @param min_distance Minimum distance in mm
     * @param approach_rate Approach rate in mm/s
     * @return true if critical proximity detected
     * 
     * Property 32: Critical proximity alert
     * Requirement 7.6
     */
    bool isCriticalProximity(uint16_t min_distance, float approach_rate) const;
    
    /**
     * Check if combined near-miss conditions are met within time window
     * 
     * @return true if combined conditions detected
     * 
     * Property 31: Combined near-miss detection
     * Requirement 7.5
     */
    bool isCombinedNearMiss() const;
    
    /**
     * Get timestamp of last rapid approach detection
     * 
     * @return Timestamp in milliseconds
     */
    uint32_t getLastRapidApproachTime() const;
    
    /**
     * Get timestamp of last high jerk detection
     * 
     * @return Timestamp in milliseconds
     */
    uint32_t getLastHighJerkTime() const;
    
private:
    // Synchronization state
    uint32_t syncWindow_ms;         // Synchronization window (default 10ms)
    imu_data_t lastIMU;             // Last IMU reading for interpolation
    tof_data_t lastToF;             // Last ToF reading for rate calculation
    bool driftDetected;             // Timestamp drift detection flag
    uint32_t driftMagnitude;        // Magnitude of detected drift
    
    // Feature extraction state
    uint16_t previousMinDistance;   // Previous minimum distance for approach rate
    float previousAccel[3];         // Previous acceleration for jerk calculation
    uint32_t lastFeatureTime;       // Timestamp of last feature extraction
    
    // Distance filtering (noise reduction)
    static const int DISTANCE_FILTER_SIZE = 5;  // 5-sample moving average
    uint16_t distanceFilterBuffer[DISTANCE_FILTER_SIZE];
    int distanceFilterIndex;
    int distanceFilterCount;
    
    // Near-miss detection state
    uint32_t lastRapidApproachTime; // Timestamp of last rapid approach
    uint32_t lastHighJerkTime;      // Timestamp of last high jerk
    bool rapidApproachActive;       // Rapid approach condition active
    bool highJerkActive;            // High jerk condition active
    
    // Helper methods
    bool isWithinSyncWindow(uint32_t time1, uint32_t time2) const;
    void detectTimestampDrift(uint32_t tof_time, uint32_t imu_time);
    void updateNearMissState(const feature_vector_t& features);
    uint16_t applyDistanceFilter(uint16_t raw_distance);  // Moving average filter
};

#endif // __cplusplus

#endif // DATA_PROCESSOR_H
