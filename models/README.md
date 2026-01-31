# Trained Models

**Purpose**: Store trained ML models and training artifacts

---

## Files Generated Here

### Model Files

1. **`near_miss_model.h5`** - TensorFlow Keras model (full precision)
2. **`near_miss_model.tflite`** - TensorFlow Lite model (optimized for ESP32)
3. **`model_data.h`** - C header file (embedded in firmware)

### Training Artifacts

4. **`training_history.png`** - Loss and accuracy curves
5. **`model_architecture.txt`** - Neural network structure
6. **`performance_report.txt`** - Detailed metrics

---

## Model Deployment

**Important**: The model is **embedded in firmware**, not loaded from SD card!

### Deployment Process

1. **Train Model**
   ```bash
   python tools/quick_start_ml.py
   ```
   - Generates `model_data.h`

2. **Copy to Firmware**
   - File automatically saved to: `components/ml_inference/include/model_data.h`
   - Model embedded as C array

3. **Rebuild Firmware**
   ```bash
   idf.py build flash monitor
   ```
   - Model compiled into firmware
   - ML inference active on boot

---

## Model Files Explained

### `near_miss_model.h5`
- Full TensorFlow model
- Used for retraining and analysis
- Not deployed to ESP32

### `near_miss_model.tflite`
- Optimized for embedded devices
- Quantized for smaller size
- Used as reference

### `model_data.h`
- C header file with model as byte array
- Embedded in firmware
- **This is what gets deployed to ESP32**

---

## Model Performance

Check `performance_report.txt` for:
- Accuracy
- Precision
- Recall
- F1 Score
- Confusion Matrix

Target: >90% accuracy on test set
