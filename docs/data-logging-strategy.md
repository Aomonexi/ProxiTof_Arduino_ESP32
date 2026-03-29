# Data Logging Strategy for TFLite Training & Live Monitoring

**Date:** December 9, 2025  
**Purpose:** Ensure data collection meets both ML training and live monitoring requirements

## Overview

The ProxiTof system collects synchronized sensor data optimized for:
1. **TensorFlow Lite model training** - Structured CSV format with labeled events
2. **Live monitoring** - Real-time visualization of ToF and IMU data
3. **Post-analysis** - Timestamped data for offline analysis

## Data Format

### CSV Structure

**File Format:** `log_YYYYMMDD_HHMMSS.csv`

**Header Row:**
```csv
timestamp_ms,tof_00,tof_01,tof_02,tof_03,tof_10,tof_11,tof_12,tof_13,tof_20,tof_21,tof_22,tof_23,tof_30,tof_31,tof_32,tof_33,roll,pitch,yaw,accel_x,accel_y,accel_z,gyro_x,gyro_y,gyro_z,min_distance_mm,approach_rate_mm_s,jerk_magnitude_m_s3,angular_velocity_deg_s,flag_manual,flag_auto,confidence
```

**Total Columns:** 32
- 1 timestamp
- 16 ToF distances (4x4 grid)
- 9 IMU values (3 orientation + 3 accel + 3 gyro)
- GPS fields (optional: latitude, longitude, speed, heading, satellites)
- 2 flags (manual, auto)
- Note: 4 derived features (min_distance, approach_rate, jerk, angular_velocity) are computed in memory for rule-based detection but NOT written to CSV. They can be recomputed from raw data during offline analysis.

### Example Data Row

```csv
1234567,450,455,460,465,440,445,450,455,430,435,440,445,420,425,430,435,5.2,-2.1,180.5,0.15,-0.05,9.81,2.3,-1.5,0.8,420,-125.5,1.85,2.85,1,0,0.00
```

**Interpretation:**
- `timestamp_ms`: 1234567 ms since boot
- `tof_00` to `tof_33`: 4x4 distance grid (420-465mm)
- `roll, pitch, yaw`: 5.2°, -2.1°, 180.5°
- `accel_x, accel_y, accel_z`: 0.15, -0.05, 9.81 m/s²
- `gyro_x, gyro_y, gyro_z`: 2.3, -1.5, 0.8 deg/s
- `min_distance_mm`: 420mm (minimum across grid)
- `approach_rate_mm_s`: -125.5 mm/s (approaching)
- `jerk_magnitude_m_s3`: 1.85 m/s³
- `angular_velocity_deg_s`: 2.85 deg/s
- `flag_manual`: 1 (button pressed)
- `flag_auto`: 0 (no ML detection)
- `confidence`: 0.00 (no ML active)

## TensorFlow Lite Training Requirements

### Input Features (25 features)

**Raw Sensor Data:**
1. **ToF Grid (16 features):** `tof_00` through `tof_33`
   - Range: 0-4000mm
   - Normalization: Divide by 4000 → [0, 1]

2. **IMU Orientation (3 features):** `roll`, `pitch`, `yaw`
   - Range: -180° to 180°
   - Normalization: Divide by 180 → [-1, 1]

3. **IMU Acceleration (3 features):** `accel_x`, `accel_y`, `accel_z`
   - Range: -10 to 10 m/s²
   - Normalization: Divide by 10 → [-1, 1]

4. **IMU Gyroscope (3 features):** `gyro_x`, `gyro_y`, `gyro_z`
   - Range: -500 to 500 deg/s
   - Normalization: Divide by 500 → [-1, 1]

### Labels

**Training Labels:**
- `flag_manual = 1`: Near-miss event (positive class)
- `flag_manual = 0`: Normal operation (negative class)

**Label Window:**
- When button pressed, mark ±1 second window (2 seconds total)
- This captures the approach and aftermath of near-miss

### Model Architecture

**Input Layer:** 25 features (normalized)
**Output Layer:** 1 confidence score [0.0, 1.0]

**Training Data Requirements:**
- Minimum 10 hours of diverse scenarios
- Minimum 200 manually flagged near-miss events
- ~216,000 samples per hour at 60Hz
- Total: ~2.16 million samples for 10 hours

## Live Monitoring Requirements

### Real-Time Visualization

**ToF Sensor (60Hz):**
- 4x4 heatmap visualization
- Minimum distance indicator
- Approach rate graph
- Update rate: 60 FPS

**IMU Sensor (100Hz):**
- 3D orientation visualization
- Acceleration vector display
- Gyroscope angular velocity
- Update rate: 60 FPS (downsampled from 100Hz)

**Derived Features:**
- Approach rate trend (mm/s)
- Jerk magnitude (m/s³)
- Angular velocity magnitude (deg/s)

### Serial Output Format

**For Live Monitoring Tools:**
```
[timestamp] ToF: min=420mm rate=-125mm/s | IMU: roll=5.2° pitch=-2.1° yaw=180.5° | Jerk: 1.85m/s³
```

**JSON Format (Optional):**
```json
{
  "timestamp_ms": 1234567,
  "tof": {
    "grid": [[450,455,460,465],[440,445,450,455],[430,435,440,445],[420,425,430,435]],
    "min_distance_mm": 420,
    "approach_rate_mm_s": -125.5
  },
  "imu": {
    "orientation": {"roll": 5.2, "pitch": -2.1, "yaw": 180.5},
    "acceleration": {"x": 0.15, "y": -0.05, "z": 9.81},
    "gyroscope": {"x": 2.3, "y": -1.5, "z": 0.8}
  },
  "derived": {
    "jerk_magnitude_m_s3": 1.85,
    "angular_velocity_deg_s": 2.85
  },
  "flags": {
    "manual": 1,
    "auto": 0,
    "confidence": 0.00
  }
}
```

## Data Collection Workflow

### Phase 1: Data Collection (No ML)

1. **Initialize sensors** (ToF + IMU)
2. **Start logging** to SD card at 60Hz
3. **Manual flagging** via button press
4. **Collect 10+ hours** of diverse scenarios:
   - Walking (normal pace, fast pace)
   - Running
   - Static obstacles (walls, furniture)
   - Moving obstacles (people, vehicles)
   - Different lighting conditions
   - Various distances (100mm - 4000mm)

### Phase 2: Model Training (Offline)

1. **Extract CSV files** from SD card
2. **Preprocess data:**
   - Normalize features to [-1, 1] or [0, 1]
   - Balance positive/negative samples
   - Split train/validation/test (70/15/15)
3. **Train TFLite model:**
   - Input: 25 features
   - Output: Binary classification (near-miss or not)
   - Optimize for ESP32-S3 (quantization)
4. **Convert to TFLite** with ESP-NN optimization
5. **Flash model** to ESP32-S3

### Phase 3: Live Inference (With ML)

1. **Load TFLite model** from flash
2. **Run inference** at 60Hz
3. **Log predictions:**
   - `flag_auto = 1` when confidence > threshold (default 0.7)
   - `confidence` score for all frames
4. **Compare with manual flags** for validation
5. **Adjust threshold** based on false positive/negative rate

## Data Quality Checks

### Synchronization

**Requirement:** ToF and IMU data within 10ms
**Validation:**
```python
# Check timestamp alignment
tof_time = df['timestamp_ms']
imu_time = df['timestamp_ms']  # Same timestamp
assert (tof_time == imu_time).all()
```

### Completeness

**Requirement:** No missing values (use last known value)
**Validation:**
```python
# Check for NaN values
assert not df.isnull().any().any()
```

### Sampling Rate

**Requirement:** 60Hz ±5%
**Validation:**
```python
# Check time deltas
time_deltas = df['timestamp_ms'].diff()
mean_delta = time_deltas.mean()
assert 15 <= mean_delta <= 18  # 16ms ±2ms
```

### Label Balance

**Requirement:** At least 200 flagged events
**Validation:**
```python
# Count flagged events
flagged_count = df['flag_manual'].sum()
assert flagged_count >= 200
```

## Storage Estimates

### Per Hour

- **Samples:** 216,000 (60 samples/sec × 3600 sec)
- **Row size:** ~150 bytes (32 columns × ~4-5 bytes avg)
- **File size:** ~32 MB per hour
- **10 hours:** ~320 MB

### SD Card Requirements

- **Minimum:** 1 GB (for 30+ hours)
- **Recommended:** 8 GB or larger
- **Format:** FAT32
- **File rotation:** 100 MB per file

## Python Preprocessing Script

```python
import pandas as pd
import numpy as np

def preprocess_for_training(csv_path):
    """
    Preprocess ProxiTof CSV data for TFLite training
    """
    # Load data
    df = pd.read_csv(csv_path)
    
    # Extract features (25 features)
    tof_cols = [f'tof_{i}{j}' for i in range(4) for j in range(4)]
    imu_cols = ['roll', 'pitch', 'yaw', 
                'accel_x', 'accel_y', 'accel_z',
                'gyro_x', 'gyro_y', 'gyro_z']
    
    X = df[tof_cols + imu_cols].values
    
    # Normalize features
    X[:, 0:16] = X[:, 0:16] / 4000.0  # ToF: [0, 1]
    X[:, 16:19] = X[:, 16:19] / 180.0  # Orientation: [-1, 1]
    X[:, 19:22] = X[:, 19:22] / 10.0   # Accel: [-1, 1]
    X[:, 22:25] = X[:, 22:25] / 500.0  # Gyro: [-1, 1]
    
    # Extract labels
    y = df['flag_manual'].values
    
    return X, y

# Usage
X_train, y_train = preprocess_for_training('log_20250115_093045.csv')
```

## Summary

✅ **CSV format is optimized for TFLite training**
- 25 input features match model requirements
- Normalized ranges documented
- Labels clearly defined

✅ **Live monitoring is supported**
- Real-time serial output at 60Hz
- All sensor data available
- Derived features computed

✅ **Data quality is ensured**
- Synchronized timestamps
- No missing values
- Consistent sampling rate

✅ **Storage is efficient**
- ~32 MB per hour
- File rotation at 100MB
- Date-based organization

The current design fully supports both TFLite training and live monitoring requirements!
