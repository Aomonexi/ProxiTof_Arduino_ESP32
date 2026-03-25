# ESP-NN Training Workflow - From Manual Flagging to Automated Detection

**Date**: January 27, 2026  
**Purpose**: Complete workflow for training ESP-NN using manual flags as ground truth labels

---

## Overview

The system uses **supervised learning** where YOU (the user) teach the neural network what a "near-miss" looks like by pressing the ACTION button during dangerous situations. The neural network learns from your examples.

---

## The Training Pipeline

```
┌─────────────────────────────────────────────────────────────────┐
│                    PHASE 1: DATA COLLECTION                     │
│                     (You riding the bike)                       │
└─────────────────────────────────────────────────────────────────┘
                              ↓
        ┌─────────────────────────────────────────┐
        │  Normal riding: System logs everything  │
        │  - ToF: 16 distances                    │
        │  - IMU: 9 values (orientation + motion) │
        │  - Timestamp                            │
        │  - flag_manual = 0 (no button press)    │
        └─────────────────────────────────────────┘
                              ↓
        ┌─────────────────────────────────────────┐
        │  Near-miss happens!                     │
        │  → You press ACTION button (GPIO 3)     │
        │  → System logs with flag_manual = 1     │
        │  → LED flashes BLUE                     │
        └─────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────────┐
│                    PHASE 2: DATA EXTRACTION                     │
│                  (Python script on computer)                    │
└─────────────────────────────────────────────────────────────────┘
                              ↓
        ┌─────────────────────────────────────────┐
        │  Extract log files from SD card         │
        │  → Connect USB (LEFT port)              │
        │  → Copy .bin files to computer          │
        │  → Parse binary format                  │
        └─────────────────────────────────────────┘
                              ↓
        ┌─────────────────────────────────────────┐
        │  Create labeled dataset                 │
        │  → flag_manual = 1 → Label = "near-miss"│
        │  → flag_manual = 0 → Label = "safe"     │
        └─────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────────┐
│                    PHASE 3: MODEL TRAINING                      │
│                  (Python + ESP-NN on computer)                  │
└─────────────────────────────────────────────────────────────────┘
                              ↓
        ┌─────────────────────────────────────────┐
        │  Train neural network                   │
        │  Input: 28 sensor features              │
        │  Output: near-miss probability          │
        │  → Network learns patterns              │
        └─────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────────┐
│                    PHASE 4: DEPLOYMENT                          │
│                  (Flash model back to ESP32)                    │
└─────────────────────────────────────────────────────────────────┘
                              ↓
        ┌─────────────────────────────────────────┐
        │  System now detects automatically!      │
        │  → flag_auto = 1 when detected          │
        │  → LED flashes ORANGE/RED               │
        │  → No button press needed               │
        └─────────────────────────────────────────┘
```

---

## Phase 1: Data Collection (On Device)

### What Happens When You Press ACTION Button

**Normal Riding (No Button Press):**
```c
log_entry_t entry = {
    .timestamp_ms = 12345,
    .tof = {450, 523, 612, ...},      // 16 distances
    .roll = 12.3, .pitch = -5.7, .yaw = 180.0,
    .accel_x = 0.98, .accel_y = 0.02, .accel_z = 9.81,
    .gyro_x = 0.5, .gyro_y = 0.3, .gyro_z = 0.1,
    .flag_manual = 0,  // ← NO BUTTON PRESS = SAFE
    .flag_auto = 0
};
```

**Near-Miss (You Press Button):**
```c
log_entry_t entry = {
    .timestamp_ms = 12345,
    .tof = {150, 180, 200, ...},      // Close distances!
    .roll = 25.5, .pitch = -15.2, .yaw = 185.3,
    .accel_x = 2.5, .accel_y = 1.8, .accel_z = 10.2,
    .gyro_x = 45.2, .gyro_y = 30.1, .gyro_z = 15.5,
    .flag_manual = 1,  // ← BUTTON PRESSED = NEAR-MISS!
    .flag_auto = 0
};
```

### Data Collection Strategy

**Goal**: Collect diverse examples of near-misses

**What to Flag:**
1. **Close calls with vehicles** - Car cuts you off
2. **Sudden braking** - You brake hard to avoid collision
3. **Swerving to avoid** - You swerve around obstacle
4. **Door opening** - Parked car door opens in front of you
5. **Pedestrian crossing** - Pedestrian steps into path
6. **Tight squeeze** - Passing between obstacles with little clearance

**How to Flag:**
- Press and hold ACTION button (GPIO 3) during the event
- LED turns BLUE while held
- Release when danger passes
- System logs all samples while button held with `flag_manual = 1`

**Collection Tips:**
- Collect at least 100-200 near-miss examples
- Collect 10x more normal riding examples (1000-2000)
- Ride in different conditions (day/night, traffic/quiet)
- Include false positives (things that LOOK dangerous but aren't)

---

## Phase 2: Data Extraction (Python Script)

### Step 1: Extract Log Files

**Connect USB (LEFT port):**
```bash
# Device appears as USB flash drive
# Copy all .bin files from /logs/ folder
cp /Volumes/PROXITOF/logs/*.bin ~/proxitof_data/
```

### Step 2: Parse Binary Logs

**File**: `tools/parse_logs.py`

```python
import struct
import numpy as np
import pandas as pd

def parse_log_file(filename):
    """
    Parse binary log file into structured data
    
    Log entry format (99 bytes):
    - timestamp_ms: uint32 (4 bytes)
    - tof[16]: uint16 × 16 (32 bytes)
    - roll, pitch, yaw: float × 3 (12 bytes)
    - accel_x, accel_y, accel_z: float × 3 (12 bytes)
    - gyro_x, gyro_y, gyro_z: float × 3 (12 bytes)
    - latitude, longitude: double × 2 (16 bytes)
    - speed_ms, heading_deg: float × 2 (8 bytes)
    - gps_satellites: uint8 (1 byte)
    - flag_manual: uint8 (1 byte)
    - flag_auto: uint8 (1 byte)
    Total: 99 bytes per entry
    """
    
    entries = []
    entry_size = 99
    
    with open(filename, 'rb') as f:
        data = f.read()
        
    # Parse each entry
    for i in range(0, len(data), entry_size):
        if i + entry_size > len(data):
            break  # Incomplete entry
            
        chunk = data[i:i+entry_size]
        
        # Unpack binary data
        # Format: I = uint32, H = uint16, f = float, d = double, B = uint8
        fmt = 'I'      # timestamp_ms (4 bytes)
        fmt += '16H'   # tof[16] (32 bytes)
        fmt += '9f'    # roll, pitch, yaw, accel_x/y/z, gyro_x/y/z (36 bytes)
        fmt += '2d'    # latitude, longitude (16 bytes)
        fmt += '2f'    # speed_ms, heading_deg (8 bytes)
        fmt += '3B'    # gps_satellites, flag_manual, flag_auto (3 bytes)
        
        values = struct.unpack(fmt, chunk)
        
        entry = {
            'timestamp_ms': values[0],
            'tof': list(values[1:17]),
            'roll': values[17],
            'pitch': values[18],
            'yaw': values[19],
            'accel_x': values[20],
            'accel_y': values[21],
            'accel_z': values[22],
            'gyro_x': values[23],
            'gyro_y': values[24],
            'gyro_z': values[25],
            'latitude': values[26],
            'longitude': values[27],
            'speed_ms': values[28],
            'heading_deg': values[29],
            'gps_satellites': values[30],
            'flag_manual': values[31],
            'flag_auto': values[32]
        }
        
        entries.append(entry)
    
    return entries

def create_dataset(log_files):
    """Create training dataset from multiple log files"""
    all_entries = []
    
    for log_file in log_files:
        print(f"Parsing {log_file}...")
        entries = parse_log_file(log_file)
        all_entries.extend(entries)
    
    print(f"Total entries: {len(all_entries)}")
    
    # Convert to DataFrame
    df = pd.DataFrame(all_entries)
    
    # Expand ToF array into separate columns
    tof_df = pd.DataFrame(df['tof'].tolist(), columns=[f'tof_{i}' for i in range(16)])
    df = pd.concat([df.drop('tof', axis=1), tof_df], axis=1)
    
    return df

# Example usage
if __name__ == '__main__':
    log_files = [
        'log_2026-01-27_14-30-00.bin',
        'log_2026-01-27_15-45-00.bin',
        'log_2026-01-28_09-15-00.bin'
    ]
    
    df = create_dataset(log_files)
    
    # Save as CSV for inspection
    df.to_csv('training_data.csv', index=False)
    
    # Statistics
    print(f"\nDataset Statistics:")
    print(f"Total samples: {len(df)}")
    print(f"Near-miss (manual): {df['flag_manual'].sum()}")
    print(f"Near-miss (auto): {df['flag_auto'].sum()}")
    print(f"Safe samples: {len(df) - df['flag_manual'].sum() - df['flag_auto'].sum()}")
```

---

## Phase 3: Model Training (ESP-NN)

### Step 1: Prepare Training Data

**File**: `tools/prepare_training_data.py`

```python
import numpy as np
import pandas as pd
from sklearn.model_selection import train_test_split
from sklearn.preprocessing import StandardScaler

def prepare_features_and_labels(df):
    """
    Extract features (X) and labels (y) from dataset
    
    Features: 28 sensor values
    Labels: 0 = safe, 1 = near-miss
    """
    
    # Feature columns (28 total)
    feature_cols = (
        [f'tof_{i}' for i in range(16)] +  # 16 ToF distances
        ['roll', 'pitch', 'yaw'] +          # 3 orientation
        ['accel_x', 'accel_y', 'accel_z'] + # 3 acceleration
        ['gyro_x', 'gyro_y', 'gyro_z']      # 3 gyroscope
    )
    
    # Extract features
    X = df[feature_cols].values
    
    # Create labels: 1 if flagged (manual OR auto), 0 otherwise
    y = ((df['flag_manual'] == 1) | (df['flag_auto'] == 1)).astype(int).values
    
    print(f"\nFeature shape: {X.shape}")
    print(f"Label shape: {y.shape}")
    print(f"Near-miss samples: {y.sum()} ({100*y.sum()/len(y):.2f}%)")
    print(f"Safe samples: {len(y) - y.sum()} ({100*(len(y)-y.sum())/len(y):.2f}%)")
    
    return X, y

def balance_dataset(X, y, ratio=10):
    """
    Balance dataset by undersampling majority class
    
    Args:
        ratio: Ratio of safe to near-miss samples (e.g., 10:1)
    """
    
    # Separate classes
    near_miss_idx = np.where(y == 1)[0]
    safe_idx = np.where(y == 0)[0]
    
    # Undersample safe class
    n_near_miss = len(near_miss_idx)
    n_safe_keep = min(len(safe_idx), n_near_miss * ratio)
    
    safe_idx_keep = np.random.choice(safe_idx, n_safe_keep, replace=False)
    
    # Combine
    keep_idx = np.concatenate([near_miss_idx, safe_idx_keep])
    np.random.shuffle(keep_idx)
    
    X_balanced = X[keep_idx]
    y_balanced = y[keep_idx]
    
    print(f"\nBalanced dataset:")
    print(f"Near-miss: {y_balanced.sum()}")
    print(f"Safe: {len(y_balanced) - y_balanced.sum()}")
    print(f"Ratio: {(len(y_balanced) - y_balanced.sum()) / y_balanced.sum():.1f}:1")
    
    return X_balanced, y_balanced

def normalize_features(X_train, X_test):
    """Normalize features to zero mean and unit variance"""
    scaler = StandardScaler()
    X_train_norm = scaler.fit_transform(X_train)
    X_test_norm = scaler.transform(X_test)
    
    return X_train_norm, X_test_norm, scaler

# Example usage
if __name__ == '__main__':
    # Load dataset
    df = pd.read_csv('training_data.csv')
    
    # Prepare features and labels
    X, y = prepare_features_and_labels(df)
    
    # Balance dataset (10:1 ratio)
    X_balanced, y_balanced = balance_dataset(X, y, ratio=10)
    
    # Split into train/test (80/20)
    X_train, X_test, y_train, y_test = train_test_split(
        X_balanced, y_balanced, test_size=0.2, random_state=42, stratify=y_balanced
    )
    
    # Normalize features
    X_train_norm, X_test_norm, scaler = normalize_features(X_train, X_test)
    
    # Save processed data
    np.save('X_train.npy', X_train_norm)
    np.save('X_test.npy', X_test_norm)
    np.save('y_train.npy', y_train)
    np.save('y_test.npy', y_test)
    
    # Save scaler for deployment
    import pickle
    with open('scaler.pkl', 'wb') as f:
        pickle.dump(scaler, f)
    
    print("\nTraining data saved!")
```

### Step 2: Train Neural Network

**File**: `tools/train_espnn.py`

```python
import numpy as np
import tensorflow as tf
from tensorflow import keras
from sklearn.metrics import classification_report, confusion_matrix

def create_model(input_size=28, hidden_size=16):
    """
    Create neural network for near-miss detection
    
    Architecture:
    - Input: 28 features (16 ToF + 9 IMU + 3 derived)
    - Hidden: 16 neurons (ReLU activation)
    - Output: 2 classes (safe, near-miss)
    """
    
    model = keras.Sequential([
        keras.layers.Input(shape=(input_size,)),
        keras.layers.Dense(hidden_size, activation='relu', name='hidden'),
        keras.layers.Dropout(0.2),  # Prevent overfitting
        keras.layers.Dense(2, activation='softmax', name='output')
    ])
    
    model.compile(
        optimizer='adam',
        loss='sparse_categorical_crossentropy',
        metrics=['accuracy']
    )
    
    return model

def train_model(X_train, y_train, X_test, y_test, epochs=100):
    """Train the model"""
    
    # Create model
    model = create_model()
    
    print("\nModel Architecture:")
    model.summary()
    
    # Callbacks
    early_stop = keras.callbacks.EarlyStopping(
        monitor='val_loss',
        patience=10,
        restore_best_weights=True
    )
    
    # Train
    print("\nTraining...")
    history = model.fit(
        X_train, y_train,
        validation_data=(X_test, y_test),
        epochs=epochs,
        batch_size=32,
        callbacks=[early_stop],
        verbose=1
    )
    
    return model, history

def evaluate_model(model, X_test, y_test):
    """Evaluate model performance"""
    
    # Predictions
    y_pred_proba = model.predict(X_test)
    y_pred = np.argmax(y_pred_proba, axis=1)
    
    # Metrics
    print("\nClassification Report:")
    print(classification_report(y_test, y_pred, target_names=['Safe', 'Near-Miss']))
    
    print("\nConfusion Matrix:")
    print(confusion_matrix(y_test, y_pred))
    
    # Calculate metrics
    accuracy = np.mean(y_pred == y_test)
    
    # Near-miss detection rate (recall for class 1)
    near_miss_detected = np.sum((y_test == 1) & (y_pred == 1))
    near_miss_total = np.sum(y_test == 1)
    detection_rate = near_miss_detected / near_miss_total if near_miss_total > 0 else 0
    
    # False alarm rate (false positives)
    false_alarms = np.sum((y_test == 0) & (y_pred == 1))
    safe_total = np.sum(y_test == 0)
    false_alarm_rate = false_alarms / safe_total if safe_total > 0 else 0
    
    print(f"\nPerformance Metrics:")
    print(f"Accuracy: {accuracy:.2%}")
    print(f"Near-miss detection rate: {detection_rate:.2%}")
    print(f"False alarm rate: {false_alarm_rate:.2%}")
    
    return accuracy, detection_rate, false_alarm_rate

# Example usage
if __name__ == '__main__':
    # Load training data
    X_train = np.load('X_train.npy')
    X_test = np.load('X_test.npy')
    y_train = np.load('y_train.npy')
    y_test = np.load('y_test.npy')
    
    # Train model
    model, history = train_model(X_train, y_train, X_test, y_test, epochs=100)
    
    # Evaluate
    evaluate_model(model, X_test, y_test)
    
    # Save model
    model.save('near_miss_model.h5')
    print("\nModel saved to near_miss_model.h5")
```

---

## Phase 4: Deployment (Back to ESP32)

### Step 1: Convert Model to ESP-NN Format

**File**: `tools/convert_to_espnn.py`

```python
import tensorflow as tf
import numpy as np

def convert_keras_to_espnn(model_path, output_path='model_weights.h'):
    """Convert Keras model to ESP-NN C header file"""
    
    # Load model
    model = tf.keras.models.load_model(model_path)
    
    # Extract weights
    weights = model.get_weights()
    
    # Generate C header file
    with open(output_path, 'w') as f:
        f.write("// Auto-generated ESP-NN model weights\n")
        f.write("// Generated from trained Keras model\n\n")
        
        f.write("#ifndef MODEL_WEIGHTS_H\n")
        f.write("#define MODEL_WEIGHTS_H\n\n")
        
        # Hidden layer weights
        W1 = weights[0]  # Shape: (28, 16)
        b1 = weights[1]  # Shape: (16,)
        
        f.write(f"// Hidden layer weights (28 x 16)\n")
        f.write(f"const float hidden_weights[{W1.shape[0]}][{W1.shape[1]}] = {{\n")
        for i in range(W1.shape[0]):
            f.write("    {")
            f.write(", ".join([f"{w:.6f}f" for w in W1[i]]))
            f.write("},\n")
        f.write("};\n\n")
        
        f.write(f"// Hidden layer biases (16)\n")
        f.write(f"const float hidden_biases[{len(b1)}] = {{")
        f.write(", ".join([f"{b:.6f}f" for b in b1]))
        f.write("};\n\n")
        
        # Output layer weights
        W2 = weights[2]  # Shape: (16, 2)
        b2 = weights[3]  # Shape: (2,)
        
        f.write(f"// Output layer weights (16 x 2)\n")
        f.write(f"const float output_weights[{W2.shape[0]}][{W2.shape[1]}] = {{\n")
        for i in range(W2.shape[0]):
            f.write("    {")
            f.write(", ".join([f"{w:.6f}f" for w in W2[i]]))
            f.write("},\n")
        f.write("};\n\n")
        
        f.write(f"// Output layer biases (2)\n")
        f.write(f"const float output_biases[{len(b2)}] = {{")
        f.write(", ".join([f"{b:.6f}f" for b in b2]))
        f.write("};\n\n")
        
        f.write("#endif // MODEL_WEIGHTS_H\n")
    
    print(f"Model weights saved to {output_path}")
    print(f"Copy this file to: components/data_processor/include/")

# Example usage
if __name__ == '__main__':
    convert_keras_to_espnn('near_miss_model.h5', 'model_weights.h')
```

### Step 2: Update ESP32 Code

**File**: `components/data_processor/near_miss_detector.c` (NEW)

```c
#include "model_weights.h"
#include <math.h>

// ReLU activation
static float relu(float x) {
    return x > 0 ? x : 0;
}

// Softmax activation
static void softmax(float *input, int size) {
    float max = input[0];
    for (int i = 1; i < size; i++) {
        if (input[i] > max) max = input[i];
    }
    
    float sum = 0;
    for (int i = 0; i < size; i++) {
        input[i] = expf(input[i] - max);
        sum += input[i];
    }
    
    for (int i = 0; i < size; i++) {
        input[i] /= sum;
    }
}

/**
 * Run neural network inference
 * Returns probability of near-miss (0.0 to 1.0)
 */
float predict_near_miss(float features[28]) {
    // Hidden layer (28 -> 16)
    float hidden[16];
    for (int i = 0; i < 16; i++) {
        hidden[i] = hidden_biases[i];
        for (int j = 0; j < 28; j++) {
            hidden[i] += features[j] * hidden_weights[j][i];
        }
        hidden[i] = relu(hidden[i]);
    }
    
    // Output layer (16 -> 2)
    float output[2];
    for (int i = 0; i < 2; i++) {
        output[i] = output_biases[i];
        for (int j = 0; j < 16; j++) {
            output[i] += hidden[j] * output_weights[j][i];
        }
    }
    
    // Softmax
    softmax(output, 2);
    
    // Return probability of near-miss (class 1)
    return output[1];
}
```

### Step 3: Integrate into Main Loop

**File**: `main/main.c` (UPDATE)

```c
// Add at top
#include "near_miss_detector.h"

// In main loop, replace rule-based detection with neural network
if (data_processor != NULL && tof_count > 0 && imu_count > 0) {
    // Prepare features for neural network
    float features[28];
    
    // ToF features (16)
    for (int i = 0; i < 16; i++) {
        features[i] = (float)tof_data.distances[i];
    }
    
    // IMU features (9)
    features[16] = imu_data.roll;
    features[17] = imu_data.pitch;
    features[18] = imu_data.yaw;
    features[19] = imu_data.accel_x;
    features[20] = imu_data.accel_y;
    features[21] = imu_data.accel_z;
    features[22] = imu_data.gyro_x;
    features[23] = imu_data.gyro_y;
    features[24] = imu_data.gyro_z;
    
    // Derived features (3) - optional
    features[25] = get_min_distance(&tof_data);
    features[26] = calculate_approach_rate();
    features[27] = calculate_jerk_magnitude();
    
    // Run neural network
    float near_miss_probability = predict_near_miss(features);
    
    // Threshold for detection (tune this!)
    bool near_miss_detected = (near_miss_probability > 0.7);  // 70% confidence
    
    if (near_miss_detected) {
        ESP_LOGW(TAG, "⚠️  NEAR-MISS DETECTED BY NEURAL NETWORK!");
        ESP_LOGW(TAG, "Confidence: %.1f%%", near_miss_probability * 100);
        // Set flag_auto = 1 in log entry
    }
}
```

---

## Summary

**The Complete Workflow:**

1. **Collect Data** (1-2 weeks of riding)
   - Press ACTION button during near-misses
   - System logs with `flag_manual = 1`
   - Collect 100-200 near-miss examples
   - Collect 1000-2000 safe examples

2. **Extract & Prepare** (1 hour)
   - Copy log files from SD card
   - Parse binary format
   - Create labeled dataset
   - Balance classes (10:1 ratio)

3. **Train Model** (30 minutes)
   - Train neural network on labeled data
   - Evaluate performance (aim for >90% accuracy)
   - Convert to ESP-NN format

4. **Deploy** (10 minutes)
   - Flash model weights to ESP32
   - System now detects automatically!
   - `flag_auto = 1` when detected

**Key Points:**
- ✅ Manual flags = Ground truth labels
- ✅ Neural network learns from YOUR examples
- ✅ More data = Better accuracy
- ✅ Diverse examples = Better generalization
- ✅ After training, system detects automatically

---

**Document Version:** 1.0  
**Last Updated:** January 27, 2026  
**Status:** ✅ Complete training workflow documented
