# Complete ML Training Guide for ESP-NN - ProxiTof Near-Miss Detection

**Date**: January 31, 2026  
**Purpose**: Step-by-step guide for PC setup, data collection, model training, and ESP32 deployment

---

## Overview

This guide covers the complete machine learning workflow for training a near-miss detection model that runs on ESP32-S3 using ESP-NN acceleration. The system uses **TensorFlow** (not PyTorch) because ESP-NN is optimized for TensorFlow Lite models.

**Why TensorFlow?**
- ✅ ESP-NN is designed for TensorFlow Lite models
- ✅ TFLite Micro runs natively on ESP32
- ✅ Espressif provides official TFLite component for ESP-IDF
- ✅ ESP-NN accelerates TFLite operations (fully connected, ReLU, etc.)
- ❌ PyTorch is NOT compatible with ESP-NN

---

## Part 1: PC Setup and Software Installation

### Prerequisites

**Operating System**: Windows, macOS, or Linux  
**Python Version**: 3.8 - 3.11 (3.11 recommended)  
**Disk Space**: ~5GB for Python packages and datasets  
**RAM**: Minimum 8GB (16GB recommended)

### Step 1: Install Python and Create Virtual Environment

**Windows:**
```powershell
# Download Python from python.org (3.11.x recommended)
# Install with "Add to PATH" checked

# Create virtual environment
python -m venv venv_proxitof

# Activate
.\venv_proxitof\Scripts\Activate.ps1

# If execution policy error:
Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser
```

**macOS/Linux:**
```bash
# Install Python (if not already installed)
# macOS: brew install python@3.11
# Linux: sudo apt install python3.11 python3.11-venv

# Create virtual environment
python3.11 -m venv venv_proxitof

# Activate
source venv_proxitof/bin/activate
```

### Step 2: Install Required Python Packages

The `requirements_ml.txt` file is already created in the project root:
```txt
# Core ML libraries
tensorflow>=2.15,<3.0  # ESP-NN requires TensorFlow 2.x
numpy>=1.24
pandas>=2.0
scikit-learn>=1.3

# Data visualization
matplotlib>=3.8
seaborn>=0.13

# Model conversion
flatbuffers>=23.5

# Utilities
tqdm>=4.66
jupyter>=1.0

# Optional: For advanced analysis
scipy>=1.11
```

**Important**: TensorFlow 2.x is required for ESP-NN compatibility. TensorFlow 3.x is NOT yet compatible with ESP32 TFLite Micro.

Install packages:
```bash
pip install --upgrade pip
pip install -r requirements_ml.txt
```


### Step 3: Verify Installation

```python
# test_installation.py
import tensorflow as tf
import numpy as np
import pandas as pd
import sklearn

print("TensorFlow version:", tf.__version__)
print("NumPy version:", np.__version__)
print("Pandas version:", pd.__version__)
print("Scikit-learn version:", sklearn.__version__)

# Test TensorFlow
print("\nTesting TensorFlow...")
model = tf.keras.Sequential([
    tf.keras.layers.Dense(10, activation='relu', input_shape=(5,)),
    tf.keras.layers.Dense(2, activation='softmax')
])
print("✅ TensorFlow working!")

# Test TFLite conversion
converter = tf.lite.TFLiteConverter.from_keras_model(model)
tflite_model = converter.convert()
print("✅ TFLite conversion working!")

print("\n🎉 All packages installed successfully!")
```

Run test:
```bash
python test_installation.py
```

---

## Part 2: Understanding Your Data Format

### Current CSV Format (from your system)

Your system logs data in CSV format with these columns:

```csv
timestamp_ms,tof_00,tof_01,...,tof_33,roll,pitch,yaw,accel_x,accel_y,accel_z,gyro_x,gyro_y,gyro_z,latitude,longitude,speed_ms,heading_deg,gps_satellites,flag_manual,flag_auto
```

**Total Features**: 28 sensor values
- **16 ToF distances** (tof_00 to tof_33 in 4x4 grid)
- **3 Orientation** (roll, pitch, yaw in degrees)
- **3 Acceleration** (accel_x, accel_y, accel_z in m/s²)
- **3 Gyroscope** (gyro_x, gyro_y, gyro_z in deg/s)
- **3 Derived** (min_distance, approach_rate, jerk_magnitude)

**Labels**:
- `flag_manual = 1`: You pressed ACTION button (near-miss)
- `flag_manual = 0`: Normal riding (safe)

### Example Data Row

**Normal Riding:**
```csv
12345,450,523,612,580,490,510,520,530,540,550,560,570,580,590,600,610,12.3,-5.7,180.0,0.98,0.02,9.81,0.5,0.3,0.1,51.5074,-0.1278,5.2,90.5,8,0,0
```

**Near-Miss (Button Pressed):**
```csv
12346,150,180,200,220,160,170,180,190,200,210,220,230,240,250,260,270,25.5,-15.2,185.3,2.5,1.8,10.2,45.2,30.1,15.5,51.5074,-0.1278,5.2,90.5,8,1,0
```


---

## Part 3: Data Collection Strategy

### What to Collect

**Goal**: Minimum 100-200 near-miss examples, 1000-2000 safe examples

**Near-Miss Examples (Press ACTION button):**
1. **Close calls with vehicles** - Car cuts you off, sudden lane change
2. **Emergency braking** - You brake hard to avoid collision
3. **Swerving maneuvers** - Quick swerve to avoid obstacle
4. **Door zone incidents** - Parked car door opens in front
5. **Pedestrian conflicts** - Person steps into your path
6. **Tight squeezes** - Passing between obstacles with <50cm clearance
7. **Intersection conflicts** - Vehicle runs red light/stop sign
8. **Overtaking too close** - Vehicle passes with minimal clearance

**Safe Examples (No button press):**
- Normal riding in traffic
- Maintaining safe following distance
- Smooth turns and lane changes
- Riding in bike lanes
- Stopped at traffic lights
- Cruising on empty roads

### Collection Tips

**Diversity is Key:**
- ✅ Different times of day (morning, afternoon, evening)
- ✅ Different weather conditions (sunny, cloudy, rain)
- ✅ Different traffic levels (light, moderate, heavy)
- ✅ Different road types (bike lanes, shared roads, paths)
- ✅ Different speeds (slow <10 km/h, medium 10-20 km/h, fast >20 km/h)

**Button Pressing Technique:**
- Press and HOLD ACTION button during the entire near-miss event
- Release when danger passes
- LED turns BLUE while held
- All samples while held are marked with `flag_manual = 1`

**Data Quality:**
- Collect at least 2-3 hours of riding per week
- Aim for 10-20 near-miss events per session
- Review data regularly to ensure good quality
- Delete corrupted or invalid log files

---

## Part 4: Data Extraction and Preparation

### Step 1: Extract CSV Files from SD Card

**Using USB Transfer Mode:**
```bash
# 1. Navigate to Mode 7 (USB Transfer) on device
# 2. Press ACTION button to activate USB mode
# 3. Connect USB cable to LEFT port (USB-OTG)
# 4. Device appears as flash drive

# Windows
copy E:\*.csv C:\proxitof_data\raw\

# macOS
cp /Volumes/PROXITOF/*.csv ~/proxitof_data/raw/

# Linux
cp /media/user/PROXITOF/*.csv ~/proxitof_data/raw/

# 5. Eject drive on PC
# 6. Disconnect cable
# 7. Press ACTION to exit USB mode
```

### Step 2: Load and Inspect Data

Create `tools/load_data.py`:


---

## Part 5: Training Workflow - Step by Step

### Step 1: Load and Inspect Data

```bash
cd tools
python load_data.py
```

**Expected Output:**
```
Found 5 CSV files
Loading log_20260130_143000.csv...
Loading log_20260130_153000.csv...
...

✅ Loaded 15234 total samples

==============================================================
DATA INSPECTION
==============================================================

Total samples: 15234
Near-miss (flag_manual=1): 156 (1.02%)
Safe (flag_manual=0):      15078 (98.98%)
Ratio (safe:near-miss):    96.7:1

✅ Combined dataset saved to ../data/processed/combined_data.csv
```

**What to Check:**
- ✅ At least 100-200 near-miss examples
- ✅ At least 1000-2000 safe examples
- ✅ No missing values in critical columns
- ✅ Sensor values in reasonable ranges

### Step 2: Prepare Training Data

```bash
python prepare_training_data.py
```

**Expected Output:**
```
==============================================================
PREPARING TRAINING DATA
==============================================================

✅ Loaded 15234 samples

==============================================================
FEATURE EXTRACTION
==============================================================
Feature shape: (15234, 25)
Label shape: (15234,)

==============================================================
DATASET BALANCING
==============================================================
Original dataset:
  Near-miss: 156 (1.02%)
  Safe:      15078 (98.98%)
  Ratio:     96.7:1

Balanced dataset (target ratio 10:1):
  Near-miss: 156 (9.09%)
  Safe:      1560 (90.91%)
  Ratio:     10.0:1
  Total:     1716 samples

==============================================================
TRAIN/TEST SPLIT
==============================================================
Training set:   1372 samples (80%)
Test set:       344 samples (20%)

==============================================================
FEATURE NORMALIZATION
==============================================================
Training set:
  Before: mean=123.45, std=456.78
  After:  mean=0.00, std=1.00

✅ X_train.npy       - Training features (1372, 25)
✅ X_test.npy        - Test features (344, 25)
✅ y_train.npy       - Training labels (1372,)
✅ y_test.npy        - Test labels (344,)
✅ scaler.pkl        - Feature scaler (for deployment)
✅ feature_names.txt - Feature column names

🎉 Data preparation complete!
```

### Step 3: Train Neural Network

```bash
python train_model.py
```

**Expected Output:**
```
==============================================================
MODEL TRAINING
==============================================================

Model Architecture:
Model: "near_miss_detector"
_________________________________________________________________
 Layer (type)                Output Shape              Param #   
=================================================================
 hidden (Dense)              (None, 16)                416       
 dropout (Dropout)           (None, 16)                0         
 output (Dense)              (None, 2)                 34        
=================================================================
Total params: 450
Trainable params: 450
Non-trainable params: 0

Training for up to 100 epochs...
Epoch 1/100
43/43 [==============================] - 1s 12ms/step - loss: 0.6234 - accuracy: 0.6789 - val_loss: 0.5123 - val_accuracy: 0.7456
...
Epoch 45/100
43/43 [==============================] - 0s 8ms/step - loss: 0.1234 - accuracy: 0.9567 - val_loss: 0.1456 - val_accuracy: 0.9456

==============================================================
MODEL EVALUATION
==============================================================

Classification Report:
              precision    recall  f1-score   support

        Safe     0.9600    0.9800    0.9699       310
   Near-Miss     0.8500    0.7500    0.7966        34

    accuracy                         0.9456       344

Confusion Matrix:
                Predicted
                Safe  Near-Miss
Actual Safe      304   6
       Near-Miss   8  26

==============================================================
PERFORMANCE METRICS
==============================================================
Accuracy:          94.56%
Detection Rate:    76.47% (near-miss recall)
False Alarm Rate:  1.94%
Precision:         81.25%
F1 Score:          0.7966
ROC AUC:           0.9234

==============================================================
INTERPRETATION
==============================================================
✅ Model correctly detects 76% of near-miss events
⚠️  Model has 2% false alarm rate
✅ When model alerts, it's correct 81% of the time

✅ Model saved to ../models/near_miss_model.h5
✅ Training history plot saved to ../models/training_history.png

🎉 Training complete!
```

**What to Look For:**
- ✅ Accuracy > 90%
- ✅ Detection rate > 75% (higher is better)
- ✅ False alarm rate < 5% (lower is better)
- ✅ F1 score > 0.75

**If Performance is Poor:**
- Collect more near-miss examples (aim for 200+)
- Collect more diverse safe examples
- Try different model architectures (more hidden neurons)
- Adjust class weights or balance ratio

### Step 4: Convert to TensorFlow Lite

```bash
python convert_to_tflite.py
```

**Expected Output:**
```
==============================================================
CONVERTING TO TENSORFLOW LITE
==============================================================
Input model: ../models/near_miss_model.h5
Output model: ../models/near_miss_model.tflite
Quantization: Disabled (float32)

✅ TFLite model saved

Model size: 2,456 bytes (2.40 KB)

==============================================================
TESTING TFLITE MODEL
==============================================================

✅ TFLite model accuracy: 94.48%
Keras model accuracy:  94.56%
Accuracy difference:   0.08%

✅ TFLite model matches Keras model accuracy

==============================================================
GENERATING C HEADER FILE
==============================================================
✅ C header file saved to ../components/ml_inference/model_data.h

Copy this file to your ESP32 project:
  cp ../components/ml_inference/model_data.h <your_project>/components/ml_inference/include/

🎉 Conversion complete!
```

---

## Part 6: ESP32 Deployment

### Step 1: Copy Model to ESP32 Project

```bash
# Copy generated header file
cp models/model_data.h components/ml_inference/include/

# Or for quantized model
cp models/model_data_quant.h components/ml_inference/include/model_data.h
```

### Step 2: Create ML Inference Component

The ML inference component is already scaffolded in your project. You need to implement the inference logic.

**File**: `components/ml_inference/ml_inference.c` (create if not exists)


### Step 3: Update ESP-IDF Component Configuration

**File**: `components/ml_inference/CMakeLists.txt`

```cmake
idf_component_register(
    SRCS "ml_inference.c"
    INCLUDE_DIRS "include"
    REQUIRES "esp-tflite-micro"  # TensorFlow Lite Micro
)
```

**File**: `components/ml_inference/idf_component.yml`

```yaml
dependencies:
  espressif/esp-tflite-micro: "^1.3.1"
```

### Step 4: Implement ML Inference

**File**: `components/ml_inference/include/ml_inference.h`

```c
#ifndef ML_INFERENCE_H
#define ML_INFERENCE_H

#include <stdbool.h>
#include <stdint.h>

/**
 * Initialize ML inference engine
 * Loads TFLite model and allocates tensor arena
 * 
 * @return true if successful, false otherwise
 */
bool ml_inference_init(void);

/**
 * Run inference on sensor features
 * 
 * @param features Array of 25 normalized features
 * @return Probability of near-miss (0.0 to 1.0)
 */
float ml_inference_predict(const float features[25]);

/**
 * Check if ML inference is ready
 * 
 * @return true if initialized and ready
 */
bool ml_inference_is_ready(void);

/**
 * Get inference latency in milliseconds
 * 
 * @return Last inference time in ms
 */
uint32_t ml_inference_get_latency_ms(void);

#endif // ML_INFERENCE_H
```

**File**: `components/ml_inference/ml_inference.c`

```c
#include "ml_inference.h"
#include "model_data.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/system_setup.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "ML_INFERENCE";

// TFLite Micro components
static const tflite::Model* model = nullptr;
static tflite::MicroInterpreter* interpreter = nullptr;
static TfLiteTensor* input = nullptr;
static TfLiteTensor* output = nullptr;

// Tensor arena (adjust size if needed)
constexpr int kTensorArenaSize = 10 * 1024;  // 10KB
static uint8_t tensor_arena[kTensorArenaSize];

// Performance tracking
static uint32_t last_inference_time_ms = 0;
static bool initialized = false;

bool ml_inference_init(void) {
    ESP_LOGI(TAG, "Initializing ML inference...");
    
    // Load model
    model = tflite::GetModel(model_data);
    if (model->version() != TFLITE_SCHEMA_VERSION) {
        ESP_LOGE(TAG, "Model schema version mismatch!");
        ESP_LOGE(TAG, "  Model: %d, Expected: %d", 
                 model->version(), TFLITE_SCHEMA_VERSION);
        return false;
    }
    ESP_LOGI(TAG, "✅ Model loaded (version %d)", model->version());
    
    // Register operations (ESP-NN accelerated)
    static tflite::MicroMutableOpResolver<3> resolver;
    resolver.AddFullyConnected();  // ESP-NN accelerated
    resolver.AddRelu();            // ESP-NN accelerated
    resolver.AddSoftmax();
    
    // Create interpreter
    static tflite::MicroInterpreter static_interpreter(
        model, resolver, tensor_arena, kTensorArenaSize);
    interpreter = &static_interpreter;
    
    // Allocate tensors
    TfLiteStatus allocate_status = interpreter->AllocateTensors();
    if (allocate_status != kTfLiteOk) {
        ESP_LOGE(TAG, "AllocateTensors() failed!");
        return false;
    }
    
    // Get input/output tensors
    input = interpreter->input(0);
    output = interpreter->output(0);
    
    // Verify tensor shapes
    if (input->dims->size != 2 || input->dims->data[1] != 25) {
        ESP_LOGE(TAG, "Invalid input tensor shape!");
        return false;
    }
    
    if (output->dims->size != 2 || output->dims->data[1] != 2) {
        ESP_LOGE(TAG, "Invalid output tensor shape!");
        return false;
    }
    
    ESP_LOGI(TAG, "✅ Tensors allocated");
    ESP_LOGI(TAG, "   Input:  [%d, %d]", input->dims->data[0], input->dims->data[1]);
    ESP_LOGI(TAG, "   Output: [%d, %d]", output->dims->data[0], output->dims->data[1]);
    ESP_LOGI(TAG, "   Arena used: %d / %d bytes", 
             interpreter->arena_used_bytes(), kTensorArenaSize);
    
    initialized = true;
    ESP_LOGI(TAG, "🎉 ML inference ready!");
    
    return true;
}

float ml_inference_predict(const float features[25]) {
    if (!initialized) {
        ESP_LOGW(TAG, "ML inference not initialized!");
        return 0.0f;
    }
    
    // Start timing
    int64_t start_time = esp_timer_get_time();
    
    // Copy features to input tensor
    for (int i = 0; i < 25; i++) {
        input->data.f[i] = features[i];
    }
    
    // Run inference
    TfLiteStatus invoke_status = interpreter->Invoke();
    if (invoke_status != kTfLiteOk) {
        ESP_LOGE(TAG, "Invoke() failed!");
        return 0.0f;
    }
    
    // Get output (probability of near-miss)
    float prob_safe = output->data.f[0];
    float prob_near_miss = output->data.f[1];
    
    // Calculate inference time
    int64_t end_time = esp_timer_get_time();
    last_inference_time_ms = (end_time - start_time) / 1000;
    
    // Log if inference is slow
    if (last_inference_time_ms > 50) {
        ESP_LOGW(TAG, "⚠️  Slow inference: %lu ms", last_inference_time_ms);
    }
    
    return prob_near_miss;
}

bool ml_inference_is_ready(void) {
    return initialized;
}

uint32_t ml_inference_get_latency_ms(void) {
    return last_inference_time_ms;
}
```

### Step 5: Integrate into Main Application

**File**: `main/main.c` (add ML inference)

```c
#include "ml_inference.h"
#include <pickle.h>  // For scaler parameters

// Load scaler parameters (from Python scaler.pkl)
// You need to extract mean and std from scaler.pkl and hardcode here
static const float scaler_mean[25] = {
    // ToF means (16 values)
    450.0, 523.0, 612.0, 580.0, 490.0, 510.0, 520.0, 530.0,
    540.0, 550.0, 560.0, 570.0, 580.0, 590.0, 600.0, 610.0,
    // IMU means (9 values)
    0.0, 0.0, 0.0,  // roll, pitch, yaw
    0.0, 0.0, 9.81, // accel_x, accel_y, accel_z
    0.0, 0.0, 0.0   // gyro_x, gyro_y, gyro_z
};

static const float scaler_std[25] = {
    // ToF stds (16 values)
    200.0, 200.0, 200.0, 200.0, 200.0, 200.0, 200.0, 200.0,
    200.0, 200.0, 200.0, 200.0, 200.0, 200.0, 200.0, 200.0,
    // IMU stds (9 values)
    10.0, 10.0, 180.0,  // roll, pitch, yaw
    2.0, 2.0, 2.0,      // accel_x, accel_y, accel_z
    50.0, 50.0, 50.0    // gyro_x, gyro_y, gyro_z
};

void normalize_features(const tof_data_t *tof, const imu_data_t *imu, 
                       float features[25]) {
    int idx = 0;
    
    // ToF features (16)
    for (int i = 0; i < 16; i++) {
        features[idx] = (tof->distances[i] - scaler_mean[idx]) / scaler_std[idx];
        idx++;
    }
    
    // IMU features (9)
    features[idx++] = (imu->roll - scaler_mean[idx]) / scaler_std[idx];
    features[idx++] = (imu->pitch - scaler_mean[idx]) / scaler_std[idx];
    features[idx++] = (imu->yaw - scaler_mean[idx]) / scaler_std[idx];
    features[idx++] = (imu->accel_x - scaler_mean[idx]) / scaler_std[idx];
    features[idx++] = (imu->accel_y - scaler_mean[idx]) / scaler_std[idx];
    features[idx++] = (imu->accel_z - scaler_mean[idx]) / scaler_std[idx];
    features[idx++] = (imu->gyro_x - scaler_mean[idx]) / scaler_std[idx];
    features[idx++] = (imu->gyro_y - scaler_mean[idx]) / scaler_std[idx];
    features[idx++] = (imu->gyro_z - scaler_mean[idx]) / scaler_std[idx];
}

// In app_main()
void app_main(void) {
    // ... existing initialization ...
    
    // Initialize ML inference
    if (ml_inference_init()) {
        ESP_LOGI(TAG, "✅ ML inference enabled");
    } else {
        ESP_LOGW(TAG, "⚠️  ML inference disabled (continuing without ML)");
    }
    
    // ... rest of initialization ...
}

// In main loop
while (1) {
    // ... read sensors ...
    
    if (ml_inference_is_ready() && tof_count > 0 && imu_count > 0) {
        // Prepare features
        float features[25];
        normalize_features(&tof_data, &imu_data, features);
        
        // Run inference
        float near_miss_prob = ml_inference_predict(features);
        
        // Threshold for detection (tune this!)
        const float THRESHOLD = 0.70;  // 70% confidence
        
        if (near_miss_prob > THRESHOLD) {
            ESP_LOGW(TAG, "⚠️  NEAR-MISS DETECTED BY ML!");
            ESP_LOGW(TAG, "   Confidence: %.1f%%", near_miss_prob * 100);
            ESP_LOGW(TAG, "   Latency: %lu ms", ml_inference_get_latency_ms());
            
            // Set auto flag in log entry
            log_entry.flag_auto = 1;
            log_entry.confidence = near_miss_prob;
            
            // Visual/audio feedback
            // LED flash orange, buzzer beep, etc.
        }
    }
    
    // ... rest of loop ...
}
```

---

## Part 7: Testing and Tuning

### Test 1: Verify ML Inference Works

```bash
# Flash firmware
idf.py flash monitor
```

**Expected Serial Output:**
```
I (1234) ML_INFERENCE: Initializing ML inference...
I (1245) ML_INFERENCE: ✅ Model loaded (version 3)
I (1256) ML_INFERENCE: ✅ Tensors allocated
I (1256) ML_INFERENCE:    Input:  [1, 25]
I (1256) ML_INFERENCE:    Output: [1, 2]
I (1256) ML_INFERENCE:    Arena used: 8456 / 10240 bytes
I (1267) ML_INFERENCE: 🎉 ML inference ready!
```

### Test 2: Monitor Inference Performance

Watch for inference latency in serial output:
```
I (5678) MAIN: ⚠️  NEAR-MISS DETECTED BY ML!
I (5678) MAIN:    Confidence: 85.3%
I (5678) MAIN:    Latency: 12 ms
```

**Target Performance:**
- ✅ Latency < 20ms (should be ~10-15ms with ESP-NN)
- ✅ Detection rate > 75%
- ✅ False alarm rate < 5%

### Test 3: Tune Detection Threshold

Adjust threshold based on field testing:

```c
// Conservative (fewer false alarms, may miss some events)
const float THRESHOLD = 0.80;  // 80% confidence

// Balanced (recommended starting point)
const float THRESHOLD = 0.70;  // 70% confidence

// Aggressive (catch more events, more false alarms)
const float THRESHOLD = 0.60;  // 60% confidence
```

---

## Part 8: Troubleshooting

### Issue: Model won't load

**Symptoms:**
```
E (1234) ML_INFERENCE: Model schema version mismatch!
```

**Solution:**
- Ensure TensorFlow version matches (2.15.0)
- Regenerate TFLite model with correct version
- Check model_data.h is not corrupted

### Issue: Inference is slow (>50ms)

**Symptoms:**
```
W (5678) ML_INFERENCE: ⚠️  Slow inference: 67 ms
```

**Solutions:**
1. Enable int8 quantization (use `convert_to_tflite.py` with `quantize=True`)
2. Reduce model size (fewer hidden neurons)
3. Increase tensor arena size
4. Verify ESP-NN is enabled in sdkconfig

### Issue: Poor detection rate

**Symptoms:**
- Model misses obvious near-miss events
- Detection rate < 70%

**Solutions:**
1. Collect more near-miss training examples (aim for 200+)
2. Collect more diverse examples (different scenarios)
3. Lower detection threshold (e.g., 0.60 instead of 0.70)
4. Retrain model with different architecture

### Issue: Too many false alarms

**Symptoms:**
- Model triggers on normal riding
- False alarm rate > 10%

**Solutions:**
1. Collect more diverse safe examples
2. Increase detection threshold (e.g., 0.80 instead of 0.70)
3. Review training data for mislabeled examples
4. Add more features (derived features like approach rate)

---

## Summary

**Complete Workflow:**

1. ✅ **PC Setup** - Install Python, TensorFlow, required packages
2. ✅ **Data Collection** - Ride bike, press button during near-misses
3. ✅ **Data Extraction** - Copy CSV files from SD card via USB
4. ✅ **Data Preparation** - Load, balance, normalize, split dataset
5. ✅ **Model Training** - Train neural network with TensorFlow
6. ✅ **Model Conversion** - Convert to TFLite format
7. ✅ **ESP32 Deployment** - Copy model, implement inference, flash firmware
8. ✅ **Testing & Tuning** - Test in field, adjust threshold, iterate

**Key Points:**
- ✅ Use **TensorFlow** (not PyTorch) - ESP-NN requires TFLite
- ✅ Collect **diverse examples** - Different scenarios, conditions
- ✅ **Balance dataset** - 10:1 ratio (safe:near-miss)
- ✅ **Normalize features** - Critical for neural network training
- ✅ **Test thoroughly** - Verify accuracy before deployment
- ✅ **Tune threshold** - Balance detection rate vs false alarms

**Next Steps:**
1. Start data collection (aim for 100-200 near-miss examples)
2. Run training pipeline when you have enough data
3. Deploy model to ESP32 and test in field
4. Iterate based on performance

---

**Document Version:** 1.0  
**Last Updated:** January 31, 2026  
**Status:** ✅ Complete ML training guide

