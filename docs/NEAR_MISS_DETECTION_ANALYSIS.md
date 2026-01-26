# Near-Miss Detection: Current Implementation & Future ML Integration

**Date**: January 26, 2026  
**Status**: Algorithm-based (Rule-based) - No ML yet  
**Purpose**: Analysis of current detection logic and ML integration plan

---

## Your Questions Answered

### Q1: Why is Jerk Non-Zero When Not Moving?

**Short Answer**: Sensor noise and gravity!

**Detailed Explanation**:

#### What You're Seeing
```
Stationary device on table:
- Acceleration: X=0.02, Y=-0.01, Z=9.81 m/s²
- Jerk: 0.5-2.0 m/s³ (should be 0, but isn't!)
```

#### Why This Happens

**1. Sensor Noise**
```c
// IMU sensor has inherent noise
Real acceleration:     0.000 m/s²
Measured (frame 1):    0.015 m/s²  ← Noise!
Measured (frame 2):   -0.008 m/s²  ← More noise!

// Jerk calculation:
delta_a = -0.008 - 0.015 = -0.023 m/s²
delta_t = 0.01 s (100 Hz)
jerk = -0.023 / 0.01 = -2.3 m/s³  ← Non-zero from noise!
```

**2. Gravity Component**
```c
// Even when stationary, Z-axis measures gravity
Z-axis: 9.81 m/s² (gravity)

// Small orientation changes cause jerk:
Tilt 0.1° → Z changes from 9.81 to 9.80 m/s²
delta_a = 0.01 m/s²
jerk = 0.01 / 0.01 = 1.0 m/s³  ← From tiny tilt!
```

**3. Vibrations**
- Table vibrations
- Air conditioning
- Nearby footsteps
- Device's own electronics

#### Solution: Add Jerk Threshold

**Current Code** (too sensitive):
```c
if (jerk_magnitude > 20.0f) {  // Triggers on real motion
    near_miss_active = true;
}
```

**Better Code** (with noise filtering):
```c
// Only trigger on significant jerk (ignore noise)
if (jerk_magnitude > 50.0f) {  // Higher threshold
    near_miss_active = true;
}

// Or use moving average to smooth out noise
static float jerk_history[10] = {0};
float avg_jerk = calculate_moving_average(jerk_history, jerk_magnitude);
if (avg_jerk > 30.0f) {
    near_miss_active = true;
}
```

---

### Q2: Near-Miss Detection - Too Sensitive?

**You're absolutely right!** The current implementation is **too simple** and will cause **false positives**.

#### Current Logic (Too Simple!)

```c
// PROBLEM: Each condition triggers independently!
if (min_distance < 300) {
    near_miss = true;  // Just being close = near miss?
}

if (approach_rate < -500.0f) {
    near_miss = true;  // Just moving fast = near miss?
}

if (jerk_magnitude > 20.0f) {
    near_miss = true;  // Just running = near miss?
}
```

**False Positive Scenarios**:
1. **Walking with device** → High jerk → False alarm!
2. **Standing near wall** → Close distance → False alarm!
3. **Running** → High jerk + approach rate → False alarm!
4. **Driving** → Constant motion → False alarm!

#### What a REAL Near-Miss Should Be

A near-miss is when **multiple conditions occur together in quick succession**:

```
Real Near-Miss Example:
1. Object is far (2000mm)
2. Rapid approach starts (-800mm/s)  ← Moving toward you!
3. Distance drops quickly (2000 → 500mm in 2 seconds)
4. High jerk detected (sudden stop or dodge)
5. Object gets very close (< 300mm)

Timeline:
T=0.0s: 2000mm, approach=-800mm/s  ← Approaching!
T=1.0s: 1200mm, approach=-800mm/s  ← Still approaching!
T=1.5s:  500mm, approach=-600mm/s  ← Getting close!
T=1.8s:  300mm, jerk=50m/s³        ← Sudden brake/dodge!
T=2.0s:  250mm, approach=0mm/s     ← Near miss!
```

#### Better Detection Logic (Combined Conditions)

```c
// Track state over time
typedef struct {
    uint32_t rapid_approach_start_time;
    uint32_t high_jerk_time;
    uint16_t min_distance_during_approach;
    bool approach_active;
} near_miss_state_t;

static near_miss_state_t nm_state = {0};

// Better near-miss detection
bool detect_near_miss(float approach_rate, float jerk, uint16_t distance) {
    uint32_t now = get_timestamp_ms();
    
    // 1. Detect rapid approach START
    if (approach_rate < -500.0f && !nm_state.approach_active) {
        nm_state.rapid_approach_start_time = now;
        nm_state.approach_active = true;
        nm_state.min_distance_during_approach = distance;
    }
    
    // 2. Track minimum distance during approach
    if (nm_state.approach_active && distance < nm_state.min_distance_during_approach) {
        nm_state.min_distance_during_approach = distance;
    }
    
    // 3. Detect high jerk during approach
    if (nm_state.approach_active && jerk > 50.0f) {
        nm_state.high_jerk_time = now;
    }
    
    // 4. Check if near-miss conditions met
    bool near_miss = false;
    if (nm_state.approach_active) {
        uint32_t approach_duration = now - nm_state.rapid_approach_start_time;
        uint32_t time_since_jerk = now - nm_state.high_jerk_time;
        
        // Near-miss if:
        // - Rapid approach lasted 0.5-5 seconds
        // - Got very close (< 300mm)
        // - High jerk occurred within 2 seconds
        if (approach_duration > 500 && approach_duration < 5000 &&
            nm_state.min_distance_during_approach < 300 &&
            time_since_jerk < 2000) {
            near_miss = true;
        }
    }
    
    // 5. Reset state if approach ended
    if (approach_rate > -100.0f) {  // Slowed down or moving away
        nm_state.approach_active = false;
    }
    
    return near_miss;
}
```

#### Recommended Thresholds (Less Sensitive)

| Condition | Current | Recommended | Reason |
|-----------|---------|-------------|--------|
| **Critical Proximity** | < 300mm | < 200mm | Closer = more critical |
| **Rapid Approach** | > 500mm/s | > 800mm/s | Faster = more dangerous |
| **High Jerk** | > 20 m/s³ | > 50 m/s³ | Filter out normal motion |
| **High Rotation** | > 180 deg/s | > 300 deg/s | Filter out normal turns |
| **Time Window** | None | 0.5-5 seconds | Must happen in sequence |

---

### Q3: Machine Learning Integration - Where Is It?

**Current Status**: ❌ **No ML yet - Pure algorithm/rule-based**

You're correct - the current implementation is **entirely rule-based** with hardcoded thresholds. Here's the plan for ML integration:

#### Current System Architecture

```
┌─────────────┐
│   Sensors   │
│ (ToF + IMU) │
└──────┬──────┘
       │
       ▼
┌─────────────────┐
│ Feature Extract │  ← We're here!
│ - Min distance  │
│ - Approach rate │
│ - Jerk          │
│ - Angular vel   │
└──────┬──────────┘
       │
       ▼
┌─────────────────┐
│ Rule-Based      │  ← Current implementation
│ Detection       │
│ if (dist < 300) │
│   near_miss!    │
└──────┬──────────┘
       │
       ▼
┌─────────────────┐
│   Log Event     │
└─────────────────┘
```

#### Future ML Architecture

```
┌─────────────┐
│   Sensors   │
│ (ToF + IMU) │
└──────┬──────┘
       │
       ▼
┌─────────────────┐
│ Feature Extract │
│ - Min distance  │
│ - Approach rate │
│ - Jerk          │
│ - Angular vel   │
│ - Time series   │  ← Add temporal features
└──────┬──────────┘
       │
       ▼
┌─────────────────┐
│  ESP-NN Model   │  ← Add ML model here!
│  (Trained CNN)  │
│                 │
│ Input: Features │
│ Output: Score   │
│   0.0-1.0       │
└──────┬──────────┘
       │
       ▼
┌─────────────────┐
│  Threshold      │
│ if (score>0.8)  │
│   near_miss!    │
└──────┬──────────┘
       │
       ▼
┌─────────────────┐
│   Log Event     │
└─────────────────┘
```

#### ML Integration Steps

**Phase 1: Data Collection** (Current - What you're doing now!)
```c
// Collect labeled data
typedef struct {
    // Features
    float features[10];  // min_dist, approach_rate, jerk, etc.
    
    // Label (manual or algorithm-based)
    bool is_near_miss;   // True/False
    
    // Context
    uint32_t timestamp;
    char scenario[32];   // "walking", "cycling", "near_miss", etc.
} training_sample_t;

// Log to SD card for later training
storage_manager_write_training_sample(&sample);
```

**Phase 2: Model Training** (Offline - On your computer)
```python
# Python script to train model
import tensorflow as tf
import numpy as np

# 1. Load data from SD card
data = load_csv("proxitof_data.csv")
X = data[['min_dist', 'approach_rate', 'jerk', 'angular_vel']]
y = data['is_near_miss']

# 2. Train model
model = tf.keras.Sequential([
    tf.keras.layers.Dense(16, activation='relu'),
    tf.keras.layers.Dense(8, activation='relu'),
    tf.keras.layers.Dense(1, activation='sigmoid')
])
model.compile(optimizer='adam', loss='binary_crossentropy')
model.fit(X, y, epochs=50)

# 3. Convert to TensorFlow Lite
converter = tf.lite.TFLiteConverter.from_keras_model(model)
tflite_model = converter.convert()

# 4. Quantize for ESP32
converter.optimizations = [tf.lite.Optimize.DEFAULT]
quantized_model = converter.convert()

# 5. Save for ESP32
with open('near_miss_model.tflite', 'wb') as f:
    f.write(quantized_model)
```

**Phase 3: ESP-NN Integration** (On ESP32)
```c
#include "esp_nn.h"
#include "tensorflow/lite/micro/micro_interpreter.h"

// Load model into ESP32
static const unsigned char model_data[] = {
    #include "near_miss_model.h"  // Generated from .tflite
};

// Initialize TFLite Micro
tflite::MicroInterpreter* interpreter;

// Run inference
float predict_near_miss(feature_vector_t* features) {
    // 1. Prepare input tensor
    float* input = interpreter->input(0)->data.f;
    input[0] = features->min_distance / 4000.0f;  // Normalize
    input[1] = features->approach_rate / 1000.0f;
    input[2] = features->jerk_magnitude / 100.0f;
    input[3] = features->angular_velocity / 500.0f;
    
    // 2. Run inference
    interpreter->Invoke();
    
    // 3. Get output (probability 0.0-1.0)
    float* output = interpreter->output(0)->data.f;
    return output[0];  // 0.0 = not near-miss, 1.0 = near-miss
}

// Use in detection
bool detect_near_miss_ml(feature_vector_t* features) {
    float score = predict_near_miss(features);
    
    // Threshold (tune based on false positive rate)
    return score > 0.8f;  // 80% confidence
}
```

#### Why ML Will Be Better

**Current Rule-Based**:
```c
// Fixed thresholds - doesn't adapt
if (distance < 300 && approach_rate < -500) {
    return true;  // Always triggers on these values
}
```

**ML-Based**:
```c
// Learns patterns from real data
// Can distinguish:
// - Walking near wall (NOT near-miss)
// - Cycling past pedestrian (NOT near-miss)
// - Car suddenly braking (NEAR-MISS!)
// - Pedestrian jumping out (NEAR-MISS!)

float score = ml_model(features);
// Score considers:
// - Temporal patterns (how values change over time)
// - Context (speed, environment)
// - Learned patterns from training data
```

#### ESP-NN Performance

**ESP32-S3 Capabilities**:
- **CPU**: 240 MHz dual-core
- **RAM**: 512 KB
- **Flash**: 16 MB (your board)
- **AI Acceleration**: No dedicated NPU, but optimized libraries

**Expected Performance**:
- **Inference time**: 5-20ms per prediction
- **Frequency**: Can run at 50-200 Hz (plenty for 60 Hz sensors)
- **Model size**: 10-50 KB (tiny!)
- **Accuracy**: 90-95% (with good training data)

---

## Recommended Next Steps

### Immediate (Fix Current Issues)

1. **Increase jerk threshold**: 20 → 50 m/s³
2. **Add time-based logic**: Require conditions in sequence
3. **Add state machine**: Track approach → close → jerk timeline
4. **Filter sensor noise**: Moving average or low-pass filter

### Short-term (Improve Algorithm)

1. **Collect real data**: Log scenarios (walking, cycling, near-misses)
2. **Analyze patterns**: What makes a real near-miss?
3. **Tune thresholds**: Based on real-world testing
4. **Add context**: Speed, environment, user activity

### Long-term (ML Integration)

1. **Collect labeled dataset**: 1000+ samples (500 near-miss, 500 normal)
2. **Train model**: TensorFlow → TFLite
3. **Integrate ESP-NN**: Add TFLite Micro to project
4. **Deploy and test**: Compare ML vs rule-based
5. **Iterate**: Retrain with more data

---

## Code Changes Needed Now

Want me to implement:
1. ✅ **Higher jerk threshold** (50 m/s³ instead of 20)
2. ✅ **Time-based detection** (conditions must occur in sequence)
3. ✅ **State machine** (track approach timeline)
4. ✅ **Noise filtering** (moving average for jerk)

This will make the current system much more reliable before adding ML!

---

**Summary**:
- ✅ Jerk non-zero due to sensor noise and gravity
- ✅ Current detection too sensitive (needs time-based logic)
- ✅ No ML yet - pure algorithm (ML integration planned)
- ✅ Need to collect data first, then train model
- ✅ ESP-NN integration is feasible and will improve accuracy

Want me to implement the improved detection logic now?
