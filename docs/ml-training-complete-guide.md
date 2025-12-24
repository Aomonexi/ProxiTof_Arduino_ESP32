# ProxiTof ML Training Guide - Complete TensorFlow Pipeline

## Overview

This guide covers the complete machine learning pipeline for the ProxiTof Near-Miss Detection System, from raw sensor data to deployed TensorFlow Lite model on ESP32-S3.

## Data Collection Phase

### 1. CSV Data Format (from ESP32-S3)

Your ESP32-S3 system generates CSV files with this structure:
```csv
timestamp_ms,tof_00,tof_01,...,tof_33,roll,pitch,yaw,accel_x,accel_y,accel_z,gyro_x,gyro_y,gyro_z,min_distance_mm,approach_rate_mm_s,jerk_magnitude_m_s3,angular_velocity_deg_s,flag_manual,flag_auto,confidence
```

**Features (25 total):**
- 16 ToF distances (tof_00 to tof_33) - 4x4 grid in mm
- 3 orientation values (roll, pitch, yaw) - degrees
- 3 acceleration values (accel_x, accel_y, accel_z) - m/s²
- 3 gyroscope values (gyro_x, gyro_y, gyro_z) - deg/s

**Labels:**
- `flag_manual`: 1 = near-miss event (manually flagged), 0 = normal
- `flag_auto`: 1 = ML detected event, 0 = normal (used for validation)

### 2. Data Collection Requirements

**Minimum Dataset Size:**
- 10+ hours of diverse scenarios
- 200+ manually flagged near-miss events
- 10,000+ normal samples for balance

**Scenarios to Collect:**
- Walking at different speeds
- Running/jogging
- Static obstacles (walls, furniture)
- Moving obstacles (people, vehicles)
- Different lighting conditions
- Various distances (0.1m to 4m)

## TensorFlow Training Pipeline

### Step 1: Environment Setup

```python
# Create and activate virtual environment
python -m venv proxitof_ml
# Windows: proxitof_ml\Scripts\activate
# Linux/Mac: source proxitof_ml/bin/activate

# Install requirements
pip install -r requirements_proxitof_thesis.txt
```

### Step 2: Data Loading and Preprocessing

```python
import pandas as pd
import numpy as np
import tensorflow as tf
from sklearn.model_selection import train_test_split
from sklearn.preprocessing import StandardScaler
import matplotlib.pyplot as plt

def load_proxitof_data(data_directory):
    """Load all CSV files from data collection"""
    import glob
    import os
    
    csv_files = glob.glob(os.path.join(data_directory, "**/*.csv"), recursive=True)
    dataframes = []
    
    for file in csv_files:
        try:
            df = pd.read_csv(file)
            dataframes.append(df)
            print(f"Loaded {len(df)} samples from {file}")
        except Exception as e:
            print(f"Error loading {file}: {e}")
    
    # Combine all data
    combined_df = pd.concat(dataframes, ignore_index=True)
    print(f"Total samples: {len(combined_df)}")
    return combined_df

def preprocess_data(df):
    """Clean and prepare data for training"""
    
    # Define feature columns (25 features as per your spec)
    feature_columns = [
        # ToF distances (16 features)
        'tof_00', 'tof_01', 'tof_02', 'tof_03',
        'tof_10', 'tof_11', 'tof_12', 'tof_13', 
        'tof_20', 'tof_21', 'tof_22', 'tof_23',
        'tof_30', 'tof_31', 'tof_32', 'tof_33',
        # IMU orientation (3 features)
        'roll', 'pitch', 'yaw',
        # IMU acceleration (3 features) 
        'accel_x', 'accel_y', 'accel_z',
        # IMU gyroscope (3 features)
        'gyro_x', 'gyro_y', 'gyro_z'
    ]
    
    # Extract features and labels
    X = df[feature_columns].copy()
    y = df['flag_manual'].copy()  # Use manual flags as ground truth
    
    # Handle missing values
    X = X.fillna(method='ffill').fillna(0)
    
    # Remove outliers (ToF sensor max range is 4000mm)
    tof_columns = [col for col in feature_columns if col.startswith('tof_')]
    X[tof_columns] = X[tof_columns].clip(0, 4000)
    
    # Remove invalid samples
    valid_mask = ~(X.isnull().any(axis=1) | y.isnull())
    X = X[valid_mask]
    y = y[valid_mask]
    
    print(f"After preprocessing: {len(X)} samples")
    print(f"Near-miss events: {y.sum()} ({y.mean()*100:.1f}%)")
    
    return X, y

# Load your data
df = load_proxitof_data("path/to/your/sd_card_data/")
X, y = preprocess_data(df)
```

### Step 3: Feature Engineering and Normalization

```python
def create_features(X):
    """Create additional engineered features"""
    X_enhanced = X.copy()
    
    # ToF-based features
    tof_cols = [col for col in X.columns if col.startswith('tof_')]
    X_enhanced['min_distance'] = X[tof_cols].min(axis=1)
    X_enhanced['max_distance'] = X[tof_cols].max(axis=1)
    X_enhanced['distance_std'] = X[tof_cols].std(axis=1)
    X_enhanced['distance_range'] = X_enhanced['max_distance'] - X_enhanced['min_distance']
    
    # IMU-based features
    X_enhanced['accel_magnitude'] = np.sqrt(
        X['accel_x']**2 + X['accel_y']**2 + X['accel_z']**2
    )
    X_enhanced['gyro_magnitude'] = np.sqrt(
        X['gyro_x']**2 + X['gyro_y']**2 + X['gyro_z']**2
    )
    
    return X_enhanced

def normalize_features(X_train, X_val, X_test):
    """Normalize features for neural network training"""
    
    # Separate ToF and IMU features for different normalization
    tof_cols = [col for col in X_train.columns if col.startswith('tof_') or 'distance' in col]
    orientation_cols = ['roll', 'pitch', 'yaw']
    accel_cols = ['accel_x', 'accel_y', 'accel_z']
    gyro_cols = ['gyro_x', 'gyro_y', 'gyro_z']
    other_cols = [col for col in X_train.columns if col not in tof_cols + orientation_cols + accel_cols + gyro_cols]
    
    # Create scalers for different feature types
    scalers = {}
    
    # ToF distances: normalize to [0, 1] based on sensor range (0-4000mm)
    scalers['tof'] = lambda x: x / 4000.0
    
    # Orientation: normalize to [-1, 1] based on ±180 degrees
    scalers['orientation'] = lambda x: x / 180.0
    
    # Acceleration: normalize to [-1, 1] based on ±10 m/s²
    scalers['accel'] = lambda x: np.clip(x / 10.0, -1, 1)
    
    # Gyroscope: normalize to [-1, 1] based on ±500 deg/s
    scalers['gyro'] = lambda x: np.clip(x / 500.0, -1, 1)
    
    # Other features: standard scaling
    scaler_other = StandardScaler()
    if other_cols:
        scaler_other.fit(X_train[other_cols])
        scalers['other'] = scaler_other
    
    def apply_scaling(X):
        X_scaled = X.copy()
        X_scaled[tof_cols] = scalers['tof'](X_scaled[tof_cols])
        X_scaled[orientation_cols] = scalers['orientation'](X_scaled[orientation_cols])
        X_scaled[accel_cols] = scalers['accel'](X_scaled[accel_cols])
        X_scaled[gyro_cols] = scalers['gyro'](X_scaled[gyro_cols])
        if other_cols:
            X_scaled[other_cols] = scalers['other'].transform(X_scaled[other_cols])
        return X_scaled
    
    return apply_scaling(X_train), apply_scaling(X_val), apply_scaling(X_test), scalers

# Apply feature engineering
X_enhanced = create_features(X)

# Split data
X_train, X_temp, y_train, y_temp = train_test_split(
    X_enhanced, y, test_size=0.3, random_state=42, stratify=y
)
X_val, X_test, y_val, y_test = train_test_split(
    X_temp, y_temp, test_size=0.5, random_state=42, stratify=y_temp
)

# Normalize features
X_train_norm, X_val_norm, X_test_norm, scalers = normalize_features(X_train, X_val, X_test)

print(f"Training set: {len(X_train_norm)} samples")
print(f"Validation set: {len(X_val_norm)} samples") 
print(f"Test set: {len(X_test_norm)} samples")
```

### Step 4: Model Architecture Design

```python
def create_proxitof_model(input_shape, dropout_rate=0.3):
    """Create neural network optimized for ESP32-S3 deployment"""
    
    model = tf.keras.Sequential([
        # Input layer
        tf.keras.layers.Input(shape=input_shape),
        
        # First hidden layer - larger for feature extraction
        tf.keras.layers.Dense(64, activation='relu', name='dense_1'),
        tf.keras.layers.Dropout(dropout_rate),
        
        # Second hidden layer - smaller for compression
        tf.keras.layers.Dense(32, activation='relu', name='dense_2'),
        tf.keras.layers.Dropout(dropout_rate),
        
        # Third hidden layer - even smaller
        tf.keras.layers.Dense(16, activation='relu', name='dense_3'),
        tf.keras.layers.Dropout(dropout_rate),
        
        # Output layer - binary classification
        tf.keras.layers.Dense(1, activation='sigmoid', name='output')
    ])
    
    return model

# Create model
input_shape = (X_train_norm.shape[1],)
model = create_proxitof_model(input_shape)

# Compile model
model.compile(
    optimizer=tf.keras.optimizers.Adam(learning_rate=0.001),
    loss='binary_crossentropy',
    metrics=['accuracy', 'precision', 'recall']
)

model.summary()
```

### Step 5: Training with Class Balancing

```python
from sklearn.utils.class_weight import compute_class_weight

# Handle class imbalance
class_weights = compute_class_weight(
    'balanced',
    classes=np.unique(y_train),
    y=y_train
)
class_weight_dict = {i: class_weights[i] for i in range(len(class_weights))}

print(f"Class weights: {class_weight_dict}")

# Callbacks for training
callbacks = [
    tf.keras.callbacks.EarlyStopping(
        monitor='val_loss',
        patience=10,
        restore_best_weights=True
    ),
    tf.keras.callbacks.ReduceLROnPlateau(
        monitor='val_loss',
        factor=0.5,
        patience=5,
        min_lr=1e-6
    ),
    tf.keras.callbacks.ModelCheckpoint(
        'best_proxitof_model.h5',
        monitor='val_loss',
        save_best_only=True
    )
]

# Train model
history = model.fit(
    X_train_norm, y_train,
    validation_data=(X_val_norm, y_val),
    epochs=100,
    batch_size=32,
    class_weight=class_weight_dict,
    callbacks=callbacks,
    verbose=1
)
```

### Step 6: Model Evaluation

```python
from sklearn.metrics import classification_report, confusion_matrix
import seaborn as sns

# Evaluate on test set
test_predictions = model.predict(X_test_norm)
test_pred_binary = (test_predictions > 0.5).astype(int)

print("Test Set Evaluation:")
print(classification_report(y_test, test_pred_binary))

# Confusion matrix
cm = confusion_matrix(y_test, test_pred_binary)
plt.figure(figsize=(8, 6))
sns.heatmap(cm, annot=True, fmt='d', cmap='Blues')
plt.title('Confusion Matrix - ProxiTof Near-Miss Detection')
plt.ylabel('True Label')
plt.xlabel('Predicted Label')
plt.show()

# Plot training history
fig, axes = plt.subplots(2, 2, figsize=(12, 8))

axes[0,0].plot(history.history['loss'], label='Training Loss')
axes[0,0].plot(history.history['val_loss'], label='Validation Loss')
axes[0,0].set_title('Model Loss')
axes[0,0].legend()

axes[0,1].plot(history.history['accuracy'], label='Training Accuracy')
axes[0,1].plot(history.history['val_accuracy'], label='Validation Accuracy')
axes[0,1].set_title('Model Accuracy')
axes[0,1].legend()

axes[1,0].plot(history.history['precision'], label='Training Precision')
axes[1,0].plot(history.history['val_precision'], label='Validation Precision')
axes[1,0].set_title('Model Precision')
axes[1,0].legend()

axes[1,1].plot(history.history['recall'], label='Training Recall')
axes[1,1].plot(history.history['val_recall'], label='Validation Recall')
axes[1,1].set_title('Model Recall')
axes[1,1].legend()

plt.tight_layout()
plt.show()
```

### Step 7: Convert to LiteRT (formerly TensorFlow Lite) for ESP32-S3

```python
def convert_to_litert(model, X_sample):
    """Convert Keras model to LiteRT (formerly TensorFlow Lite) with quantization"""
    
    # Note: Google is migrating TensorFlow Lite to ai-edge-litert
    # For now, use TensorFlow's built-in converter
    converter = tf.lite.TFLiteConverter.from_keras_model(model)
    
    # Enable optimizations for smaller model size
    converter.optimizations = [tf.lite.Optimize.DEFAULT]
    
    # Representative dataset for quantization
    def representative_dataset():
        for i in range(100):
            # Use random samples from training data
            idx = np.random.randint(0, len(X_sample))
            yield [X_sample[idx:idx+1].astype(np.float32)]
    
    # Set quantization options
    converter.representative_dataset = representative_dataset
    converter.target_spec.supported_ops = [tf.lite.OpsSet.TFLITE_BUILTINS_INT8]
    converter.inference_input_type = tf.float32
    converter.inference_output_type = tf.float32
    
    # Convert model
    litert_model = converter.convert()
    
    return litert_model

# Convert model
litert_model = convert_to_litert(model, X_train_norm)

# Save LiteRT model (still uses .tflite extension for ESP32-S3 compatibility)
with open('proxitof_model.tflite', 'wb') as f:
    f.write(litert_model)

print(f"LiteRT model size: {len(litert_model) / 1024:.1f} KB")

# Test LiteRT model
interpreter = tf.lite.Interpreter(model_content=litert_model)
interpreter.allocate_tensors()

# Get input and output details
input_details = interpreter.get_input_details()
output_details = interpreter.get_output_details()

print("Input shape:", input_details[0]['shape'])
print("Output shape:", output_details[0]['shape'])

# Test inference
test_sample = X_test_norm.iloc[0:1].values.astype(np.float32)
interpreter.set_tensor(input_details[0]['index'], test_sample)
interpreter.invoke()
litert_prediction = interpreter.get_tensor(output_details[0]['index'])

print(f"Original model prediction: {model.predict(test_sample)[0][0]:.4f}")
print(f"LiteRT model prediction: {litert_prediction[0][0]:.4f}")
```

### Step 8: Generate Model Header for ESP32-S3

```python
def generate_model_header(litert_model, header_filename='proxitof_model.h'):
    """Generate C header file for ESP32-S3 deployment"""
    
    model_size = len(litert_model)
    
    header_content = f"""// ProxiTof Near-Miss Detection Model
// Generated automatically from LiteRT (formerly TensorFlow Lite) model
// Model size: {model_size} bytes

#ifndef PROXITOF_MODEL_H
#define PROXITOF_MODEL_H

#include <stdint.h>

// Model data
const unsigned int proxitof_model_len = {model_size};
const unsigned char proxitof_model_data[] = {{
"""
    
    # Convert model bytes to C array
    for i, byte in enumerate(litert_model):
        if i % 12 == 0:
            header_content += "\n  "
        header_content += f"0x{byte:02x},"
    
    header_content = header_content.rstrip(',')  # Remove last comma
    
    header_content += """
};

#endif // PROXITOF_MODEL_H
"""
    
    # Write header file
    with open(header_filename, 'w') as f:
        f.write(header_content)
    
    print(f"Model header saved as {header_filename}")
    print(f"Copy this file to your ESP32-S3 project's include/ directory")

# Generate header file
generate_model_header(litert_model)
```

## Model Performance Targets

**For ESP32-S3 Deployment:**
- Model size: <100KB
- Inference time: <50ms
- Accuracy: >90%
- Precision: >85% (minimize false positives)
- Recall: >90% (catch most near-miss events)

**Memory Requirements:**
- Tensor arena: ~50KB
- Model flash storage: ~100KB
- Total RAM usage: <200KB

## Next Steps

1. **Collect Data**: Run your ESP32-S3 system to collect training data
2. **Train Model**: Use this pipeline to train your TensorFlow model
3. **Deploy**: Copy the generated header file to your ESP32-S3 project
4. **Validate**: Test the deployed model with real sensor data
5. **Iterate**: Retrain with more data to improve performance

This complete pipeline takes you from raw sensor CSV data to a deployed TensorFlow Lite model running on your ESP32-S3 hardware!