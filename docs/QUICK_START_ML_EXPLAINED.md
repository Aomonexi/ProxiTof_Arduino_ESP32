# Quick Start ML Explained - What Happens Inside

**Date**: January 31, 2026  
**Purpose**: Detailed explanation of what `quick_start_ml.py` does

---

## Overview

`quick_start_ml.py` is a **complete automated pipeline** that takes you from raw CSV files to a deployable ESP32 model in one command.

**What it does:**
1. ✅ Loads CSV files from SD card
2. ✅ Prepares training data (balance, normalize, split)
3. ✅ Trains neural network
4. ✅ Converts to TensorFlow Lite
5. ✅ Generates C header file for ESP32

**One command:**
```bash
python tools/quick_start_ml.py
```

---

## Step-by-Step Breakdown

### Step 1: Load Data

**What happens:**
```python
from load_data import load_csv_files, inspect_data

df = load_csv_files('../data/raw')
df = inspect_data(df)
```

**Actions:**
1. Scans `data/raw/` directory for CSV files
2. Loads all CSV files into memory
3. Combines into single DataFrame
4. Checks for missing values
5. Analyzes label distribution (near-miss vs safe)
6. Prints data statistics

**Example output:**
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
Columns: 28
Memory usage: 3.45 MB

LABEL DISTRIBUTION
----------------------------------------------------------
Near-miss (flag_manual=1): 156 (1.02%)
Safe (flag_manual=0):      15078 (98.98%)
Ratio (safe:near-miss):    96.7:1

SENSOR DATA RANGES
----------------------------------------------------------
ToF distances:  min=50mm, max=4000mm, mean=1234mm
Roll:           min=-45.2°, max=42.8°
Pitch:          min=-38.5°, max=35.2°
Acceleration:   min=8.5m/s², max=12.3m/s²

DATA QUALITY
----------------------------------------------------------
✅ No missing values
✅ No duplicate rows

✅ Combined dataset saved to ../data/processed/combined_data.csv
```

**What to check:**
- ✅ At least 100-200 near-miss examples
- ✅ At least 1000-2000 safe examples
- ✅ No missing values in critical columns
- ✅ Sensor values in reasonable ranges

---

### Step 2: Prepare Training Data

**What happens:**
```python
from prepare_training_data import prepare_dataset

prepare_dataset(
    csv_file='../data/processed/combined_data.csv',
    output_dir='../data/processed',
    balance_ratio=10,
    test_size=0.2,
    random_state=42
)
```

**Actions:**

#### 2.1 Extract Features and Labels

```python
# Extract 25 features (16 ToF + 9 IMU)
tof_cols = [f'tof_{row}{col}' for row in range(4) for col in range(4)]
imu_cols = ['roll', 'pitch', 'yaw', 'accel_x', 'accel_y', 'accel_z',
            'gyro_x', 'gyro_y', 'gyro_z']
feature_cols = tof_cols + imu_cols

X = df[feature_cols].values  # Shape: (15234, 25)
y = df['flag_manual'].values  # Shape: (15234,)
```

**Why these features?**
- ToF: Detects obstacles and distances
- IMU: Detects bike motion and orientation
- flag_manual: User's judgment of near-miss events

#### 2.2 Balance Dataset

```python
# Original: 15078 safe, 156 near-miss (96.7:1 ratio)
# Target: 10:1 ratio

# Keep all 156 near-miss samples
# Randomly select 1560 safe samples (156 × 10)
# Result: 1716 total samples (1560 safe + 156 near-miss)
```

**Why balance?**
- Neural networks learn better with balanced data
- Prevents model from just predicting "safe" all the time
- 10:1 ratio is realistic (most riding is safe)

**Example output:**
```
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
```

#### 2.3 Split Train/Test

```python
# Split 80% training, 20% testing
X_train, X_test, y_train, y_test = train_test_split(
    X_balanced, y_balanced, 
    test_size=0.2, 
    stratify=y_balanced  # Maintain class ratio in both sets
)

# Result:
# Training: 1372 samples (80%)
# Test: 344 samples (20%)
```

**Why split?**
- Training set: Model learns from this
- Test set: Evaluate model on unseen data
- Prevents overfitting (memorizing training data)

#### 2.4 Normalize Features

```python
# Before normalization:
# ToF: 50-4000mm, IMU: -45° to +45°, Accel: 8-12 m/s²
# Different scales make training difficult!

scaler = StandardScaler()
X_train_norm = scaler.fit_transform(X_train)
X_test_norm = scaler.transform(X_test)

# After normalization:
# All features: mean=0, std=1
# Same scale makes training easier!
```

**Why normalize?**
- Neural networks work best with normalized data
- Prevents large values from dominating
- Speeds up training convergence

**Example output:**
```
==============================================================
FEATURE NORMALIZATION
==============================================================
Training set:
  Before: mean=123.45, std=456.78
  After:  mean=0.00, std=1.00
```

#### 2.5 Save Processed Data

```python
# Save for training
np.save('X_train.npy', X_train_norm)
np.save('X_test.npy', X_test_norm)
np.save('y_train.npy', y_train)
np.save('y_test.npy', y_test)

# Save scaler (needed for deployment!)
pickle.dump(scaler, open('scaler.pkl', 'wb'))

# Save feature names
with open('feature_names.txt', 'w') as f:
    for col in feature_cols:
        f.write(f"{col}\n")
```

**Why save scaler?**
- ESP32 needs to normalize features the same way
- Scaler contains mean and std for each feature
- Must use same normalization in deployment!

---

### Step 3: Train Model

**What happens:**
```python
from train_model import train_model, evaluate_model, plot_training_history

# Load prepared data
X_train = np.load('../data/processed/X_train.npy')
X_test = np.load('../data/processed/X_test.npy')
y_train = np.load('../data/processed/y_train.npy')
y_test = np.load('../data/processed/y_test.npy')

# Train
model, history = train_model(X_train, y_train, X_test, y_test, 
                             epochs=100, batch_size=32)
```

**Actions:**

#### 3.1 Create Model Architecture

```python
model = keras.Sequential([
    keras.layers.Input(shape=(25,)),           # 25 input features
    keras.layers.Dense(16, activation='relu'), # 16 hidden neurons
    keras.layers.Dropout(0.2),                 # Prevent overfitting
    keras.layers.Dense(2, activation='softmax') # 2 outputs (safe, near-miss)
])
```

**Why this architecture?**
- Small enough to fit in ESP32 memory (~450 parameters)
- Uses ESP-NN accelerated operations (Dense, ReLU)
- Simple enough to run at 60Hz on ESP32
- Proven effective for binary classification

**Model size:**
- Input layer: 25 features
- Hidden layer: 25 × 16 + 16 = 416 parameters
- Output layer: 16 × 2 + 2 = 34 parameters
- Total: 450 parameters (~2KB model)

#### 3.2 Compile Model

```python
model.compile(
    optimizer='adam',                      # Adaptive learning rate
    loss='sparse_categorical_crossentropy', # For classification
    metrics=['accuracy']                   # Track accuracy
)
```

#### 3.3 Calculate Class Weights

```python
# Handle imbalanced data (10:1 ratio)
n_samples = 1372
n_safe = 1234
n_near_miss = 138

class_weight = {
    0: n_samples / (2 * n_safe),      # Safe: 0.556
    1: n_samples / (2 * n_near_miss)  # Near-miss: 4.971
}
```

**Why class weights?**
- Penalizes model more for missing near-miss events
- Compensates for imbalanced data
- Improves detection rate

#### 3.4 Train with Callbacks

```python
callbacks = [
    # Stop if no improvement for 15 epochs
    EarlyStopping(monitor='val_loss', patience=15),
    
    # Save best model
    ModelCheckpoint('best_model.h5', save_best_only=True),
    
    # Reduce learning rate if stuck
    ReduceLROnPlateau(monitor='val_loss', factor=0.5, patience=5)
]

history = model.fit(
    X_train, y_train,
    validation_data=(X_test, y_test),
    epochs=100,
    batch_size=32,
    class_weight=class_weight,
    callbacks=callbacks
)
```

**Example output:**
```
==============================================================
MODEL TRAINING
==============================================================

Model Architecture:
_________________________________________________________________
 Layer (type)                Output Shape              Param #   
=================================================================
 hidden (Dense)              (None, 16)                416       
 dropout (Dropout)           (None, 16)                0         
 output (Dense)              (None, 2)                 34        
=================================================================
Total params: 450
Trainable params: 450

Class weights:
  Safe (0):      0.556
  Near-miss (1): 4.971

Training for up to 100 epochs...
Epoch 1/100
43/43 [==============================] - 1s 12ms/step - loss: 0.6234 - accuracy: 0.6789 - val_loss: 0.5123 - val_accuracy: 0.7456
Epoch 2/100
43/43 [==============================] - 0s 8ms/step - loss: 0.4567 - accuracy: 0.7890 - val_loss: 0.3456 - val_accuracy: 0.8567
...
Epoch 45/100
43/43 [==============================] - 0s 8ms/step - loss: 0.1234 - accuracy: 0.9567 - val_loss: 0.1456 - val_accuracy: 0.9456

Restoring model weights from the end of the best epoch: 30.
Epoch 45: early stopping
```

**What to watch:**
- ✅ Accuracy increasing (should reach >90%)
- ✅ Loss decreasing (should reach <0.2)
- ✅ Validation accuracy close to training accuracy
- ⚠️ If validation accuracy much lower → overfitting

#### 3.5 Evaluate Model

```python
metrics = evaluate_model(model, X_test, y_test)
```

**Example output:**
```
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
```

**Understanding metrics:**

- **Accuracy (94.56%)**: Overall correctness
  - Good, but can be misleading with imbalanced data
  
- **Detection Rate (76.47%)**: Recall for near-miss class
  - "Of all actual near-misses, how many did we detect?"
  - Higher is better (aim for >75%)
  
- **False Alarm Rate (1.94%)**: False positives
  - "Of all safe situations, how many did we incorrectly flag?"
  - Lower is better (aim for <5%)
  
- **Precision (81.25%)**: Positive predictive value
  - "When model says near-miss, how often is it correct?"
  - Higher is better (aim for >80%)
  
- **F1 Score (0.7966)**: Harmonic mean of precision and recall
  - Balances detection rate and precision
  - Higher is better (aim for >0.75)

#### 3.6 Plot Training History

```python
plot_training_history(history)
```

Generates `training_history.png` showing:
- Accuracy over epochs (training vs validation)
- Loss over epochs (training vs validation)

**What to look for:**
- ✅ Both curves increasing/decreasing together
- ⚠️ Large gap between training and validation → overfitting
- ✅ Curves plateau → model converged

---

### Step 4: Convert to TensorFlow Lite

**What happens:**
```python
from convert_to_tflite import convert_to_tflite, test_tflite_model, generate_c_header

# Convert to TFLite (float32)
convert_to_tflite('../models/near_miss_model.h5', 
                 '../models/near_miss_model.tflite', 
                 quantize=False)

# Test TFLite model
test_tflite_model('../models/near_miss_model.tflite')

# Generate C header
generate_c_header('../models/near_miss_model.tflite',
                 '../components/ml_inference/model_data.h')
```

**Actions:**

#### 4.1 Convert Keras to TFLite

```python
# Load Keras model
model = tf.keras.models.load_model('near_miss_model.h5')

# Create converter
converter = tf.lite.TFLiteConverter.from_keras_model(model)

# Apply optimizations
converter.optimizations = [tf.lite.Optimize.DEFAULT]

# Convert
tflite_model = converter.convert()

# Save
with open('near_miss_model.tflite', 'wb') as f:
    f.write(tflite_model)
```

**Why TFLite?**
- Optimized for embedded devices
- Smaller model size (~2-3KB vs ~10KB for Keras)
- Faster inference on ESP32
- Compatible with ESP-NN acceleration

**Example output:**
```
==============================================================
CONVERTING TO TENSORFLOW LITE
==============================================================
Input model: ../models/near_miss_model.h5
Output model: ../models/near_miss_model.tflite
Quantization: Disabled (float32)

Loading Keras model...
✅ Model loaded

Converting to TFLite...
✅ TFLite model saved

Model size: 2,456 bytes (2.40 KB)
```

#### 4.2 Test TFLite Model

```python
# Load TFLite model
interpreter = tf.lite.Interpreter(model_path='near_miss_model.tflite')
interpreter.allocate_tensors()

# Run inference on test set
for i in range(len(X_test)):
    interpreter.set_tensor(input_index, X_test[i:i+1])
    interpreter.invoke()
    output = interpreter.get_tensor(output_index)
    predictions.append(output[0])

# Calculate accuracy
accuracy = np.mean(predictions == y_test)
```

**Example output:**
```
==============================================================
TESTING TFLITE MODEL
==============================================================

Input details:
  Shape: [1, 25]
  Type:  <class 'numpy.float32'>

Output details:
  Shape: [1, 2]
  Type:  <class 'numpy.float32'>

Testing on 344 samples...

✅ TFLite model accuracy: 94.48%
Keras model accuracy:  94.56%
Accuracy difference:   0.08%

✅ TFLite model matches Keras model accuracy
```

**What to check:**
- ✅ Accuracy difference < 1% (conversion didn't break model)
- ✅ Model size < 10KB (fits in ESP32 memory)
- ✅ Input/output shapes correct ([1, 25] → [1, 2])

#### 4.3 Generate C Header File

```python
# Read TFLite model as bytes
with open('near_miss_model.tflite', 'rb') as f:
    tflite_model = f.read()

# Generate C array
with open('model_data.h', 'w') as f:
    f.write("#ifndef MODEL_DATA_H\n")
    f.write("#define MODEL_DATA_H\n\n")
    f.write(f"const unsigned int model_data_len = {len(tflite_model)};\n\n")
    f.write("const unsigned char model_data[] = {\n")
    
    for i in range(0, len(tflite_model), 12):
        chunk = tflite_model[i:i+12]
        hex_values = ', '.join([f'0x{b:02x}' for b in chunk])
        f.write(f"  {hex_values},\n")
    
    f.write("};\n\n")
    f.write("#endif\n")
```

**Result**: `model_data.h` file containing:
```c
#ifndef MODEL_DATA_H
#define MODEL_DATA_H

const unsigned int model_data_len = 2456;

const unsigned char model_data[] = {
  0x1c, 0x00, 0x00, 0x00, 0x54, 0x46, 0x4c, 0x33, 0x00, 0x00, 0x12, 0x00,
  0x1c, 0x00, 0x04, 0x00, 0x08, 0x00, 0x0c, 0x00, 0x10, 0x00, 0x14, 0x00,
  ...
};

#endif
```

**Example output:**
```
==============================================================
GENERATING C HEADER FILE
==============================================================
✅ C header file saved to ../components/ml_inference/model_data.h

Copy this file to your ESP32 project:
  cp ../components/ml_inference/model_data.h <your_project>/components/ml_inference/include/
```

---

## Final Output

### Generated Files

```
models/
├── near_miss_model.h5           # Keras model (10KB)
├── near_miss_model.tflite       # TFLite model (2.4KB)
├── training_history.png         # Training plot
└── best_model.h5                # Best checkpoint

data/processed/
├── combined_data.csv            # All CSV files combined
├── X_train.npy                  # Training features
├── X_test.npy                   # Test features
├── y_train.npy                  # Training labels
├── y_test.npy                   # Test labels
├── scaler.pkl                   # Feature scaler
└── feature_names.txt            # Feature column names

components/ml_inference/
└── model_data.h                 # C header for ESP32
```

### Summary Output

```
==============================================================
🎉 ML TRAINING PIPELINE COMPLETE!
==============================================================

Generated files:
  ✅ ../models/near_miss_model.h5 - Trained Keras model
  ✅ ../models/near_miss_model.tflite - TFLite model
  ✅ ../models/training_history.png - Training plot
  ✅ ../components/ml_inference/model_data.h - C header for ESP32

Next steps:
  1. Copy model_data.h to your ESP32 project
  2. Implement ML inference component (see docs/ML_TRAINING_COMPLETE_GUIDE.md)
  3. Flash firmware: idf.py flash monitor
  4. Test in field and tune threshold
```

---

## What Makes This "Quick Start"?

### Without quick_start_ml.py

You would need to run 4 separate scripts:

```bash
# Step 1: Load data
python tools/load_data.py

# Step 2: Prepare data
python tools/prepare_training_data.py

# Step 3: Train model
python tools/train_model.py

# Step 4: Convert to TFLite
python tools/convert_to_tflite.py
```

**Time**: ~15-20 minutes (manual intervention between steps)

### With quick_start_ml.py

One command does everything:

```bash
python tools/quick_start_ml.py
```

**Time**: ~5-10 minutes (fully automated)

---

## When to Use Each Script

### Use quick_start_ml.py when:
- ✅ First time training
- ✅ Want complete pipeline
- ✅ Don't need to customize parameters
- ✅ Just want a working model quickly

### Use individual scripts when:
- ✅ Need to customize parameters
- ✅ Want to experiment with different architectures
- ✅ Need to debug specific steps
- ✅ Want to iterate on one step only

**Example**: If you want to try different model architectures:
```bash
# Use quick_start_ml.py to prepare data
python tools/quick_start_ml.py  # Stop after Step 2

# Then manually train with different architectures
python tools/train_model.py --hidden_size 32  # Try 32 neurons
python tools/train_model.py --hidden_size 8   # Try 8 neurons
```

---

## Troubleshooting

### Issue: "No CSV files found"

**Solution:**
```bash
# 1. Check data directory exists
ls data/raw/

# 2. Copy CSV files from SD card
# Activate USB mode on device (Mode 7, press ACTION)
# Connect USB cable to LEFT port
# Copy *.csv files to data/raw/

# 3. Run again
python tools/quick_start_ml.py
```

### Issue: "Not enough near-miss examples"

**Output:**
```
⚠️  WARNING: Only 45 near-miss examples!
   Recommendation: Collect at least 100-200 near-miss examples
```

**Solution:**
- Collect more data (ride more, press button during near-misses)
- Aim for 100-200 near-miss examples minimum
- More data = better model!

### Issue: "Poor model performance"

**Output:**
```
⚠️  WARNING: Detection rate is low (<80%)
   Consider: More training data, different features, or model tuning
```

**Solutions:**
1. Collect more diverse training data
2. Try different model architecture (more neurons)
3. Adjust class weights
4. Lower detection threshold

### Issue: "TensorFlow version mismatch"

**Output:**
```
E (1234) ML_INFERENCE: Model schema version mismatch!
```

**Solution:**
```bash
# Ensure TensorFlow 2.x
pip install "tensorflow>=2.15,<3.0"

# Regenerate model
python tools/quick_start_ml.py
```

---

## Summary

### What quick_start_ml.py Does

1. **Loads** CSV files from SD card
2. **Inspects** data quality and distribution
3. **Extracts** 25 features (16 ToF + 9 IMU)
4. **Balances** dataset (10:1 ratio)
5. **Normalizes** features (mean=0, std=1)
6. **Splits** train/test (80%/20%)
7. **Trains** neural network (450 parameters)
8. **Evaluates** performance (accuracy, detection rate, etc.)
9. **Converts** to TensorFlow Lite
10. **Generates** C header file for ESP32

### Key Benefits

- ✅ **Automated** - One command does everything
- ✅ **Fast** - 5-10 minutes total
- ✅ **Validated** - Tests model at each step
- ✅ **Production-ready** - Generates deployable model
- ✅ **Documented** - Prints detailed progress

### Next Steps

1. Run `python tools/quick_start_ml.py`
2. Copy `model_data.h` to ESP32 project
3. Implement ML inference (see `docs/ESPNN_IMPLEMENTATION_GUIDE.md`)
4. Flash and test!

---

**Document Version:** 1.0  
**Last Updated:** January 31, 2026  
**Status:** ✅ Complete explanation of quick_start_ml.py
