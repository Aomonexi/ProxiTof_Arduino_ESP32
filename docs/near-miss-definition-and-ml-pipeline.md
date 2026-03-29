# Near-Miss Definition and Machine Learning Pipeline

**Date:** January 20, 2026  
**System:** ProxiTof Near-Miss Detection System  
**Purpose:** Complete explanation of near-miss criteria and ML training workflow

---

## 1. What is a Near-Miss Event?

### 1.1 Conceptual Definition

A **near-miss event** is a situation where a collision was narrowly avoided, typically characterized by:
- Close proximity to an object
- Rapid approach velocity
- Sudden evasive maneuver (high jerk)
- Combination of the above factors

### 1.2 Sensor-Based Detection Criteria

The system uses **multi-modal detection** combining distance and motion sensors:

#### Raw Sensor Readings

**ToF Sensor (VL53L8CX):**
```
Input: 4×4 distance grid (16 zones)
Range: 0-4000 mm
Frequency: 60 Hz
Example: [850, 920, 880, 910, 800, 750, 820, 870, ...]
         Zone 5 = 750mm (closest object)
```

**IMU Sensor (BNO08x):**
```
Orientation: Roll, Pitch, Yaw (degrees)
Acceleration: X, Y, Z (m/s²)
Gyroscope: X, Y, Z (deg/s)
Frequency: 100 Hz
Example: accel = [0.8, 0.3, 9.7] m/s²
```

#### Derived Features (Calculated)

**1. Minimum Distance**
```
Formula: d_min = min{d_i | i ∈ [0, 15]}
Example: 750 mm (from 16-zone grid)
Meaning: Closest object is 75 cm away
```

**2. Approach Rate**
```
Formula: v_approach = (d_current - d_previous) / Δt
Example: (750 - 850) / 0.016 = -6250 mm/s = -6.25 m/s
Meaning: Object approaching at 6.25 m/s (22.5 km/h)
Sign: Negative = approaching, Positive = receding
```

**3. Jerk Magnitude**
```
Formula: |j⃗| = √(j_x² + j_y² + j_z²)
         where j_x = (a_current_x - a_previous_x) / Δt
Example: 3.45 m/s³
Meaning: Sudden acceleration change (evasive maneuver)
```

**4. Angular Velocity**
```
Formula: |ω⃗| = √(ω_x² + ω_y² + ω_z²)
Example: 46.8 deg/s
Meaning: Rotation speed (turning, spinning)
```

---

## 2. Near-Miss Detection Rules

### 2.1 Rule-Based Detection (Current Implementation)

The system uses a **three-tier hierarchy** to classify near-miss events:

#### Tier 1: Critical Proximity (Highest Priority)
```
Condition: (d_min < 300 mm) AND (v_approach < -200 mm/s)

Example:
  d_min = 250 mm (25 cm)
  v_approach = -300 mm/s (approaching)
  Result: CRITICAL PROXIMITY ALERT ⚠️
  
Meaning: Object very close AND approaching
Risk Level: IMMINENT COLLISION
```

#### Tier 2: Combined Near-Miss
```
Condition: (Rapid Approach) AND (High Jerk) within 500ms

Rapid Approach: v_approach < -500 mm/s
High Jerk: |j⃗| > 2.0 m/s³
Temporal Window: 500 ms

Example:
  t=0ms:   v_approach = -625 mm/s (rapid approach detected)
  t=250ms: |j⃗| = 3.45 m/s³ (high jerk detected)
  Time difference: 250ms < 500ms
  Result: COMBINED NEAR-MISS ⚠️
  
Meaning: Fast approach + Evasive maneuver
Risk Level: HIGH - Near-miss event likely
```

#### Tier 3: Individual Conditions
```
Condition: (Rapid Approach) OR (High Jerk)

Example 1 - Rapid Approach Only:
  v_approach = -625 mm/s
  |j⃗| = 0.5 m/s³ (normal)
  Result: Potential near-miss (logged, not alerted)
  
Example 2 - High Jerk Only:
  v_approach = -100 mm/s (slow)
  |j⃗| = 3.2 m/s³ (sudden movement)
  Result: Potential near-miss (logged, not alerted)
  
Meaning: One risk factor present
Risk Level: MODERATE - Monitor situation
```

### 2.2 Detection Thresholds Summary

| Parameter | Threshold | Physical Meaning |
|-----------|-----------|------------------|
| **Critical Distance** | < 300 mm | Within arm's reach (30 cm) |
| **Critical Approach** | < -200 mm/s | Approaching at 0.2 m/s when close |
| **Rapid Approach** | < -500 mm/s | Approaching at 0.5 m/s (1.8 km/h) |
| **High Jerk** | > 2.0 m/s³ | Sudden acceleration change |
| **Temporal Window** | 500 ms | Human reaction time window |

### 2.3 Real-World Examples

**Example 1: Pedestrian Crossing**
```
Scenario: Cyclist approaching pedestrian who suddenly crosses path

t=0ms:
  d_min = 2000 mm (2 meters away)
  v_approach = -1250 mm/s (approaching at 1.25 m/s)
  |j⃗| = 0.8 m/s³ (normal cycling)
  Detection: Rapid approach detected

t=800ms:
  d_min = 1000 mm (1 meter away)
  v_approach = -1250 mm/s (still approaching)
  |j⃗| = 0.9 m/s³ (normal)
  Detection: Rapid approach continues

t=1200ms (pedestrian steps out):
  d_min = 500 mm (50 cm)
  v_approach = -1250 mm/s (still approaching)
  |j⃗| = 5.2 m/s³ (EMERGENCY BRAKE!)
  Detection: HIGH JERK + RAPID APPROACH
  Time since rapid approach: 1200ms > 500ms
  Result: Individual conditions (not combined)

t=1250ms:
  d_min = 450 mm
  v_approach = -800 mm/s (slowing)
  |j⃗| = 6.8 m/s³ (hard braking)
  Detection: COMBINED NEAR-MISS ⚠️
  (Both conditions within 500ms window)
```

**Example 2: Vehicle Cut-Off**
```
Scenario: Vehicle suddenly changes lanes in front of cyclist

t=0ms:
  d_min = 5000 mm (5 meters)
  v_approach = -500 mm/s (normal traffic)
  |j⃗| = 0.5 m/s³
  Detection: None

t=500ms (vehicle cuts in):
  d_min = 800 mm (80 cm)
  v_approach = -2500 mm/s (RAPID!)
  |j⃗| = 0.6 m/s³
  Detection: RAPID APPROACH

t=650ms (cyclist swerves):
  d_min = 600 mm
  v_approach = -1800 mm/s
  |j⃗| = 4.5 m/s³ (EVASIVE MANEUVER!)
  Detection: COMBINED NEAR-MISS ⚠️
  Time difference: 150ms < 500ms
```

---

## 3. Data Labeling Strategy

### 3.1 Automatic Labeling (Rule-Based)

The system automatically labels data in real-time:

```csv
timestamp_ms, ..., flag_manual, flag_auto, confidence
125430, ..., 0, 1, 0.00
```

**flag_auto = 1** when:
- Critical proximity detected, OR
- Combined near-miss detected

**flag_auto = 0** when:
- No near-miss conditions met
- Individual conditions only (logged but not flagged)

### 3.2 Manual Labeling (Button Press)

User can manually flag events by pressing button:

```csv
timestamp_ms, ..., flag_manual, flag_auto, confidence
125430, ..., 1, 0, 0.00
```

**flag_manual = 1** when:
- Button pressed during event
- User perceived near-miss
- May not match automatic detection

### 3.3 Combined Labeling Strategy

**Ground Truth Label:**
```
near_miss_label = flag_manual OR flag_auto

Examples:
  flag_manual=0, flag_auto=0 → Label: 0 (normal)
  flag_manual=1, flag_auto=0 → Label: 1 (user-perceived near-miss)
  flag_manual=0, flag_auto=1 → Label: 1 (algorithm-detected near-miss)
  flag_manual=1, flag_auto=1 → Label: 1 (confirmed near-miss)
```

**Label Quality:**
- **High confidence:** flag_manual=1 AND flag_auto=1 (both agree)
- **Medium confidence:** flag_manual=1 OR flag_auto=1 (one source)
- **Ambiguous:** flag_manual=1 AND flag_auto=0 (disagreement)

### 3.4 Dataset Composition

**Target Distribution:**
```
Normal events: 90-95% (flag=0)
Near-miss events: 5-10% (flag=1)

Minimum dataset size: 10 hours of riding
Expected near-miss events: 50-100 events
Total samples: ~2,160,000 (60 Hz × 10 hours)
```

---

## 4. Machine Learning Pipeline

### 4.1 Framework Selection: TensorFlow Lite

**Why TensorFlow Lite?**
- ✅ Official ESP32 support (TensorFlow Lite Micro)
- ✅ Optimized for embedded systems
- ✅ INT8 quantization for speed
- ✅ Small model size (<1MB)
- ✅ Extensive documentation
- ✅ Python training → C++ inference

**Alternatives Considered:**
- ❌ PyTorch Mobile: Limited ESP32 support
- ❌ ESP-NN: Lower-level, more complex
- ❌ ONNX Runtime: Larger footprint

### 4.2 Complete ML Workflow

```
┌─────────────────────────────────────────────────────────────┐
│  PHASE 1: DATA COLLECTION (ESP32)                           │
├─────────────────────────────────────────────────────────────┤
│  1. Ride with ProxiTof device (10+ hours)                   │
│  2. Automatic labeling (flag_auto)                          │
│  3. Manual labeling (flag_manual via button)                │
│  4. CSV files saved to SD card                              │
│  Output: log_YYYYMMDD_HHMMSS.csv files                      │
└─────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────┐
│  PHASE 2: DATA PREPROCESSING (Python)                       │
├─────────────────────────────────────────────────────────────┤
│  1. Load CSV files from SD card                             │
│  2. Combine flag_manual and flag_auto → ground truth        │
│  3. Handle class imbalance (SMOTE, undersampling)           │
│  4. Normalize features (StandardScaler)                     │
│  5. Create sliding windows (1 second = 60 samples)          │
│  6. Split: 70% train, 15% validation, 15% test              │
│  Output: train.npz, val.npz, test.npz                       │
└─────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────┐
│  PHASE 3: MODEL TRAINING (Python + TensorFlow)              │
├─────────────────────────────────────────────────────────────┤
│  Framework: TensorFlow 2.20+                                │
│  Architecture: LSTM or 1D-CNN                               │
│  Input: 25 raw features (16 ToF + 9 IMU), z-score normalized │
│  Output: Binary classification (near-miss / normal)         │
│  Loss: Binary crossentropy                                  │
│  Metrics: Precision, Recall, F1-Score, AUC-ROC              │
│  Training: 50-100 epochs with early stopping                │
│  Output: model.h5 (Keras format)                            │
└─────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────┐
│  PHASE 4: MODEL CONVERSION (TensorFlow Lite)                │
├─────────────────────────────────────────────────────────────┤
│  1. Convert Keras model → TFLite format                     │
│  2. Apply INT8 quantization (8-bit integers)                │
│  3. Optimize for ESP32 (reduce size, increase speed)        │
│  4. Validate accuracy after quantization                    │
│  Output: model_quantized.tflite (<500 KB)                   │
└─────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────┐
│  PHASE 5: MODEL DEPLOYMENT (ESP32)                          │
├─────────────────────────────────────────────────────────────┤
│  1. Convert .tflite → C array (xxd or tflite_convert)       │
│  2. Embed model in ESP32 flash memory                       │
│  3. Initialize TFLite Micro interpreter                     │
│  4. Allocate tensor arena in PSRAM (4MB)                    │
│  5. Run inference at 10 Hz (every 6 frames)                 │
│  6. Combine with rule-based detection (hybrid)              │
│  Output: Real-time ML-based near-miss detection             │
└─────────────────────────────────────────────────────────────┘
```

### 4.3 Model Architecture

**Recommended: 1D Convolutional Neural Network (1D-CNN)**

```python
import tensorflow as tf

model = tf.keras.Sequential([
    # Input: (batch, 25 features) - raw sensor values only
    tf.keras.layers.Input(shape=(60, 29)),
    
    # Convolutional layers for temporal feature extraction
    tf.keras.layers.Conv1D(32, kernel_size=3, activation='relu'),
    tf.keras.layers.MaxPooling1D(pool_size=2),
    tf.keras.layers.Dropout(0.3),
    
    tf.keras.layers.Conv1D(64, kernel_size=3, activation='relu'),
    tf.keras.layers.MaxPooling1D(pool_size=2),
    tf.keras.layers.Dropout(0.3),
    
    # Flatten and dense layers
    tf.keras.layers.Flatten(),
    tf.keras.layers.Dense(64, activation='relu'),
    tf.keras.layers.Dropout(0.4),
    
    # Output: Binary classification
    tf.keras.layers.Dense(1, activation='sigmoid')
])

model.compile(
    optimizer='adam',
    loss='binary_crossentropy',
    metrics=['accuracy', 'precision', 'recall', 'AUC']
)
```

**Model Size:**
- Parameters: ~50,000
- Float32 size: ~200 KB
- INT8 quantized: ~50 KB
- With TFLite overhead: ~100 KB total

**Alternative: LSTM (for sequential patterns)**

```python
model = tf.keras.Sequential([
    tf.keras.layers.Input(shape=(60, 29)),
    tf.keras.layers.LSTM(64, return_sequences=True),
    tf.keras.layers.Dropout(0.3),
    tf.keras.layers.LSTM(32),
    tf.keras.layers.Dropout(0.3),
    tf.keras.layers.Dense(32, activation='relu'),
    tf.keras.layers.Dense(1, activation='sigmoid')
])
```

---

## 5. Training Data Format

### 5.1 CSV Structure

```csv
timestamp_ms,tof_00,tof_01,...,tof_33,roll,pitch,yaw,accel_x,accel_y,accel_z,gyro_x,gyro_y,gyro_z,flag_manual
125430,850,920,880,910,800,750,820,870,900,850,880,920,950,940,960,980,15.2,-8.5,45.3,0.8,0.3,9.7,5.2,8.1,42.5,0
125446,845,915,875,905,795,745,815,865,895,845,875,915,945,935,955,975,15.3,-8.6,45.4,0.9,0.4,9.6,5.3,8.2,42.6,0
125462,840,910,870,900,790,740,810,860,890,840,870,910,940,930,950,970,15.4,-8.7,45.5,1.0,0.5,9.5,5.4,8.3,42.7,1
```

**Columns (26 total):**
- timestamp_ms (1)
- tof_00 to tof_33 (16 ToF zones)
- roll, pitch, yaw (3 orientation)
- accel_x, accel_y, accel_z (3 acceleration)
- gyro_x, gyro_y, gyro_z (3 gyroscope)
- flag_manual (1 label)

**Note:** Derived features (min_distance, approach_rate, jerk, angular_velocity) are calculated during preprocessing, not stored in CSV.

### 5.2 Python Preprocessing Script

```python
import pandas as pd
import numpy as np
from sklearn.preprocessing import StandardScaler
from sklearn.model_selection import train_test_split

def load_and_preprocess(csv_files):
    # Load all CSV files
    dfs = [pd.read_csv(f) for f in csv_files]
    data = pd.concat(dfs, ignore_index=True)
    
    # Calculate derived features
    data['min_distance'] = data[[f'tof_{i:02d}' for i in range(16)]].min(axis=1)
    data['approach_rate'] = data['min_distance'].diff() / data['timestamp_ms'].diff() * 1000
    
    # Calculate jerk magnitude
    accel_cols = ['accel_x', 'accel_y', 'accel_z']
    accel_diff = data[accel_cols].diff()
    time_diff = data['timestamp_ms'].diff() / 1000
    jerk = accel_diff.div(time_diff, axis=0)
    data['jerk_magnitude'] = np.sqrt((jerk**2).sum(axis=1))
    
    # Calculate angular velocity
    gyro_cols = ['gyro_x', 'gyro_y', 'gyro_z']
    data['angular_velocity'] = np.sqrt((data[gyro_cols]**2).sum(axis=1))
    
    # Create feature matrix (25 raw features — derived features are NOT used for ML)
    feature_cols = (
        [f'tof_{i:02d}' for i in range(16)] +
        ['roll', 'pitch', 'yaw'] +
        accel_cols + gyro_cols +
        ['min_distance', 'approach_rate', 'jerk_magnitude', 'angular_velocity']
    )
    
    X = data[feature_cols].values
    y = data['flag_manual'].values
    
    # Normalize features
    scaler = StandardScaler()
    X_scaled = scaler.fit_transform(X)
    
    # Create sliding windows (60 samples = 1 second at 60 Hz)
    X_windows, y_windows = create_windows(X_scaled, y, window_size=60)
    
    # Split dataset
    X_train, X_temp, y_train, y_temp = train_test_split(
        X_windows, y_windows, test_size=0.3, stratify=y_windows, random_state=42
    )
    X_val, X_test, y_val, y_test = train_test_split(
        X_temp, y_temp, test_size=0.5, stratify=y_temp, random_state=42
    )
    
    return (X_train, y_train), (X_val, y_val), (X_test, y_test), scaler

def create_windows(X, y, window_size=60):
    X_windows = []
    y_windows = []
    
    for i in range(len(X) - window_size + 1):
        X_windows.append(X[i:i+window_size])
        # Label is 1 if ANY sample in window is labeled as near-miss
        y_windows.append(int(y[i:i+window_size].max()))
    
    return np.array(X_windows), np.array(y_windows)
```

---

## 6. Model Conversion and Deployment

### 6.1 TensorFlow Lite Conversion

```python
import tensorflow as tf

# Load trained model
model = tf.keras.models.load_model('model.h5')

# Convert to TFLite with INT8 quantization
converter = tf.lite.TFLiteConverter.from_keras_model(model)
converter.optimizations = [tf.lite.Optimize.DEFAULT]
converter.target_spec.supported_types = [tf.int8]

# Representative dataset for quantization
def representative_dataset():
    for i in range(100):
        yield [X_train[i:i+1].astype(np.float32)]

converter.representative_dataset = representative_dataset
converter.inference_input_type = tf.int8
converter.inference_output_type = tf.int8

# Convert
tflite_model = converter.convert()

# Save
with open('model_quantized.tflite', 'wb') as f:
    f.write(tflite_model)

print(f"Model size: {len(tflite_model) / 1024:.2f} KB")
```

### 6.2 Convert to C Array

```bash
# Convert .tflite to C header file
xxd -i model_quantized.tflite > model_data.h
```

Output:
```c
unsigned char model_quantized_tflite[] = {
  0x1c, 0x00, 0x00, 0x00, 0x54, 0x46, 0x4c, 0x33, ...
};
unsigned int model_quantized_tflite_len = 51234;
```

### 6.3 ESP32 Integration

```cpp
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "model_data.h"

// Allocate tensor arena in PSRAM
constexpr int kTensorArenaSize = 4 * 1024 * 1024;  // 4MB
uint8_t *tensor_arena = (uint8_t*)heap_caps_malloc(
    kTensorArenaSize, MALLOC_CAP_SPIRAM
);

// Load model
const tflite::Model* model = tflite::GetModel(model_quantized_tflite);

// Create interpreter
tflite::MicroInterpreter interpreter(
    model, resolver, tensor_arena, kTensorArenaSize
);

// Allocate tensors
interpreter.AllocateTensors();

// Run inference
TfLiteTensor* input = interpreter.input(0);
TfLiteTensor* output = interpreter.output(0);

// Copy features to input tensor
for (int i = 0; i < 60; i++) {
    for (int j = 0; j < 29; j++) {
        input->data.int8[i * 29 + j] = quantized_features[i][j];
    }
}

// Invoke
interpreter.Invoke();

// Get result
int8_t prediction = output->data.int8[0];
float confidence = (prediction + 128) / 255.0f;  // Convert to 0-1

if (confidence > 0.7) {
    ESP_LOGW(TAG, "ML Near-Miss Detected! Confidence: %.2f", confidence);
}
```

---

## 7. Hybrid Detection System

### 7.1 Combining Rule-Based and ML

```
Final Decision = Rule-Based OR ML-Based

Priority:
1. Rule-based (fast, deterministic)
2. ML-based (learned patterns, confidence)
3. Combined (best of both)

Example:
  Rule-based: DETECTED (critical proximity)
  ML-based: NOT DETECTED (confidence=0.3)
  Final: DETECTED (rule-based takes priority)
  
  Rule-based: NOT DETECTED
  ML-based: DETECTED (confidence=0.85)
  Final: DETECTED (ML detected pattern)
```

### 7.2 Confidence Scoring

```
confidence_final = max(confidence_rule, confidence_ml)

where:
  confidence_rule = 1.0 if detected, 0.0 otherwise
  confidence_ml = model output (0.0 to 1.0)

Threshold: 0.7 (configurable)
```

---

## 8. Summary

### 8.1 Near-Miss Criteria

**Sensor-Based:**
- Minimum distance < 300mm
- Approach rate < -500 mm/s
- Jerk magnitude > 2.0 m/s³
- Combined conditions within 500ms

**User-Based:**
- Button press during event
- Subjective perception

**ML-Based (Future):**
- Learned patterns from training data
- Confidence score > 0.7

### 8.2 ML Framework

**Selected: TensorFlow Lite for Microcontrollers**
- Training: Python + TensorFlow 2.20+
- Deployment: C++ + TFLite Micro
- Quantization: INT8 (8-bit)
- Model size: <500 KB
- Inference: 10 Hz (every 6 frames)

### 8.3 Complete Workflow

1. ✅ **Data Collection** - Ride with device, automatic + manual labeling
2. ✅ **Preprocessing** - Calculate features, normalize, create windows
3. ⏳ **Training** - 1D-CNN or LSTM, binary classification
4. ⏳ **Conversion** - TFLite INT8 quantization
5. ⏳ **Deployment** - Embed in ESP32, hybrid detection
6. ⏳ **Validation** - Field testing, accuracy measurement

---

**Document Version:** 1.0  
**Date:** January 20, 2026  
**Status:** ✅ Rule-Based Detection Active, ML Pipeline Documented  
**Next Step:** Collect training dataset (10+ hours)

