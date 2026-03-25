# ProxiTof Tools Reference

**Last Updated**: January 31, 2026

---

## ML Training Tools

### quick_start_ml.py
**All-in-one ML training pipeline**

```bash
cd tools
python quick_start_ml.py
```

Runs complete workflow:
1. Data quality check
2. Load CSV files
3. Prepare training data
4. Train neural network
5. Convert to TFLite
6. Generate C header with model + scaler

---

### check_data_quality.py
**Validate CSV files before training**

```bash
python check_data_quality.py ../data/raw
```

Checks for:
- Missing columns
- Empty files
- Missing values
- Sensor anomalies (all zeros, stuck values)
- Timestamp issues
- Flag statistics

---

### analyze_data.py
**Generate thesis-ready statistics and reports**

```bash
python analyze_data.py ../data/raw
python analyze_data.py ../data/raw --output ../docs/analysis_report.md
```

Generates:
- Dataset overview (samples, duration, flags)
- ToF sensor statistics
- IMU sensor statistics
- Near-miss event analysis
- Data quality assessment
- Thesis recommendations

---

### load_data.py
**Load and combine CSV files**

```bash
python load_data.py
```

- Loads all CSV files from `data/raw/`
- Combines into single DataFrame
- Handles column name variations
- Saves to `data/processed/combined_data.csv`

---

### prepare_training_data.py
**Prepare data for ML training**

```bash
python prepare_training_data.py
```

- Extracts 25 features (16 ToF + 9 IMU)
- Uses `flag_manual` as ground truth (NOT `flag_auto`)
- Balances dataset (undersamples safe class)
- Normalizes features (StandardScaler)
- Splits train/test (80/20)
- Saves scaler parameters for deployment

---

### train_model.py
**Train neural network**

```bash
python train_model.py
```

- Creates 25→16→2 neural network
- Uses class weighting for imbalanced data
- Early stopping to prevent overfitting
- Saves best model and training history

---

### convert_to_tflite.py
**Convert model for ESP32**

```bash
python convert_to_tflite.py
```

- Converts Keras model to TFLite
- Tests accuracy preservation
- Generates C header with:
  - Model data (byte array)
  - Scaler mean (25 values)
  - Scaler std (25 values)

---

## Utility Tools

### verify_packages.py
**Check Python package versions**

```bash
python verify_packages.py
```

Verifies TensorFlow, NumPy, Pandas, Scikit-learn compatibility.

---

### convert_logo.py
**Convert logo image to C header**

```bash
python convert_logo.py logo.png
```

Converts image to byte array for OLED display.

---

## USB Tools

### proxitof_usb_tool.py
**USB interface for device communication**

```bash
python proxitof_usb_tool.py
```

- Send commands to device
- Sync time
- Read status

---

### usb_time_sync.py
**Sync device time via USB**

```bash
python usb_time_sync.py
```

Sets device RTC to current PC time.

---

## Workflow Summary

### Data Collection Phase
```bash
# 1. Build and flash firmware
idf.py build flash

# 2. Collect data (ride bike, press button for near-misses)
# 3. Copy CSV files from SD card to data/raw/

# 4. Check data quality
cd tools
python check_data_quality.py ../data/raw

# 5. Analyze data for thesis
python analyze_data.py ../data/raw --output ../docs/analysis_report.md
```

### Training Phase
```bash
# Run complete pipeline
python quick_start_ml.py

# Or run steps individually:
python load_data.py
python prepare_training_data.py
python train_model.py
python convert_to_tflite.py
```

### Deployment Phase
```bash
# Rebuild firmware with trained model
cd ..
idf.py build flash monitor

# Model and scaler are now embedded in firmware!
```

---

## Output Files

| File | Description |
|------|-------------|
| `data/processed/combined_data.csv` | Combined CSV data |
| `data/processed/X_train.npy` | Training features |
| `data/processed/X_test.npy` | Test features |
| `data/processed/y_train.npy` | Training labels |
| `data/processed/y_test.npy` | Test labels |
| `data/processed/scaler.pkl` | Scaler object |
| `data/processed/scaler_params.npz` | Scaler mean/std |
| `models/near_miss_model.h5` | Keras model |
| `models/near_miss_model.tflite` | TFLite model |
| `models/training_history.png` | Training plot |
| `components/ml_inference/include/model_data.h` | C header |

---

## Troubleshooting

### "No CSV files found"
Copy CSV files from SD card to `data/raw/` directory.

### "Missing columns"
Check CSV format matches expected columns (see `docs/CSV_FORMAT_CORRECTION.md`).

### "Not enough manual flags"
Collect more near-miss examples by pressing ACTION button during events.

### "Model accuracy too low"
- Collect more diverse training data
- Check for sensor issues
- Adjust class balance ratio

### "TFLite conversion fails"
- Update TensorFlow: `pip install --upgrade tensorflow`
- Check model architecture compatibility
