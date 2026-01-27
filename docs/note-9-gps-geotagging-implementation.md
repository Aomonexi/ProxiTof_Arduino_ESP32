# Note 9: GPS Geo-Tagging Implementation for Near-Miss Events

**Date**: January 27, 2026  
**Status**: 🟡 GPS Module Ready, Geo-Tagging Pending Implementation  
**Priority**: High (Essential for AI/ML training and location analysis)

---

## Overview

This document describes the GPS geo-tagging system for near-miss events in the ProxiTof device. GPS coordinates are captured and logged with each near-miss event to enable:

1. **Location-based analysis** - Identify dangerous areas
2. **Speed correlation** - Analyze speed vs. near-miss frequency
3. **Route mapping** - Visualize near-miss locations on maps
4. **AI/ML training** - Provide spatial context for neural network training
5. **Environmental correlation** - Link events to road conditions, weather, etc.

---

## Current Implementation Status

### ✅ Completed

1. **GPS Module Integration**
   - Neo-M9N GPS module driver implemented
   - UART communication (9600 baud, GPIO 44/43)
   - NMEA sentence parsing
   - Real-time location tracking
   - Satellite fix detection

2. **GPS Display Mode (Mode 6)**
   - Real-time GPS status display
   - Latitude/Longitude (6 decimal places)
   - Altitude, speed, heading
   - Satellite count and HDOP
   - GPS reconnect button (ACTION)

3. **GPS Initialization**
   - Automatic initialization at startup
   - Non-blocking operation
   - Error handling and retry logic
   - Timeout protection (doesn't hang)

### 🟡 Pending Implementation

1. **Geo-Tagging Integration**
   - Add GPS coordinates to near-miss event logs
   - Synchronize GPS data with near-miss detection
   - Handle GPS unavailable scenarios
   - Add GPS metadata to CSV log files

2. **Data Structure Updates**
   - Extend `log_entry_t` to include GPS fields
   - Add GPS validity flags
   - Include satellite count and accuracy

3. **AI/ML Preparation**
   - Format GPS data for neural network input
   - Create location-based feature vectors
   - Prepare for ESP-NN integration

---

## GPS Hardware Specifications

### Module: Neo-M9N (u-blox)

**Key Features:**
- **Accuracy:** 1.5m CEP (Circular Error Probable)
- **Update Rate:** 1-5 Hz (configurable)
- **Time to First Fix:** 
  - Cold start: 24 seconds
  - Hot start: 2 seconds
- **Sensitivity:** -167 dBm (tracking)
- **Channels:** 92 (concurrent)
- **Constellations:** GPS, GLONASS, Galileo, BeiDou

**Wiring:**
```
ESP32 GPIO 44 (TX) → GPS RX  (Crossover!)
ESP32 GPIO 43 (RX) ← GPS TX  (Crossover!)
GPS VCC → 5V
GPS GND → GND
```

**⚠️ Important:** UART wiring is crossover (TX→RX, RX→TX). This is correct!

---

## GPS Data Structure

### Current GPS Data (Available)

```c
typedef struct {
    // Position
    double latitude;           // Degrees (±90, positive=North)
    double longitude;          // Degrees (±180, positive=East)
    float altitude_m;          // Meters above sea level
    
    // Velocity
    float speed_kmh;           // Speed over ground (km/h)
    float speed_ms;            // Speed over ground (m/s)
    float heading_deg;         // Course over ground (0-360°)
    
    // Time (UTC from satellites)
    uint8_t hour;              // 0-23
    uint8_t minute;            // 0-59
    uint8_t second;            // 0-59
    uint16_t millisecond;      // 0-999
    
    // Date (UTC from satellites)
    uint8_t day;               // 1-31
    uint8_t month;             // 1-12
    uint16_t year;             // Full year (e.g., 2026)
    
    // Fix quality
    gps_fix_quality_t fix_quality;  // 0-8 (see enum)
    uint8_t satellites_used;        // Number of satellites
    float hdop;                     // Horizontal dilution of precision
    
    // Status
    bool valid;                // True if GPS has valid fix
    uint32_t last_update_ms;   // Timestamp of last update
    
} gps_data_t;
```

### Proposed Log Entry Extension

```c
typedef struct {
    // Existing fields
    uint32_t timestamp_ms;
    uint16_t tof[16];
    float roll, pitch, yaw;
    float accel_x, accel_y, accel_z;
    float gyro_x, gyro_y, gyro_z;
    uint8_t flag_manual;
    uint8_t flag_auto;
    
    // NEW: GPS fields
    double gps_latitude;       // Degrees (0 if no fix)
    double gps_longitude;      // Degrees (0 if no fix)
    float gps_altitude_m;      // Meters (0 if no fix)
    float gps_speed_kmh;       // km/h (0 if no fix)
    float gps_heading_deg;     // Degrees (0 if no fix)
    uint8_t gps_satellites;    // Satellite count (0 if no fix)
    float gps_hdop;            // Accuracy (0 if no fix)
    uint8_t gps_valid;         // 1 if GPS fix, 0 if no fix
    
} log_entry_t;
```

### CSV Log Format (Extended)

**Current Format:**
```csv
timestamp_ms,tof_0,tof_1,...,tof_15,roll,pitch,yaw,accel_x,accel_y,accel_z,gyro_x,gyro_y,gyro_z,flag_manual,flag_auto
```

**Proposed Format:**
```csv
timestamp_ms,tof_0,tof_1,...,tof_15,roll,pitch,yaw,accel_x,accel_y,accel_z,gyro_x,gyro_y,gyro_z,flag_manual,flag_auto,gps_lat,gps_lon,gps_alt,gps_speed,gps_heading,gps_sats,gps_hdop,gps_valid
```

**Example Entry (With GPS Fix):**
```csv
12345,450,523,...,12.3,-5.7,180.0,0.98,0.02,9.81,0.1,0.2,0.3,0,1,1.234567,103.123456,45.2,12.5,135.0,8,1.2,1
```

**Example Entry (No GPS Fix):**
```csv
12345,450,523,...,12.3,-5.7,180.0,0.98,0.02,9.81,0.1,0.2,0.3,0,0,0.0,0.0,0.0,0.0,0.0,0,0.0,0
```

---

## Implementation Plan

### Phase 1: Data Structure Updates (1-2 hours)

**Files to Modify:**
1. `components/storage_manager/include/storage_manager.h`
   - Extend `log_entry_t` structure
   - Add GPS fields

2. `components/storage_manager/storage_manager.c`
   - Update CSV header generation
   - Update CSV row formatting
   - Add GPS data serialization

**Code Changes:**

```c
// In storage_manager.h
typedef struct {
    // ... existing fields ...
    
    // GPS fields (added)
    double gps_latitude;
    double gps_longitude;
    float gps_altitude_m;
    float gps_speed_kmh;
    float gps_heading_deg;
    uint8_t gps_satellites;
    float gps_hdop;
    uint8_t gps_valid;
} log_entry_t;

// In storage_manager.c - CSV header
const char *header = 
    "timestamp_ms,tof_0,tof_1,tof_2,tof_3,tof_4,tof_5,tof_6,tof_7,"
    "tof_8,tof_9,tof_10,tof_11,tof_12,tof_13,tof_14,tof_15,"
    "roll,pitch,yaw,accel_x,accel_y,accel_z,gyro_x,gyro_y,gyro_z,"
    "flag_manual,flag_auto,"
    "gps_lat,gps_lon,gps_alt,gps_speed,gps_heading,gps_sats,gps_hdop,gps_valid\n";

// In storage_manager.c - CSV row formatting
snprintf(line, sizeof(line),
    "%lu,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,"
    "%.2f,%.2f,%.2f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%u,%u,"
    "%.6f,%.6f,%.1f,%.1f,%.0f,%u,%.1f,%u\n",
    entry->timestamp_ms,
    // ... ToF data ...
    // ... IMU data ...
    entry->flag_manual, entry->flag_auto,
    entry->gps_latitude, entry->gps_longitude, entry->gps_altitude_m,
    entry->gps_speed_kmh, entry->gps_heading_deg,
    entry->gps_satellites, entry->gps_hdop, entry->gps_valid);
```

### Phase 2: GPS Data Integration (2-3 hours)

**Files to Modify:**
1. `main/main.c`
   - Read GPS data in main loop
   - Add GPS data to log entries
   - Handle GPS unavailable scenarios

**Code Changes:**

```c
// In main.c - main loop
while (1) {
    // ... existing sensor reading ...
    
    // Read GPS data (non-blocking)
    gps_data_t gps_data;
    esp_err_t gps_err = gps_read(&gps_data);
    
    // ... near-miss detection ...
    
    // Data logging with GPS
    if (storage_manager_is_logging() && tof_count > 0 && imu_count > 0) {
        log_entry_t log_entry = {
            .timestamp_ms = current_time,
            .flag_manual = sensor_manager_is_button_held() ? 1 : 0,
            .flag_auto = near_miss_detected ? 1 : 0,
            
            // ... ToF and IMU data ...
            
            // GPS data (if available)
            .gps_latitude = (gps_err == ESP_OK && gps_data.valid) ? gps_data.latitude : 0.0,
            .gps_longitude = (gps_err == ESP_OK && gps_data.valid) ? gps_data.longitude : 0.0,
            .gps_altitude_m = (gps_err == ESP_OK && gps_data.valid) ? gps_data.altitude_m : 0.0f,
            .gps_speed_kmh = (gps_err == ESP_OK && gps_data.valid) ? gps_data.speed_kmh : 0.0f,
            .gps_heading_deg = (gps_err == ESP_OK && gps_data.valid) ? gps_data.heading_deg : 0.0f,
            .gps_satellites = (gps_err == ESP_OK && gps_data.valid) ? gps_data.satellites_used : 0,
            .gps_hdop = (gps_err == ESP_OK && gps_data.valid) ? gps_data.hdop : 0.0f,
            .gps_valid = (gps_err == ESP_OK && gps_data.valid) ? 1 : 0,
        };
        
        storage_manager_write(&log_entry);
    }
}
```

### Phase 3: Testing & Validation (1-2 hours)

**Test Cases:**

1. **GPS Available:**
   - Verify GPS coordinates in CSV logs
   - Check latitude/longitude precision (6 decimals)
   - Verify speed and heading updates
   - Confirm satellite count is logged

2. **GPS Unavailable:**
   - Verify zeros in GPS fields
   - Confirm `gps_valid=0` flag
   - Ensure logging continues normally

3. **GPS Intermittent:**
   - Test GPS loss during logging
   - Verify graceful handling
   - Check data consistency

4. **Near-Miss with GPS:**
   - Trigger near-miss event
   - Verify GPS coordinates are logged
   - Check flag_auto and GPS data correlation

### Phase 4: Documentation (30 minutes)

**Documents to Update:**
1. `docs/HOW_THE_CODE_WORKS.md` - Add GPS geo-tagging section
2. `docs/CONFIG_FILE_GUIDE.md` - Add GPS configuration options
3. `README.md` - Update features list
4. Create data analysis examples with GPS

---

## Use Cases

### 1. Near-Miss Location Mapping

**Scenario:** Cyclist wants to identify dangerous intersections

**Data Analysis:**
```python
import pandas as pd
import folium  # Map visualization

# Load near-miss events
df = pd.read_csv('log20260127.csv')
near_miss = df[df['flag_auto'] == 1]

# Filter valid GPS data
near_miss_gps = near_miss[near_miss['gps_valid'] == 1]

# Create map
map = folium.Map(location=[near_miss_gps['gps_lat'].mean(), 
                           near_miss_gps['gps_lon'].mean()],
                 zoom_start=13)

# Add markers for each near-miss
for idx, row in near_miss_gps.iterrows():
    folium.Marker(
        [row['gps_lat'], row['gps_lon']],
        popup=f"Speed: {row['gps_speed']} km/h<br>Time: {row['timestamp_ms']}",
        icon=folium.Icon(color='red')
    ).add_to(map)

map.save('near_miss_map.html')
```

### 2. Speed vs. Near-Miss Correlation

**Scenario:** Analyze if higher speeds correlate with more near-misses

**Data Analysis:**
```python
import matplotlib.pyplot as plt

# Load data
df = pd.read_csv('log20260127.csv')
near_miss = df[df['flag_auto'] == 1]
near_miss_gps = near_miss[near_miss['gps_valid'] == 1]

# Plot speed distribution
plt.hist(near_miss_gps['gps_speed'], bins=20)
plt.xlabel('Speed (km/h)')
plt.ylabel('Near-Miss Count')
plt.title('Near-Miss Events by Speed')
plt.show()

# Calculate statistics
print(f"Average speed during near-miss: {near_miss_gps['gps_speed'].mean():.1f} km/h")
print(f"Max speed during near-miss: {near_miss_gps['gps_speed'].max():.1f} km/h")
```

### 3. Route Safety Analysis

**Scenario:** Compare safety of different routes

**Data Analysis:**
```python
from sklearn.cluster import DBSCAN
import numpy as np

# Load data
df = pd.read_csv('log20260127.csv')
near_miss_gps = df[(df['flag_auto'] == 1) & (df['gps_valid'] == 1)]

# Cluster near-miss locations
coords = near_miss_gps[['gps_lat', 'gps_lon']].values
clustering = DBSCAN(eps=0.001, min_samples=2).fit(coords)

# Find hotspots (clusters with multiple events)
labels = clustering.labels_
n_clusters = len(set(labels)) - (1 if -1 in labels else 0)

print(f"Found {n_clusters} near-miss hotspots")

# Identify most dangerous cluster
for cluster_id in range(n_clusters):
    cluster_points = coords[labels == cluster_id]
    center = cluster_points.mean(axis=0)
    count = len(cluster_points)
    print(f"Hotspot {cluster_id}: {count} events at ({center[0]:.6f}, {center[1]:.6f})")
```

---

## AI/ML Integration

### Feature Vector with GPS

**Current Features (No GPS):**
- Min distance (16 values)
- Approach rate
- Jerk magnitude
- Angular velocity
- Acceleration (3 axes)
- Gyroscope (3 axes)
- Orientation (roll, pitch, yaw)

**Enhanced Features (With GPS):**
- **All current features** +
- Latitude (normalized)
- Longitude (normalized)
- Altitude (normalized)
- Speed (normalized)
- Heading (sin/cos encoded)
- Satellite count (quality indicator)
- HDOP (accuracy indicator)

### ESP-NN Input Format

```c
// Feature vector for neural network
typedef struct {
    // Sensor features (existing)
    float min_distance_norm;      // 0-1 (0-4000mm → 0-1)
    float approach_rate_norm;     // -1 to 1 (-2000 to +2000 mm/s)
    float jerk_magnitude_norm;    // 0-1 (0-50 m/s³)
    float angular_velocity_norm;  // 0-1 (0-360 deg/s)
    float accel_x_norm;           // -1 to 1 (-20 to +20 m/s²)
    float accel_y_norm;
    float accel_z_norm;
    float gyro_x_norm;            // -1 to 1 (-500 to +500 deg/s)
    float gyro_y_norm;
    float gyro_z_norm;
    float roll_norm;              // -1 to 1 (-180 to +180 deg)
    float pitch_norm;
    float yaw_norm;
    
    // GPS features (new)
    float gps_speed_norm;         // 0-1 (0-100 km/h)
    float gps_heading_sin;        // -1 to 1 (sin of heading)
    float gps_heading_cos;        // -1 to 1 (cos of heading)
    float gps_altitude_norm;      // 0-1 (0-1000m)
    float gps_quality_norm;       // 0-1 (based on HDOP and sat count)
    
    // Location encoding (optional - for location-specific models)
    float gps_lat_norm;           // -1 to 1 (local area only)
    float gps_lon_norm;           // -1 to 1 (local area only)
    
} nn_feature_vector_t;
```

### Benefits for AI/ML

1. **Spatial Context:**
   - Model can learn location-specific patterns
   - Identify dangerous areas automatically
   - Adapt to different environments

2. **Speed Correlation:**
   - Learn speed-dependent risk factors
   - Adjust thresholds based on velocity
   - Predict risk at different speeds

3. **Heading Information:**
   - Directional risk assessment
   - Turn vs. straight-line analysis
   - Intersection approach detection

4. **Data Quality:**
   - GPS quality indicators help filter noisy data
   - Satellite count indicates reliability
   - HDOP provides accuracy metric

---

## Performance Considerations

### GPS Update Rate

**Current:** 1-5 Hz (200-1000ms between updates)  
**Sensor Rate:** 60 Hz (ToF), 100 Hz (IMU)  
**Logging Rate:** 60 Hz

**Strategy:**
- Cache latest GPS data
- Use same GPS coordinates for multiple log entries
- Update GPS data when new fix arrives
- Flag indicates if GPS data is fresh or cached

### Memory Impact

**Additional Memory per Log Entry:**
- 8 GPS fields × 4 bytes average = 32 bytes
- Current entry size: ~100 bytes
- New entry size: ~132 bytes
- **Impact:** +32% memory per entry

**Mitigation:**
- GPS data compresses well (many zeros when no fix)
- Consider reducing GPS precision if needed
- Use binary format for storage (future optimization)

### CPU Impact

**GPS Reading:** < 1ms (non-blocking)  
**GPS Data Copy:** < 0.1ms  
**CSV Formatting:** +0.5ms (8 additional fields)  
**Total Impact:** < 1% CPU overhead

---

## Future Enhancements

### 1. Geo-Fencing

**Feature:** Define safe/dangerous zones
- Alert when entering dangerous area
- Adjust sensitivity based on location
- Log zone transitions

### 2. Route Recording

**Feature:** Record complete routes
- Save GPS track separately
- Correlate near-misses with route
- Replay routes with events

### 3. Cloud Sync

**Feature:** Upload GPS-tagged events to cloud
- Crowdsourced danger map
- Community safety alerts
- Real-time hazard warnings

### 4. Offline Maps

**Feature:** Pre-loaded map tiles
- Display location on OLED
- Show nearby hazards
- Navigate to safe routes

---

## Testing Checklist

### GPS Functionality
- [ ] GPS initializes at startup
- [ ] GPS acquires fix within 5 minutes outdoors
- [ ] GPS data updates at 1-5 Hz
- [ ] GPS reconnect button works
- [ ] GPS mode displays correct data

### Geo-Tagging
- [ ] GPS coordinates logged with near-miss events
- [ ] GPS fields are zero when no fix
- [ ] gps_valid flag is correct
- [ ] CSV format is correct
- [ ] Data can be imported to analysis tools

### Performance
- [ ] No lag in main loop
- [ ] Logging rate unchanged
- [ ] Memory usage acceptable
- [ ] SD card write speed sufficient

### Data Quality
- [ ] Latitude/longitude precision is 6 decimals
- [ ] Speed matches actual movement
- [ ] Heading updates correctly
- [ ] Satellite count is reasonable
- [ ] HDOP indicates good accuracy

---

## Conclusion

GPS geo-tagging is a critical feature for the ProxiTof system, enabling:
- Location-based analysis and mapping
- Speed correlation studies
- AI/ML training with spatial context
- Route safety assessment
- Community safety data sharing

**Current Status:** GPS module is fully functional and ready for geo-tagging integration.

**Next Steps:**
1. Implement data structure updates (Phase 1)
2. Integrate GPS data into logging (Phase 2)
3. Test and validate (Phase 3)
4. Update documentation (Phase 4)

**Estimated Time:** 4-6 hours total implementation time

---

**Document Version:** 1.0  
**Last Updated:** January 27, 2026  
**Status:** 🟡 Ready for Implementation  
**Priority:** High
