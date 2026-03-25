# GPS Data Strategy - Metadata vs Training Features

**Date**: January 27, 2026  
**Purpose**: Explain how GPS data is used for analysis without affecting ESP-NN training

---

## Overview

GPS data is **metadata only** - it's used for post-analysis and visualization, but **NOT** included in ESP-NN training features. This keeps the neural network focused on sensor patterns while allowing location-based analysis.

---

## Current Implementation Status

### Log Entry Structure

GPS fields are **defined** in `log_entry_t` but **not yet populated**:

```c
typedef struct {
    uint32_t timestamp_ms;
    
    // ToF data (16 zones) - USED FOR TRAINING ✅
    uint16_t tof[16];
    
    // IMU data (9 values) - USED FOR TRAINING ✅
    float roll, pitch, yaw;
    float accel_x, accel_y, accel_z;
    float gyro_x, gyro_y, gyro_z;
    
    // GPS data (location tracking) - METADATA ONLY ℹ️
    double latitude;           // Degrees (0.0 if no GPS fix)
    double longitude;          // Degrees (0.0 if no GPS fix)
    float speed_ms;            // Speed in m/s (0.0 if no GPS fix)
    float heading_deg;         // Heading in degrees (0.0 if no GPS fix)
    uint8_t gps_satellites;    // Number of satellites (0 if no GPS fix)
    
    // Flags - METADATA ONLY ℹ️
    uint8_t flag_manual;       // Manual flag (button press)
    uint8_t flag_auto;         // Automatic flag (algorithm detection)
    
} log_entry_t;
```

### Current Status

**✅ Defined**: GPS fields exist in structure  
**❌ Not Populated**: GPS data not yet written to logs  
**⏳ Future**: Will be added when GPS geo-tagging is implemented

---

## Data Separation Strategy

### Training Features (ESP-NN Input)

**What Goes Into Neural Network:**
- ✅ ToF distances (16 values)
- ✅ IMU orientation (roll, pitch, yaw)
- ✅ IMU acceleration (accel_x, accel_y, accel_z)
- ✅ IMU gyroscope (gyro_x, gyro_y, gyro_z)

**Total**: 28 features per sample

**Why These?**
- Direct sensor measurements
- Capture object proximity and motion
- Independent of location
- Generalizable across different areas

---

### Metadata (Analysis Only)

**What Stays as Metadata:**
- ℹ️ GPS latitude/longitude
- ℹ️ GPS speed
- ℹ️ GPS heading
- ℹ️ GPS satellite count
- ℹ️ Timestamp
- ℹ️ Manual flags
- ℹ️ Auto flags

**Why Metadata?**
- Location-specific (not generalizable)
- Used for post-analysis
- Helps identify high-risk areas
- Enables route mapping
- Correlates events with locations

---

## Use Cases

### 1. Training ESP-NN (Sensor Data Only)

**Input Features:**
```python
# Extract only sensor data for training
features = [
    tof[0], tof[1], ..., tof[15],      # 16 ToF values
    roll, pitch, yaw,                   # 3 orientation values
    accel_x, accel_y, accel_z,         # 3 acceleration values
    gyro_x, gyro_y, gyro_z             # 3 gyroscope values
]
# Total: 28 features
```

**Output:**
```python
label = 1 if (flag_manual or flag_auto) else 0  # Near-miss or not
```

**GPS is NOT used** - model learns sensor patterns only

---

### 2. Post-Analysis (Metadata)

**Location Heatmap:**
```python
# Group near-miss events by location
near_miss_locations = []
for entry in log_data:
    if entry.flag_auto or entry.flag_manual:
        near_miss_locations.append({
            'lat': entry.latitude,
            'lon': entry.longitude,
            'timestamp': entry.timestamp_ms,
            'severity': calculate_severity(entry)
        })

# Plot on map
plot_heatmap(near_miss_locations)
```

**Route Safety Analysis:**
```python
# Analyze which routes have most near-misses
routes = cluster_by_location(near_miss_locations)
for route in routes:
    print(f"Route: {route.name}")
    print(f"Near-misses: {route.count}")
    print(f"Risk level: {route.risk_level}")
```

---

## Implementation Plan

### Phase 1: GPS Logging (Not Yet Implemented)

**File**: `main/main.c`

**Add GPS data to log entries:**
```c
// Inside main loop, when writing log entry
if (storage_manager_is_logging() && tof_count > 0 && imu_count > 0) {
    log_entry_t log_entry = {
        .timestamp_ms = current_time,
        .flag_manual = sensor_manager_is_button_held() ? 1 : 0,
        .flag_auto = near_miss_active ? 1 : 0
    };
    
    // Copy ToF data
    for (int i = 0; i < 16; i++) {
        log_entry.tof[i] = tof_data.distances[i];
    }
    
    // Copy IMU data
    log_entry.roll = imu_data.roll;
    log_entry.pitch = imu_data.pitch;
    log_entry.yaw = imu_data.yaw;
    log_entry.accel_x = imu_data.accel_x;
    log_entry.accel_y = imu_data.accel_y;
    log_entry.accel_z = imu_data.accel_z;
    log_entry.gyro_x = imu_data.gyro_x;
    log_entry.gyro_y = imu_data.gyro_y;
    log_entry.gyro_z = imu_data.gyro_z;
    
    // ADD GPS DATA (NEW!)
    gps_data_t gps_data;
    if (gps_read(&gps_data) == ESP_OK && gps_data.valid) {
        log_entry.latitude = gps_data.latitude;
        log_entry.longitude = gps_data.longitude;
        log_entry.speed_ms = gps_data.speed_ms;
        log_entry.heading_deg = gps_data.heading_deg;
        log_entry.gps_satellites = gps_data.satellites_used;
    } else {
        // No GPS fix - set to zero
        log_entry.latitude = 0.0;
        log_entry.longitude = 0.0;
        log_entry.speed_ms = 0.0;
        log_entry.heading_deg = 0.0;
        log_entry.gps_satellites = 0;
    }
    
    // Write to buffer
    storage_manager_write(&log_entry);
}
```

---

### Phase 2: Python Analysis Script

**File**: `tools/analyze_gps_data.py`

```python
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import folium  # For map visualization

def load_log_data(filename):
    """Load binary log file"""
    # Parse binary log format
    # Each entry: timestamp(4) + tof(32) + imu(36) + gps(25) + flags(2) = 99 bytes
    pass

def extract_near_miss_locations(log_data):
    """Extract locations where near-misses occurred"""
    near_misses = []
    for entry in log_data:
        if entry['flag_auto'] or entry['flag_manual']:
            if entry['latitude'] != 0.0 and entry['longitude'] != 0.0:
                near_misses.append({
                    'lat': entry['latitude'],
                    'lon': entry['longitude'],
                    'timestamp': entry['timestamp_ms'],
                    'satellites': entry['gps_satellites']
                })
    return near_misses

def create_heatmap(near_misses, output_file='near_miss_heatmap.html'):
    """Create interactive heatmap of near-miss locations"""
    if not near_misses:
        print("No near-miss events with GPS data")
        return
    
    # Calculate center point
    center_lat = np.mean([nm['lat'] for nm in near_misses])
    center_lon = np.mean([nm['lon'] for nm in near_misses])
    
    # Create map
    m = folium.Map(location=[center_lat, center_lon], zoom_start=15)
    
    # Add markers
    for nm in near_misses:
        folium.CircleMarker(
            location=[nm['lat'], nm['lon']],
            radius=5,
            color='red',
            fill=True,
            popup=f"Time: {nm['timestamp']}ms<br>Sats: {nm['satellites']}"
        ).add_to(m)
    
    # Save map
    m.save(output_file)
    print(f"Heatmap saved to {output_file}")

def analyze_high_risk_areas(near_misses, radius_meters=50):
    """Identify areas with high concentration of near-misses"""
    # Cluster near-misses by location
    # Use DBSCAN or similar clustering algorithm
    pass

# Example usage
if __name__ == '__main__':
    log_data = load_log_data('log_2026-01-27_14-30-00.bin')
    near_misses = extract_near_miss_locations(log_data)
    create_heatmap(near_misses)
    analyze_high_risk_areas(near_misses)
```

---

### Phase 3: ESP-NN Training (GPS Excluded)

**File**: `tools/train_espnn.py`

```python
import numpy as np
from esp_nn import ESPNN

def prepare_training_data(log_data):
    """Extract ONLY sensor features for training"""
    X = []  # Features
    y = []  # Labels
    
    for entry in log_data:
        # Extract sensor features (NO GPS!)
        features = [
            *entry['tof'],           # 16 ToF values
            entry['roll'],           # 1 roll
            entry['pitch'],          # 1 pitch
            entry['yaw'],            # 1 yaw
            entry['accel_x'],        # 1 accel_x
            entry['accel_y'],        # 1 accel_y
            entry['accel_z'],        # 1 accel_z
            entry['gyro_x'],         # 1 gyro_x
            entry['gyro_y'],         # 1 gyro_y
            entry['gyro_z']          # 1 gyro_z
        ]
        # Total: 28 features
        
        # Label: near-miss or not
        label = 1 if (entry['flag_auto'] or entry['flag_manual']) else 0
        
        X.append(features)
        y.append(label)
    
    return np.array(X), np.array(y)

def train_model(X, y):
    """Train ESP-NN model"""
    model = ESPNN(input_size=28, hidden_size=16, output_size=2)
    model.train(X, y, epochs=100)
    return model

# GPS data is NEVER used in training!
```

---

## Benefits of This Approach

### 1. Generalizable Model
- Model learns sensor patterns, not locations
- Works in any environment
- Not biased by specific routes

### 2. Location Analysis
- Can identify high-risk areas
- Can correlate near-misses with locations
- Can generate safety reports by area

### 3. Privacy Friendly
- GPS data is optional
- Can disable GPS for privacy
- Model still works without GPS

### 4. Efficient Training
- Fewer features = faster training
- Less overfitting risk
- Better generalization

---

## Configuration Options

### Disable GPS Logging (Future)

**File**: `data/config.txt`

```ini
# GPS Configuration
GPS_ENABLED=true           # Enable/disable GPS module
GPS_LOGGING=false          # Log GPS data (set to false to exclude from logs)
GPS_DISPLAY_ONLY=true      # Show GPS on display but don't log
```

**Use Cases:**
- Privacy concerns: `GPS_LOGGING=false`
- Testing without GPS: `GPS_ENABLED=false`
- Display only: `GPS_DISPLAY_ONLY=true`

---

## Summary

**Current Status:**
- ✅ GPS fields defined in log structure
- ❌ GPS data not yet populated (future implementation)
- ✅ Clear separation: sensors for training, GPS for analysis

**Key Points:**
1. **GPS = Metadata** - Never used for ESP-NN training
2. **Sensors = Features** - ToF + IMU used for training
3. **Location Analysis** - GPS enables heatmaps and route analysis
4. **Optional** - System works without GPS
5. **Privacy Friendly** - GPS can be disabled

**Next Steps:**
1. Implement GPS data population in main.c (Phase 1)
2. Create Python analysis scripts (Phase 2)
3. Train ESP-NN with sensor data only (Phase 3)
4. Generate location heatmaps (Phase 2)

---

**Document Version:** 1.0  
**Last Updated:** January 27, 2026  
**Status:** ✅ Strategy documented, implementation pending
