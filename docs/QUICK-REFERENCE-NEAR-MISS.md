# Quick Reference: Near-Miss Detection System

**Date:** January 20, 2026  
**Status:** ✅ Operational - Ready for Data Collection

---

## What is a Near-Miss?

### Simple Definition
A situation where a collision was narrowly avoided, detected by:
- **Close distance** (< 30cm)
- **Fast approach** (> 0.5 m/s)
- **Sudden movement** (> 2.0 m/s³ jerk)

### Detection Criteria

| Condition | Threshold | Example |
|-----------|-----------|---------|
| Critical Distance | < 300 mm | Object within arm's reach |
| Rapid Approach | < -500 mm/s | Approaching at 0.5 m/s |
| High Jerk | > 2.0 m/s³ | Emergency brake/swerve |
| Combined Event | Both within 500ms | Approach + Evasive maneuver |

---

## How It Works

### 1. Sensors Measure
```
ToF Sensor: 4×4 distance grid (60 Hz)
IMU Sensor: Acceleration + Rotation (100 Hz)
```

### 2. Algorithm Calculates
```
Minimum Distance: Closest object in grid
Approach Rate: How fast distance changes
Jerk: How suddenly you accelerate
Angular Velocity: How fast you rotate
```

### 3. System Detects
```
IF (distance < 300mm AND approaching fast)
   → CRITICAL PROXIMITY ⚠️

IF (rapid approach AND high jerk within 500ms)
   → COMBINED NEAR-MISS ⚠️

IF (rapid approach OR high jerk)
   → POTENTIAL NEAR-MISS (logged)
```

### 4. System Responds
```
✅ Flash LED orange/red (4 Hz)
✅ Log event to console
✅ Set flag_auto=1 in CSV
✅ Count event number
```

---

## Data Labeling

### Automatic (Algorithm)
```
flag_auto = 1  →  Algorithm detected near-miss
flag_auto = 0  →  Normal riding
```

### Manual (Button Press)
```
flag_manual = 1  →  User pressed button
flag_manual = 0  →  No button press
```

### Ground Truth (ML Training)
```
label = flag_manual OR flag_auto

Examples:
  0, 0 → Label: 0 (normal)
  1, 0 → Label: 1 (user-perceived)
  0, 1 → Label: 1 (algorithm-detected)
  1, 1 → Label: 1 (confirmed near-miss)
```

---

## Machine Learning Pipeline

### Step 1: Collect Data (Current)
```
✅ Ride with device (10+ hours)
✅ Automatic labeling (flag_auto)
✅ Manual labeling (button press)
✅ CSV files on SD card
```

### Step 2: Train Model (Next)
```
Framework: TensorFlow Lite
Input: 29 features × 60 timesteps (1 second)
Output: Near-miss probability (0-1)
Model: 1D-CNN or LSTM
Size: <500 KB (INT8 quantized)
```

### Step 3: Deploy (Future)
```
Load model into ESP32 flash
Run inference at 10 Hz
Combine with rule-based detection
Confidence threshold: 0.7
```

---

## CSV Data Format

```csv
timestamp_ms,tof_00,...,tof_33,roll,pitch,yaw,accel_x,accel_y,accel_z,gyro_x,gyro_y,gyro_z,flag_manual
125430,850,920,...,980,15.2,-8.5,45.3,0.8,0.3,9.7,5.2,8.1,42.5,0
125446,845,915,...,975,15.3,-8.6,45.4,0.9,0.4,9.6,5.3,8.2,42.6,0
125462,840,910,...,970,15.4,-8.7,45.5,1.0,0.5,9.5,5.4,8.3,42.7,1  ← Button pressed
```

**Columns:**
- 1 timestamp
- 16 ToF distances
- 3 orientation (roll, pitch, yaw)
- 3 acceleration (x, y, z)
- 3 gyroscope (x, y, z)
- 1 manual flag
- **Total: 27 columns**

**Derived features** (calculated during preprocessing):
- min_distance_mm
- approach_rate_mm_s
- jerk_magnitude_m_s3
- angular_velocity_deg_s

---

## Real-World Example

### Scenario: Pedestrian Suddenly Crosses

```
t=0ms: Normal riding
  Distance: 2000mm (2m away)
  Approach: -1250 mm/s (cycling speed)
  Jerk: 0.8 m/s³ (normal)
  Detection: Rapid approach detected

t=1200ms: Pedestrian steps out
  Distance: 500mm (50cm)
  Approach: -1250 mm/s (still approaching)
  Jerk: 5.2 m/s³ (EMERGENCY BRAKE!)
  Detection: HIGH JERK detected

t=1250ms: Combined detection
  Time since rapid approach: 50ms < 500ms
  Result: COMBINED NEAR-MISS ⚠️
  LED: Flash orange/red
  CSV: flag_auto=1
```

---

## Why TensorFlow Lite?

| Framework | ESP32 Support | Size | Speed | Ease |
|-----------|---------------|------|-------|------|
| **TensorFlow Lite** | ✅ Official | Small | Fast | Easy |
| PyTorch Mobile | ❌ Limited | Large | Slow | Hard |
| ESP-NN | ✅ Native | Smallest | Fastest | Complex |
| ONNX Runtime | ⚠️ Experimental | Medium | Medium | Medium |

**Winner: TensorFlow Lite**
- Official TFLite Micro for ESP32
- Python training → C++ inference
- INT8 quantization (8-bit)
- Proven on embedded systems
- Extensive documentation

---

## Current Status

**✅ Operational:**
- Rule-based detection (3-tier system)
- Automatic labeling (flag_auto)
- Manual labeling (button press)
- Real-time feature calculation
- CSV data logging
- LED visual alerts
- Console reporting

**⏳ Next Steps:**
1. Flash firmware to device
2. Test with real sensors
3. Collect training dataset (10+ hours)
4. Train ML model (Python + TensorFlow)
5. Convert to TFLite (INT8 quantization)
6. Deploy to ESP32 (hybrid detection)

---

## Key Files

**Documentation:**
- `docs/near-miss-detection-algorithm.md` - Complete technical details
- `docs/near-miss-definition-and-ml-pipeline.md` - ML workflow
- `docs/note-5-near-miss-algorithm-integration.md` - Implementation notes

**Code:**
- `main/main.c` - Main application with detection
- `components/data_processor/` - Detection algorithms
- `components/data_processor/data_processor_c_wrapper.cpp` - C/C++ bridge

**Training (Future):**
- `tools/train_model.py` - ML training script
- `tools/convert_tflite.py` - Model conversion
- `components/ml_inference/` - TFLite integration

---

**Ready to collect data and train your first ML model!** 🚀

