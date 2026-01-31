# Feature Count Clarification - ML Training Input

**Date**: January 31, 2026  
**Status**: ✅ Verified and Corrected

---

## Question: Is the input actually 25 features?

**Answer**: YES, exactly 25 features are used for ML training.

---

## Complete Feature Breakdown

### Input Features (25 total)

**1. ToF Distance Sensors (16 features)**
- Format: 4x4 grid with naming `tof_RC` where R=row (0-3), C=col (0-3)
- Column names: `tof_00, tof_01, tof_02, tof_03, tof_10, tof_11, tof_12, tof_13, tof_20, tof_21, tof_22, tof_23, tof_30, tof_31, tof_32, tof_33`
- Units: millimeters (mm)
- Range: 0-4000mm
- Purpose: Detect obstacles in front of cyclist

**2. IMU Orientation (3 features)**
- `roll` - Rotation around forward axis (degrees)
- `pitch` - Rotation around side axis (degrees)
- `yaw` - Rotation around vertical axis (degrees)
- Purpose: Detect bike tilt, turns, and heading

**3. IMU Acceleration (3 features)**
- `accel_x` - Forward/backward acceleration (m/s²)
- `accel_y` - Left/right acceleration (m/s²)
- `accel_z` - Up/down acceleration (m/s²)
- Purpose: Detect sudden braking, swerving, impacts

**4. IMU Gyroscope (3 features)**
- `gyro_x` - Angular velocity around X axis (deg/s)
- `gyro_y` - Angular velocity around Y axis (deg/s)
- `gyro_z` - Angular velocity around Z axis (deg/s)
- Purpose: Detect rapid turning, wobbling, instability

---

## What is EXCLUDED from Training?

### Excluded Columns (NOT used as features)

**1. Timestamp**
- `timestamp_ms` - Time in milliseconds
- Why excluded: Not relevant for pattern recognition
- Used for: Data organization and logging only

**2. GPS Data (Optional)**
- `latitude` - GPS latitude
- `longitude` - GPS longitude
- `speed_ms` - Speed in m/s
- `heading_deg` - GPS heading
- `gps_satellites` - Number of satellites
- Why excluded: Not always available, not critical for near-miss detection
- Used for: Geotagging events, post-analysis only

**3. flag_auto (CRITICAL)**
- `flag_auto` - Automatic near-miss detection flag
- **Why excluded**: This is the MODEL'S OUTPUT, not input!
- Using flag_auto as input would be circular reasoning
- The model learns to predict flag_auto based on sensor data

**4. flag_manual (Label, not feature)**
- `flag_manual` - Manual button press flag
- **Why excluded from features**: This is the TRAINING LABEL (y), not a feature (X)
- Used as: The "ground truth" that the model learns to predict
- When user presses button → flag_manual=1 → model learns this is a near-miss

---

## CSV Format from Storage Manager

From `components/storage_manager/storage_manager.c` (lines 515-560):

```c
// Write CSV header
fprintf(log_file, "timestamp_ms");

// ToF columns (tof_00 to tof_33)
for (int row = 0; row < 4; row++) {
    for (int col = 0; col < 4; col++) {
        fprintf(log_file, ",tof_%d%d", row, col);
    }
}

// IMU columns
fprintf(log_file, ",roll,pitch,yaw");
fprintf(log_file, ",accel_x,accel_y,accel_z");
fprintf(log_file, ",gyro_x,gyro_y,gyro_z");

// GPS columns (conditional)
if (config->gps_logging_enabled) {
    fprintf(log_file, ",latitude,longitude,speed_ms,heading_deg,gps_satellites");
}

// Flag columns
fprintf(log_file, ",flag_manual,flag_auto");
```

**Complete CSV Header:**
```
timestamp_ms,tof_00,tof_01,tof_02,tof_03,tof_10,tof_11,tof_12,tof_13,tof_20,tof_21,tof_22,tof_23,tof_30,tof_31,tof_32,tof_33,roll,pitch,yaw,accel_x,accel_y,accel_z,gyro_x,gyro_y,gyro_z,[latitude,longitude,speed_ms,heading_deg,gps_satellites],flag_manual,flag_auto
```

---

## Training Data Extraction

From `tools/prepare_training_data.py`:

```python
def extract_features_and_labels(df):
    # ToF columns (4x4 grid: row 0-3, col 0-3)
    tof_cols = [f'tof_{row}{col}' for row in range(4) for col in range(4)]
    
    # IMU columns
    imu_cols = ['roll', 'pitch', 'yaw', 
                'accel_x', 'accel_y', 'accel_z',
                'gyro_x', 'gyro_y', 'gyro_z']
    
    feature_cols = tof_cols + imu_cols  # 16 + 9 = 25 features
    
    # Extract features (X)
    X = df[feature_cols].values.astype(np.float32)
    
    # Extract labels (y) - ONLY flag_manual!
    y = df['flag_manual'].values.astype(np.int32)
    
    return X, y, feature_cols
```

**Result:**
- X shape: (n_samples, 25) - Input features
- y shape: (n_samples,) - Labels (0=safe, 1=near-miss)

---

## Why flag_auto is Excluded

### The Problem with Including flag_auto

If we included `flag_auto` as a feature:

```python
# ❌ WRONG - Circular reasoning!
X = df[['tof_00', ..., 'gyro_z', 'flag_auto']].values  # 26 features
y = df['flag_auto'].values  # Predicting flag_auto

# Model learns: "If flag_auto=1, then predict flag_auto=1"
# This is useless! It's just copying the input.
```

### The Correct Approach

```python
# ✅ CORRECT - Learn from sensor patterns
X = df[['tof_00', ..., 'gyro_z']].values  # 25 features (sensors only)
y = df['flag_manual'].values  # Predicting based on user judgment

# Model learns: "When sensors show pattern X, user pressed button"
# After training, model can detect similar patterns automatically
```

---

## How the Model Works

### Training Phase

1. **User rides bike** and presses ACTION button during near-misses
2. **System logs data** with flag_manual=1 when button pressed
3. **Model trains** on sensor patterns (X) associated with button presses (y)
4. **Model learns** what sensor patterns indicate near-misses

### Deployment Phase

1. **System reads sensors** (ToF + IMU)
2. **Model analyzes** sensor patterns
3. **Model predicts** probability of near-miss
4. **If probability > threshold** → Set flag_auto=1
5. **System logs** both flag_manual (button) and flag_auto (model prediction)

### Example

**Training Data:**
```csv
timestamp_ms,tof_00,...,tof_33,roll,pitch,yaw,accel_x,accel_y,accel_z,gyro_x,gyro_y,gyro_z,flag_manual,flag_auto
12345,150,180,200,...,25.5,-15.2,185.3,2.5,1.8,10.2,45.2,30.1,15.5,1,0
```

**What model learns:**
- "When ToF shows close distances (150-200mm)"
- "AND bike tilts sharply (roll=25.5°, pitch=-15.2°)"
- "AND high acceleration (2.5 m/s²)"
- "AND rapid turning (gyro=45.2 deg/s)"
- "→ This is a near-miss pattern!"

**After deployment:**
- Model sees similar sensor pattern
- Model predicts: "85% probability of near-miss"
- System sets flag_auto=1 automatically
- No button press needed!

---

## Summary

### Feature Count: 25

| Category | Count | Columns |
|----------|-------|---------|
| ToF Distances | 16 | tof_00 to tof_33 (4x4 grid) |
| Orientation | 3 | roll, pitch, yaw |
| Acceleration | 3 | accel_x, accel_y, accel_z |
| Gyroscope | 3 | gyro_x, gyro_y, gyro_z |
| **TOTAL** | **25** | **All sensor data** |

### Excluded from Features

- ❌ timestamp_ms (not relevant)
- ❌ GPS data (optional, not critical)
- ❌ flag_auto (model output, not input)
- ❌ flag_manual (training label, not feature)

### Key Insight

**flag_manual** is the training label (what we want to predict)  
**flag_auto** is the model's prediction (what we're trying to generate)

Using flag_auto as input would be like:
- "Predict if it's raining by looking at whether the rain detector says it's raining"
- Circular reasoning!

Instead, we use:
- "Predict if it's raining by looking at humidity, temperature, cloud cover, etc."
- Learn patterns from sensor data!

---

**Document Version:** 1.0  
**Last Updated:** January 31, 2026  
**Status:** ✅ Complete feature clarification
