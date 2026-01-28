# Training Data Sources - Complete Picture

**Date**: January 27, 2026  
**Purpose**: Visual summary of how all data sources contribute to ESP-NN training

---

## The Three Data Sources

```
┌─────────────────────────────────────────────────────────────────┐
│                    SOURCE 1: ALGORITHMS                         │
│                   (Automatic Detection)                         │
└─────────────────────────────────────────────────────────────────┘
                              ↓
        ┌─────────────────────────────────────────┐
        │  What: Rule-based detection             │
        │  When: Continuous, real-time            │
        │  Output: flag_auto = 1                  │
        │  Examples: ~800 per 10,000 samples      │
        │                                         │
        │  Detects:                               │
        │  • Critical proximity (< 300mm)         │
        │  • Rapid approach (> 500mm/s)           │
        │  • High jerk (> 20 m/s³)                │
        │  • High rotation (> 180 deg/s)          │
        └─────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────┐
│                    SOURCE 2: MANUAL FLAGS                       │
│                   (Human Ground Truth)                          │
└─────────────────────────────────────────────────────────────────┘
                              ↓
        ┌─────────────────────────────────────────┐
        │  What: Button press (GPIO 3)            │
        │  When: You decide it's dangerous        │
        │  Output: flag_manual = 1                │
        │  Examples: ~200-400 per 10,000 samples  │
        │                                         │
        │  Captures:                              │
        │  • Subjective danger                    │
        │  • Context-aware situations             │
        │  • Algorithm misses                     │
        │  • Edge cases                           │
        └─────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────┐
│                    SOURCE 3: SENSOR DATA                        │
│                   (Raw Features)                                │
└─────────────────────────────────────────────────────────────────┘
                              ↓
        ┌─────────────────────────────────────────┐
        │  What: ToF + IMU measurements           │
        │  When: Every frame (60 Hz)              │
        │  Output: 28 features                    │
        │  Examples: ALL 10,000 samples           │
        │                                         │
        │  Includes:                              │
        │  • 16 ToF distances                     │
        │  • 9 IMU values                         │
        │  • 3 derived features                   │
        └─────────────────────────────────────────┘

                              ↓
                              ↓
                              ↓

┌─────────────────────────────────────────────────────────────────┐
│                    COMBINED TRAINING DATASET                    │
└─────────────────────────────────────────────────────────────────┘

        Features (X):           Labels (y):
        ┌──────────────┐       ┌──────────────┐
        │ ToF[16]      │       │ 0 = Safe     │
        │ IMU[9]       │  →    │ 1 = Near-miss│
        │ Derived[3]   │       └──────────────┘
        └──────────────┘
        28 features             Binary label

        Label Logic:
        y = 1 if (flag_manual == 1 OR flag_auto == 1)
        y = 0 otherwise
```

---

## Example Training Samples

### Sample 1: Algorithm Detected, Manual Confirmed

```c
log_entry_t sample1 = {
    .timestamp_ms = 12345,
    
    // Features (X)
    .tof = {150, 180, 200, 220, 250, 280, 300, 320,
            340, 360, 380, 400, 420, 440, 460, 480},  // Close!
    .roll = 25.5, .pitch = -15.2, .yaw = 185.3,
    .accel_x = 2.5, .accel_y = 1.8, .accel_z = 10.2,
    .gyro_x = 45.2, .gyro_y = 30.1, .gyro_z = 15.5,
    
    // Labels (y)
    .flag_manual = 1,  // You pressed button
    .flag_auto = 1     // Algorithm detected
};

// Training label: y = 1 (NEAR-MISS)
// Confidence: HIGH (both sources agree)
```

---

### Sample 2: Algorithm Detected, Manual Not Pressed

```c
log_entry_t sample2 = {
    .timestamp_ms = 23456,
    
    // Features (X)
    .tof = {180, 200, 220, 240, 260, 280, 300, 320,
            340, 360, 380, 400, 420, 440, 460, 480},  // Close
    .roll = 15.2, .pitch = -8.5, .yaw = 178.5,
    .accel_x = 1.8, .accel_y = 1.2, .accel_z = 9.9,
    .gyro_x = 25.5, .gyro_y = 18.2, .gyro_z = 8.5,
    
    // Labels (y)
    .flag_manual = 0,  // You didn't press (busy riding)
    .flag_auto = 1     // Algorithm detected
};

// Training label: y = 1 (NEAR-MISS)
// Confidence: MEDIUM (algorithm only)
```

---

### Sample 3: Manual Flagged, Algorithm Missed

```c
log_entry_t sample3 = {
    .timestamp_ms = 34567,
    
    // Features (X)
    .tof = {800, 850, 900, 950, 1000, 1050, 1100, 1150,
            1200, 1250, 1300, 1350, 1400, 1450, 1500, 1550},  // Far
    .roll = 5.2, .pitch = -2.5, .yaw = 175.5,
    .accel_x = 0.5, .accel_y = 0.2, .accel_z = 9.8,
    .gyro_x = 5.5, .gyro_y = 3.2, .gyro_z = 1.5,
    
    // Labels (y)
    .flag_manual = 1,  // You pressed (subjective danger)
    .flag_auto = 0     // Algorithm missed (metrics OK)
};

// Training label: y = 1 (NEAR-MISS)
// Confidence: HIGH (human judgment)
// Important: NN should learn this pattern!
```

---

### Sample 4: Neither Detected (Safe)

```c
log_entry_t sample4 = {
    .timestamp_ms = 45678,
    
    // Features (X)
    .tof = {1500, 1600, 1700, 1800, 1900, 2000, 2100, 2200,
            2300, 2400, 2500, 2600, 2700, 2800, 2900, 3000},  // Far
    .roll = 2.5, .pitch = -1.2, .yaw = 180.0,
    .accel_x = 0.1, .accel_y = 0.05, .accel_z = 9.81,
    .gyro_x = 1.5, .gyro_y = 0.8, .gyro_z = 0.5,
    
    // Labels (y)
    .flag_manual = 0,  // You didn't press
    .flag_auto = 0     // Algorithm didn't detect
};

// Training label: y = 0 (SAFE)
// Confidence: HIGH (both sources agree)
```

---

## Dataset Statistics

### Typical Dataset (10,000 samples, 2 weeks of riding)

```
Total Samples: 10,000
├─ Safe (y=0): 9,000 (90%)
│  ├─ True negatives: 8,700 (87%)
│  └─ Potential false negatives: 300 (3%)
│
└─ Near-miss (y=1): 1,000 (10%)
   ├─ Algorithm only: 600 (6%)
   ├─ Manual only: 200 (2%)
   └─ Both detected: 200 (2%)

Label Sources:
├─ flag_auto = 1: 800 samples (8%)
├─ flag_manual = 1: 400 samples (4%)
└─ Both = 1: 200 samples (2%)
```

### Label Confidence

**High Confidence (400 samples):**
- Both algorithm AND manual flag set
- Use for validation set
- Most reliable labels

**Medium Confidence (600 samples):**
- Algorithm detected, manual not pressed
- Likely near-miss (you were busy)
- Use for training

**High Confidence (200 samples):**
- Manual flagged, algorithm missed
- Important edge cases
- NN should learn these!

**High Confidence (8,700 samples):**
- Neither detected
- Clearly safe
- Use for training

---

## Training Process

### Step 1: Combine Labels

```python
# Create binary labels from flags
y = ((df['flag_manual'] == 1) | (df['flag_auto'] == 1)).astype(int)

# Result:
# y = 1 if EITHER flag is set (near-miss)
# y = 0 if NEITHER flag is set (safe)
```

### Step 2: Extract Features

```python
# Extract 28 features
X = df[[
    'tof_0', 'tof_1', ..., 'tof_15',  # 16 ToF
    'roll', 'pitch', 'yaw',            # 3 orientation
    'accel_x', 'accel_y', 'accel_z',  # 3 acceleration
    'gyro_x', 'gyro_y', 'gyro_z',     # 3 gyroscope
    'min_distance',                    # 1 derived
    'approach_rate',                   # 1 derived
    'jerk_magnitude'                   # 1 derived
]].values
```

### Step 3: Train Neural Network

```python
# Train on combined dataset
model.fit(X, y, epochs=100)

# Model learns from:
# - Algorithm patterns (flag_auto)
# - Human judgment (flag_manual)
# - Sensor patterns (X features)
```

---

## Why This Works

### 1. Complementary Sources

**Algorithms catch:**
- Obvious dangers (close, fast, sudden)
- Objective metrics
- Consistent patterns

**Manual flags catch:**
- Subjective dangers (feels unsafe)
- Context-aware situations
- Algorithm misses
- Edge cases

**Together they provide:**
- Comprehensive coverage
- Diverse examples
- Balanced dataset

---

### 2. Weak Supervision

**Problem**: Can't manually label everything

**Solution**: Algorithms provide "weak labels"
- Not perfect, but good enough
- Catch most obvious cases
- Reduce manual labeling burden

**Result**: 10x more labeled data in same time

---

### 3. Human-in-the-Loop

**Algorithms provide baseline**
- Automatic labeling
- Consistent detection
- Fast data collection

**Humans provide corrections**
- Override false positives
- Add missed cases
- Provide context

**Neural network learns from both**
- Combines strengths
- Minimizes weaknesses
- Improves over time

---

## Summary

**Three Data Sources:**
1. **Algorithms** → Automatic labels (flag_auto)
2. **Manual Flags** → Human labels (flag_manual)
3. **Sensors** → Features (ToF + IMU)

**Combined Training:**
- Features (X): 28 sensor values
- Labels (y): flag_manual OR flag_auto
- Result: Robust near-miss detector

**Key Benefits:**
- ✅ Fast data collection (algorithms)
- ✅ High quality labels (manual)
- ✅ Comprehensive coverage (both)
- ✅ Learns from experience (NN)

**The Magic:**
Algorithms bootstrap the training, manual flags provide ground truth, and the neural network learns patterns from both to become smarter than either alone!

---

**Document Version:** 1.0  
**Last Updated:** January 27, 2026  
**Status:** ✅ Complete data source documentation
