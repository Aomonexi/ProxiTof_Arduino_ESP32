/**
 * data_processor.cpp
 * 
 * Implementation of Data Processor Component
 * 
 * Requirements: 2.1, 2.4, 2.5, 2.6, 7.1-7.6, 14.1-14.4
 */

#include "data_processor.h"
#include <math.h>
#include <string.h>
#include "esp_timer.h"
#include "esp_log.h"

static const char* TAG = "DataProcessor";

// =============================================================================
// Constructor and Destructor
// =============================================================================

DataProcessor::DataProcessor() 
    : syncWindow_ms(10),
      driftDetected(false),
      driftMagnitude(0),
      previousMinDistance(0),
      lastFeatureTime(0),
      distanceFilterIndex(0),
      distanceFilterCount(0),
      lastRapidApproachTime(0),
      lastHighJerkTime(0),
      rapidApproachActive(false),
      highJerkActive(false) {
    
    // Initialize previous acceleration to zero
    previousAccel[0] = 0.0f;
    previousAccel[1] = 0.0f;
    previousAccel[2] = 0.0f;
    
    // Initialize distance filter buffer
    for (int i = 0; i < DISTANCE_FILTER_SIZE; i++) {
        distanceFilterBuffer[i] = 0;
    }
}

DataProcessor::~DataProcessor() {
    // Nothing to clean up
}

// =============================================================================
// Sensor Synchronization (Requirements 2.1, 2.4, 2.5, 2.6)
// =============================================================================

bool DataProcessor::synchronizeData(const tof_data_t& tof, const imu_data_t& imu, synced_data_t& output) {
    // Validate input data
    if (!tof.valid || !imu.valid) {
        output.synchronized = false;
        return false;
    }
    
    // Check if timestamps are within sync window (Requirement 2.4)
    // Property 9: Sensor synchronization within window
    if (!isWithinSyncWindow(tof.timestamp_ms, imu.timestamp_ms)) {
        // Use most recent IMU value for synchronization (Requirement 2.5)
        // Property 10: IMU interpolation correctness
        if (lastIMU.valid) {
            output.timestamp_ms = tof.timestamp_ms;
            output.tof = tof;
            output.imu = lastIMU;
            output.synchronized = true;
            
            // Store current readings for next iteration
            lastToF = tof;
            lastIMU = imu;
            
            return true;
        } else {
            output.synchronized = false;
            return false;
        }
    }
    
    // Timestamps are within window - synchronize with ToF timestamp
    output.timestamp_ms = tof.timestamp_ms;
    output.tof = tof;
    output.imu = imu;
    output.synchronized = true;
    
    // Detect timestamp drift (Requirement 2.6)
    // Property 11: Timestamp drift detection
    detectTimestampDrift(tof.timestamp_ms, imu.timestamp_ms);
    
    // Store current readings for next iteration
    lastToF = tof;
    lastIMU = imu;
    
    return true;
}

void DataProcessor::setSyncWindow(uint32_t window_ms) {
    syncWindow_ms = window_ms;
}

uint32_t DataProcessor::getSyncWindow() const {
    return syncWindow_ms;
}

bool DataProcessor::isDriftDetected() const {
    return driftDetected;
}

uint32_t DataProcessor::getDriftMagnitude() const {
    return driftMagnitude;
}

bool DataProcessor::isWithinSyncWindow(uint32_t time1, uint32_t time2) const {
    uint32_t diff = (time1 > time2) ? (time1 - time2) : (time2 - time1);
    return diff <= syncWindow_ms;
}

void DataProcessor::detectTimestampDrift(uint32_t tof_time, uint32_t imu_time) {
    uint32_t diff = (tof_time > imu_time) ? (tof_time - imu_time) : (imu_time - tof_time);
    
    // Consider drift if difference exceeds sync window
    if (diff > syncWindow_ms) {
        driftDetected = true;
        driftMagnitude = diff;
        
        // Log warning (Requirement 2.6)
        ESP_LOGW(TAG, "Timestamp drift detected: %lu ms", (unsigned long)diff);
    }
}

// =============================================================================
// Feature Extraction (Requirements 14.1-14.4)
// =============================================================================

void DataProcessor::extractFeatures(const synced_data_t& data, feature_vector_t& features) {
    if (!data.synchronized) {
        return;
    }
    
    // Copy raw sensor data to feature vector
    for (int i = 0; i < 16; i++) {
        features.tof_distances[i] = data.tof.distances[i];
    }
    
    features.roll = data.imu.roll;
    features.pitch = data.imu.pitch;
    features.yaw = data.imu.yaw;
    features.accel_x = data.imu.accel_x;
    features.accel_y = data.imu.accel_y;
    features.accel_z = data.imu.accel_z;
    features.gyro_x = data.imu.gyro_x;
    features.gyro_y = data.imu.gyro_y;
    features.gyro_z = data.imu.gyro_z;
    
    // Calculate derived features
    
    // 1. Minimum distance (Requirement 14.1)
    // Property 55: Minimum distance calculation
    uint16_t raw_min_distance = calculateMinDistance(data.tof.distances);
    
    // Apply moving average filter to reduce noise (especially when device is tilted)
    features.min_distance_mm = applyDistanceFilter(raw_min_distance);
    
    // 2. Approach rate (Requirement 14.2)
    // Property 56: Approach rate derivation
    if (lastFeatureTime > 0) {
        uint32_t delta_time = data.timestamp_ms - lastFeatureTime;
        features.approach_rate_mm_s = calculateApproachRate(
            features.min_distance_mm, 
            previousMinDistance, 
            delta_time
        );
    } else {
        features.approach_rate_mm_s = 0.0f;
    }
    
    // 3. Jerk magnitude (Requirement 14.3)
    // Property 57: Jerk magnitude derivation
    if (lastFeatureTime > 0) {
        uint32_t delta_time = data.timestamp_ms - lastFeatureTime;
        float current_accel[3] = {data.imu.accel_x, data.imu.accel_y, data.imu.accel_z};
        features.jerk_magnitude_m_s3 = calculateJerkMagnitude(
            current_accel,
            previousAccel,
            delta_time
        );
    } else {
        features.jerk_magnitude_m_s3 = 0.0f;
    }
    
    // 4. Angular velocity magnitude (Requirement 14.4)
    // Property 58: Angular velocity derivation
    features.angular_velocity_deg_s = calculateAngularVelocity(
        data.imu.gyro_x,
        data.imu.gyro_y,
        data.imu.gyro_z
    );
    
    // Update state for next iteration
    previousMinDistance = features.min_distance_mm;
    previousAccel[0] = data.imu.accel_x;
    previousAccel[1] = data.imu.accel_y;
    previousAccel[2] = data.imu.accel_z;
    lastFeatureTime = data.timestamp_ms;
}

uint16_t DataProcessor::calculateMinDistance(const uint16_t distances[16]) const {
    // Property 55: Minimum distance calculation
    // Requirement 14.1: Calculate minimum distance across ToF 4x4 grid
    // NOTE: 3800mm is the clamped "no target" value — exclude it from min calculation
    
    uint16_t min_dist = 3800;  // Start at max (no-target value)
    for (int i = 0; i < 16; i++) {
        if (distances[i] > 0 && distances[i] < 3800 && distances[i] < min_dist) {
            min_dist = distances[i];
        }
    }
    // If all zones report no target, return 3800 to indicate "clear"
    return min_dist;
}

float DataProcessor::calculateApproachRate(uint16_t current_min, uint16_t previous_min, 
                                           uint32_t delta_time_ms) const {
    // Property 27: Approach rate calculation correctness
    // Property 56: Approach rate derivation
    // Requirement 14.2: Calculate approach rate from consecutive ToF frames
    
    if (delta_time_ms == 0) {
        return 0.0f;
    }
    
    // If either reading is the "no target" sentinel (3800mm), skip calculation.
    // A transition between a real reading and 3800mm is NOT an approach/retreat —
    // it just means the target left or entered the sensor's range.
    // Without this guard, a 300mm → 3800mm transition would compute a false
    // approach rate of ~218,000 mm/s, triggering spurious near-miss alerts.
    if (current_min >= 3800 || previous_min >= 3800) {
        return 0.0f;
    }
    
    // Calculate rate: (current - previous) / time_in_seconds
    // Negative value means approaching (distance decreasing)
    float delta_distance = (float)(current_min - previous_min);
    float delta_time_s = delta_time_ms / 1000.0f;
    
    return delta_distance / delta_time_s;
}

float DataProcessor::calculateJerkMagnitude(float current_accel[3], float previous_accel[3],
                                            uint32_t delta_time_ms) const {
    // Property 29: Jerk magnitude calculation correctness
    // Property 57: Jerk magnitude derivation
    // Requirement 14.3: Calculate jerk magnitude from consecutive IMU acceleration
    
    if (delta_time_ms == 0) {
        return 0.0f;
    }
    
    float delta_time_s = delta_time_ms / 1000.0f;
    
    // Calculate jerk for each axis: (current_accel - previous_accel) / delta_time
    float jerk_x = (current_accel[0] - previous_accel[0]) / delta_time_s;
    float jerk_y = (current_accel[1] - previous_accel[1]) / delta_time_s;
    float jerk_z = (current_accel[2] - previous_accel[2]) / delta_time_s;
    
    // Calculate magnitude: sqrt(jerk_x² + jerk_y² + jerk_z²)
    return sqrt(jerk_x * jerk_x + jerk_y * jerk_y + jerk_z * jerk_z);
}

float DataProcessor::calculateAngularVelocity(float gyro_x, float gyro_y, float gyro_z) const {
    // Property 58: Angular velocity derivation
    // Requirement 14.4: Calculate angular velocity magnitude from gyroscope data
    
    // Calculate magnitude: sqrt(gyro_x² + gyro_y² + gyro_z²)
    return sqrt(gyro_x * gyro_x + gyro_y * gyro_y + gyro_z * gyro_z);
}

// =============================================================================
// Near-Miss Detection (Requirements 7.1-7.6)
// =============================================================================

bool DataProcessor::detectNearMiss(const feature_vector_t& features) {
    // Update near-miss detection state
    updateNearMissState(features);
    
    // =========================================================================
    // VALIDITY CHECK: Filter out invalid ToF readings
    // =========================================================================
    // Distance = 0mm indicates sensor is blocked or facing surface directly
    // Distance < 20mm is too close to be a valid near-miss (sensor blocked)
    // Distance >= 3800mm means no target in range (outdoors, open air)
    // This prevents false positives when device is on table or sensor is covered
    if (features.min_distance_mm < 20 || features.min_distance_mm >= 3800) {
        return false;  // Invalid reading - sensor blocked, too close, or no target
    }
    
    // =========================================================================
    // TIER 1: Critical Proximity (Highest Priority)
    // =========================================================================
    // Immediate danger - very close and fast
    // 20-100mm + >500mm/s approach
    // NO JERK REQUIRED - proximity + speed is enough
    if (isCriticalProximity(features.min_distance_mm, features.approach_rate_mm_s)) {
        return true;
    }
    
    // =========================================================================
    // TIER 2: Proximity Alert (High Priority - NO JERK REQUIRED)
    // =========================================================================
    // Fast approach in near-miss zone (15cm)
    // Scenario: Motorcycle passes at 15cm, cyclist doesn't react (late/no reaction)
    // This IS a near-miss even without body reaction
    // Priority: Fast approach + close distance is sufficient
    // NO JERK REQUIRED - proximity + fast approach is the highest priority
    if (features.min_distance_mm < 150 && isRapidApproach(features.approach_rate_mm_s)) {
        return true;
    }
    
    // =========================================================================
    // TIER 3: Confirmed Near-Miss (Optional Enhancement)
    // =========================================================================
    // Tier 2 + body reaction (jerk)
    // Jerk confirms the near-miss was perceived by cyclist
    // Useful for training data quality and severity classification
    // This is already covered by Tier 2, but we track it for logging
    // (Combined detection with jerk is a subset of Tier 2)
    
    return false;
}

bool DataProcessor::isRapidApproach(float approach_rate, float threshold) const {
    // Property 28: Approach rate threshold detection
    // Requirement 7.2: Detect rapid approach (>1000 mm/s = 1.0 m/s)
    // Updated threshold: More strict to reduce false positives
    
    // Negative approach rate means approaching (distance decreasing)
    // Default threshold: 1000 mm/s (was 500)
    float actual_threshold = (threshold > 0) ? threshold : 1000.0f;
    return approach_rate < -actual_threshold;
}

bool DataProcessor::isHighJerk(float jerk_magnitude, float threshold) const {
    // Property 30: Jerk threshold detection
    // Requirement 7.4: Detect high jerk (>7 m/s³)
    // Updated threshold: High enough to filter out daily vibrations
    // - Turning: gradual, low jerk
    // - Bumpy road: high-frequency, filtered
    // - Walking/pedaling: periodic, < 5 m/s³
    // - Near-miss reaction: sudden, > 7 m/s³
    
    // Default threshold: 7.0 m/s³ (was 5.0)
    float actual_threshold = (threshold > 0) ? threshold : 7.0f;
    return jerk_magnitude > actual_threshold;
}

bool DataProcessor::isCriticalProximity(uint16_t min_distance, float approach_rate) const {
    // Property 32: Critical proximity alert
    // Requirement 7.6: Detect critical proximity (20-100mm with >500 mm/s approach)
    // Updated thresholds: More strict to reduce false positives
    // Minimum 20mm to filter out sensor blockage (0mm readings)
    
    return (min_distance >= 20 && min_distance < 100) && (approach_rate < -500.0f);
}

bool DataProcessor::isCombinedNearMiss() const {
    // Property 31: Combined near-miss detection
    // Requirement 7.5: Detect combined conditions within 500ms window
    
    if (!rapidApproachActive || !highJerkActive) {
        return false;
    }
    
    // Check if both conditions occurred within 500ms of each other
    uint32_t time_diff;
    if (lastRapidApproachTime > lastHighJerkTime) {
        time_diff = lastRapidApproachTime - lastHighJerkTime;
    } else {
        time_diff = lastHighJerkTime - lastRapidApproachTime;
    }
    
    return time_diff <= 500;
}

uint32_t DataProcessor::getLastRapidApproachTime() const {
    return lastRapidApproachTime;
}

uint32_t DataProcessor::getLastHighJerkTime() const {
    return lastHighJerkTime;
}

void DataProcessor::updateNearMissState(const feature_vector_t& features) {
    // Get current time in milliseconds
    uint32_t current_time = (uint32_t)(esp_timer_get_time() / 1000);
    
    // Update rapid approach state
    if (isRapidApproach(features.approach_rate_mm_s)) {
        rapidApproachActive = true;
        lastRapidApproachTime = current_time;
    } else {
        // Clear flag if no rapid approach for 1 second
        if (rapidApproachActive && (current_time - lastRapidApproachTime > 1000)) {
            rapidApproachActive = false;
        }
    }
    
    // Update high jerk state
    if (isHighJerk(features.jerk_magnitude_m_s3)) {
        highJerkActive = true;
        lastHighJerkTime = current_time;
    } else {
        // Clear flag if no high jerk for 1 second
        if (highJerkActive && (current_time - lastHighJerkTime > 1000)) {
            highJerkActive = false;
        }
    }
}

// =============================================================================
// Distance Filtering (Noise Reduction)
// =============================================================================

uint16_t DataProcessor::applyDistanceFilter(uint16_t raw_distance) {
    // Smart adaptive filter:
    // - Filters out small noise (tilt, vibration) with moving average
    // - Preserves large rapid changes (fast-approaching objects like bikes)
    // NOTE: 3800mm is the "no target" sentinel — pass it through unchanged
    //       without updating the filter buffer, so it doesn't corrupt the
    //       moving average for when a real target returns.
    
    if (raw_distance >= 3800) {
        return 3800;  // No target — don't update filter state
    }
    
    // Add new reading to circular buffer
    distanceFilterBuffer[distanceFilterIndex] = raw_distance;
    distanceFilterIndex = (distanceFilterIndex + 1) % DISTANCE_FILTER_SIZE;
    
    // Track how many samples we have (up to DISTANCE_FILTER_SIZE)
    if (distanceFilterCount < DISTANCE_FILTER_SIZE) {
        distanceFilterCount++;
    }
    
    // Calculate moving average (only over valid non-sentinel samples)
    uint32_t sum = 0;
    for (int i = 0; i < distanceFilterCount; i++) {
        sum += distanceFilterBuffer[i];
    }
    uint16_t filtered_distance = sum / distanceFilterCount;
    
    // Calculate change magnitude
    int change = abs((int)raw_distance - (int)filtered_distance);
    
    // ADAPTIVE FILTERING:
    // If change is small (<100mm), use filtered value (reduces tilt noise)
    // If change is large (>100mm), use raw value (preserves fast objects)
    // This ensures we catch bikes passing at 20-30 km/h (5-8 m/s)
    
    if (change > 100) {
        // Large rapid change detected - likely a fast-moving object!
        // Use raw value to preserve the detection
        ESP_LOGD(TAG, "⚡ Fast change detected: %d mm - using RAW value (bike/vehicle?)", change);
        
        // Reset filter buffer to new value to avoid lag
        for (int i = 0; i < DISTANCE_FILTER_SIZE; i++) {
            distanceFilterBuffer[i] = raw_distance;
        }
        
        return raw_distance;
    } else if (change > 20) {
        // Medium change - use weighted average (70% raw, 30% filtered)
        // This balances noise reduction with responsiveness
        uint16_t weighted = (raw_distance * 7 + filtered_distance * 3) / 10;
        ESP_LOGD(TAG, "📊 Medium change: %d mm - weighted average", change);
        return weighted;
    } else {
        // Small change - use filtered value (reduces tilt/vibration noise)
        if (change > 5) {
            ESP_LOGD(TAG, "🔇 Small noise: %d mm - filtered out", change);
        }
        return filtered_distance;
    }
}
