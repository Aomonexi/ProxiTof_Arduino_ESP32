# ProxiTof ML Training Workflow - Complete Plan

## Current Status Check ✅

**What We Have Ready:**
- ✅ **Hardware**: ESP32-S3 + ToF + IMU sensors working
- ✅ **Data Collection**: System logs CSV files to SD card
- ✅ **Python Environment**: Requirements and setup guide ready
- ✅ **Training Pipeline**: Complete TensorFlow guide available
- ✅ **ESP32 Implementation**: TFLite deployment guide ready

**What We Need to Do:**
- 🔄 **Collect Training Data**: Run ESP32 system to gather labeled data
- 🔄 **Train Model**: Use Python pipeline to create ML model
- 🔄 **Deploy Model**: Convert to TFLite and flash to ESP32-S3

## Phase 1: Data Collection Setup (Week 1)

### 1.1 Complete ESP32-S3 Implementation
**Current Status**: You have sensor manager and data processor working

**Next Steps:**
```bash
# Complete the remaining ESP-IDF components
1. Implement Storage Manager (SD card logging)
2. Implement Status LED component  
3. Integrate main application with FreeRTOS tasks
4. Test end-to-end data logging
```

**Expected Output**: ESP32-S3 system that logs CSV files like:
```csv
timestamp_ms,tof_00,tof_01,...,tof_33,roll,pitch,yaw,accel_x,accel_y,accel_z,gyro_x,gyro_y,gyro_z,flag_manual,flag_auto,confidence
1234567,500,520,480,...,0.5,1.2,-0.3,0.1,0.2,9.8,0.0,0.1,0.0,0,0,0.0
```

### 1.2 Set Up Python Training Environment
**Location**: Your Windows PC (separate from ESP32-S3)

```bash
# 1. Install Python 3.13 (you're doing this now)
# 2. Create virtual environment
python -m venv venv
venv\Scripts\activate

# 3. Install ML packages
pip install -r requirements_proxitof_thesis.txt

# 4. Verify TensorFlow installation
python -c "import tensorflow as tf; print(f'TensorFlow {tf.__version__} ready!')"
```

## Phase 2: Data Collection Campaign (Week 2-3)

### 2.1 Data Collection Strategy
**Target**: 10+ hours of diverse scenarios, 200+ near-miss events

**Collection Sessions:**
```
Session 1: Indoor Walking (2 hours)
- Normal walking around furniture
- Approaching walls/obstacles
- Manual flag when getting close to objects

Session 2: Outdoor Movement (2 hours)  
- Walking in open areas
- Approaching trees, poles, people
- Different lighting conditions

Session 3: Dynamic Scenarios (2 hours)
- Running/jogging
- Quick direction changes
- Moving obstacles (people walking by)

Session 4: Edge Cases (2 hours)
- Very close objects (<30cm)
- Fast approach rates
- Multiple obstacles in ToF grid

Session 5: Validation Data (2 hours)
- Mixed scenarios for final testing
- Don't use for training (hold-out set)
```

### 2.2 Data Collection Workflow
```bash
# Daily workflow:
1. Power on ESP32-S3 with fresh SD card
2. Run data collection session (30-60 minutes)
3. Press button during near-miss events
4. Copy CSV files from SD card to PC
5. Organize files by date/session
```

**File Organization on PC:**
```
F:\Master's_Thesis\ProxiTof_Data\
├── 2025-01-15\
│   ├── session1_indoor_walking\
│   │   ├── log_20250115_090000.csv
│   │   └── log_20250115_100000.csv
│   └── session2_outdoor\
├── 2025-01-16\
└── validation_data\  # Keep separate!
```

## Phase 3: Model Training (Week 4)

### 3.1 Python Training Scripts
**Location**: Create in your project root

```bash
ProxiTof_Arduino_ESP32\
├── ml_training\          # New folder
│   ├── train_model.py    # Main training script
│   ├── data_loader.py    # CSV loading utilities
│   ├── model_builder.py  # Neural network architecture
│   └── convert_to_tflite.py  # Model conversion
├── data\                 # Training data
│   ├── collected_data\   # Your CSV files
│   └── models\          # Trained models
└── docs\                # Existing guides
```

### 3.2 Training Pipeline Execution
```bash
# Activate Python environment
venv\Scripts\activate

# Step 1: Load and preprocess data
python ml_training/data_loader.py --data_dir data/collected_data

# Step 2: Train neural network
python ml_training/train_model.py --epochs 100 --batch_size 32

# Step 3: Convert to TensorFlow Lite
python ml_training/convert_to_tflite.py --model data/models/best_model.h5

# Step 4: Generate ESP32-S3 header file
python ml_training/generate_header.py --tflite data/models/proxitof_model.tflite
```

**Expected Outputs:**
- `proxitof_model.tflite` (optimized for ESP32-S3)
- `proxitof_model.h` (C header file)
- Training metrics and validation results

## Phase 4: Model Deployment (Week 5)

### 4.1 ESP32-S3 Integration
```bash
# Copy generated files to ESP-IDF project
cp data/models/proxitof_model.h components/ml_inference/include/

# Implement ML inference component (using our guide)
# Build and flash firmware with ML model
idf.py build flash monitor
```

### 4.2 Real-World Testing
```bash
# Test scenarios:
1. Validation data playback
2. Live near-miss detection
3. Performance monitoring (inference time, accuracy)
4. False positive/negative analysis
```

## Detailed Training Scripts Plan

### Script 1: `ml_training/data_loader.py`
```python
# Purpose: Load CSV files, clean data, create train/val/test splits
# Input: Directory of CSV files from ESP32-S3
# Output: Preprocessed datasets ready for training

def load_proxitof_data(data_directory):
    # Load all CSV files
    # Clean and validate data
    # Extract features and labels
    # Handle class imbalance
    return X_train, X_val, X_test, y_train, y_val, y_test
```

### Script 2: `ml_training/train_model.py`
```python
# Purpose: Train neural network for near-miss detection
# Input: Preprocessed data from data_loader.py
# Output: Trained Keras model

def create_proxitof_model():
    # 25 input features -> 64 -> 32 -> 16 -> 1 output
    # Optimized for ESP32-S3 constraints
    return model

def train_model():
    # Handle class imbalance with weights
    # Use callbacks for early stopping
    # Save best model
    return trained_model, history
```

### Script 3: `ml_training/convert_to_tflite.py`
```python
# Purpose: Convert Keras model to TensorFlow Lite
# Input: Trained Keras model
# Output: Quantized TFLite model for ESP32-S3

def convert_to_tflite(model):
    # Apply quantization for smaller size
    # Optimize for ESP-NN acceleration
    # Validate conversion accuracy
    return tflite_model
```

### Script 4: `ml_training/generate_header.py`
```python
# Purpose: Generate C header file for ESP32-S3
# Input: TFLite model file
# Output: proxitof_model.h for ESP-IDF

def generate_model_header(tflite_model):
    # Convert binary to C array
    # Generate proper header format
    # Include size and metadata
    return header_content
```

## Timeline Summary

**Week 1**: Complete ESP32-S3 data logging implementation
**Week 2-3**: Collect 10+ hours of training data
**Week 4**: Train and optimize ML model using Python
**Week 5**: Deploy model to ESP32-S3 and validate

## Success Criteria

**Data Collection:**
- ✅ 10+ hours of sensor data
- ✅ 200+ manually flagged near-miss events
- ✅ Diverse scenarios (indoor/outdoor, different speeds)

**Model Training:**
- ✅ >90% accuracy on validation set
- ✅ >85% precision (low false positives)
- ✅ >90% recall (catch near-miss events)
- ✅ Model size <100KB for ESP32-S3

**Deployment:**
- ✅ <50ms inference time on ESP32-S3
- ✅ Real-time operation at 60Hz
- ✅ Successful near-miss detection in live testing

## Next Immediate Steps

1. **Finish Python 3.13 installation** (you're doing this now)
2. **Complete ESP32-S3 Storage Manager** (implement SD card logging)
3. **Test end-to-end data collection** (ESP32 → SD card → CSV files)
4. **Start data collection campaign** (begin gathering training data)

This plan gives you a clear path from where you are now to a fully trained and deployed ML system on your ESP32-S3!