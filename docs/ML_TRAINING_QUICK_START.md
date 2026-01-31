# ML Training Quick Start Guide

**Date**: January 31, 2026  
**Purpose**: Get started with ML training in 5 minutes

---

## Prerequisites

- ✅ Python 3.8-3.11 installed
- ✅ At least 100 near-miss examples collected
- ✅ CSV files from SD card

---

## Quick Start (5 Steps)

### Step 1: Install Python Packages

```bash
# Create virtual environment
python -m venv venv_proxitof

# Activate (Windows)
.\venv_proxitof\Scripts\Activate.ps1

# Activate (macOS/Linux)
source venv_proxitof/bin/activate

# Install packages
pip install -r requirements_ml.txt

# Verify installation
python tools/verify_packages.py
```

**Expected Output:**
```
✅ ALL PACKAGES OK!
🎉 You're ready to start ML training!
```

### Step 2: Copy Data from SD Card

```bash
# Create data directory
mkdir -p data/raw

# Activate USB mode on device:
#   1. Navigate to Mode 7 (USB Transfer)
#   2. Press ACTION button
#   3. Connect USB cable to LEFT port

# Copy CSV files
# Windows: copy E:\*.csv data\raw\
# macOS:   cp /Volumes/PROXITOF/*.csv data/raw/
# Linux:   cp /media/user/PROXITOF/*.csv data/raw/
```

### Step 3: Run Training Pipeline

```bash
cd tools
python quick_start_ml.py
```

This will:
- ✅ Load and inspect data
- ✅ Prepare training dataset
- ✅ Train neural network
- ✅ Convert to TFLite
- ✅ Generate C header file

**Expected time**: 5-10 minutes

### Step 4: Copy Model to ESP32 Project

```bash
# Model is automatically saved to:
cp components/ml_inference/model_data.h <your_location>/components/ml_inference/include/
```

### Step 5: Flash and Test

```bash
# Build and flash
idf.py build flash monitor

# Watch for ML inference messages:
# I (1234) ML_INFERENCE: 🎉 ML inference ready!
# I (5678) MAIN: ⚠️  NEAR-MISS DETECTED BY ML!
# I (5678) MAIN:    Confidence: 85.3%
```

---

## What You Need

### Software (PC)

| Package | Version | Purpose |
|---------|---------|---------|
| Python | 3.8-3.11 | ML training |
| TensorFlow | 2.15+ (2.x only) | Neural network |
| NumPy | 1.24+ | Data processing |
| Pandas | 2.0+ | CSV handling |
| Scikit-learn | 1.3+ | Data preparation |

**Why TensorFlow 2.x?**
- ✅ ESP-NN is designed for TensorFlow Lite 2.x
- ✅ TFLite Micro runs on ESP32
- ⚠️ TensorFlow 3.x is NOT yet compatible with ESP32
- ❌ PyTorch is NOT compatible

### Data (From Device)

| Item | Minimum | Recommended |
|------|---------|-------------|
| Near-miss examples | 100 | 200+ |
| Safe examples | 1000 | 2000+ |
| Collection time | 2 hours | 5+ hours |
| Diversity | 3 scenarios | 10+ scenarios |

---

## Training Scripts

All scripts are in `tools/` directory:

| Script | Purpose |
|--------|---------|
| `load_data.py` | Load and inspect CSV files |
| `prepare_training_data.py` | Balance and normalize dataset |
| `train_model.py` | Train neural network |
| `convert_to_tflite.py` | Convert to ESP32 format |
| `quick_start_ml.py` | **Run complete pipeline** |

---

## Model Architecture

```
Input (25 features)
    ↓
Dense (16 neurons, ReLU) ← ESP-NN accelerated
    ↓
Dropout (0.2)
    ↓
Dense (2 classes, Softmax)
    ↓
Output (near-miss probability)
```

**Features:**
- 16 ToF distances (4x4 grid)
- 3 Orientation (roll, pitch, yaw)
- 3 Acceleration (x, y, z)
- 3 Gyroscope (x, y, z)

**Total**: 25 features → 2 classes (safe, near-miss)

---

## Performance Targets

| Metric | Target | Good | Excellent |
|--------|--------|------|-----------|
| Accuracy | >90% | >93% | >95% |
| Detection Rate | >75% | >80% | >85% |
| False Alarm Rate | <5% | <3% | <1% |
| Inference Time | <20ms | <15ms | <10ms |

---

## Troubleshooting

### "No CSV files found"
→ Copy CSV files from SD card to `data/raw/`

### "Only X near-miss examples"
→ Collect more data (aim for 100-200 examples)

### "Model accuracy is low"
→ Collect more diverse examples, try different scenarios

### "Too many false alarms"
→ Increase detection threshold (0.70 → 0.80)

### "Missing near-miss events"
→ Decrease detection threshold (0.70 → 0.60)

---

## Next Steps

1. ✅ **Collect data** - Ride bike, press button during near-misses
2. ✅ **Run pipeline** - `python quick_start_ml.py`
3. ✅ **Deploy model** - Copy to ESP32, flash firmware
4. ✅ **Test in field** - Verify detection works
5. ✅ **Tune threshold** - Adjust based on performance
6. ✅ **Iterate** - Collect more data, retrain, improve

---

## Documentation

- **Complete Guide**: `docs/ML_TRAINING_COMPLETE_GUIDE.md`
- **ESP-NN Workflow**: `docs/ESPNN_TRAINING_WORKFLOW.md`
- **Training Data Sources**: `docs/TRAINING_DATA_SOURCES.md`
- **Hybrid Detection**: `docs/HYBRID_DETECTION_STRATEGY.md`

---

## Support

**Issues?**
1. Check `docs/ML_TRAINING_COMPLETE_GUIDE.md` for detailed instructions
2. Review error messages in console output
3. Verify data quality with `load_data.py`
4. Test model accuracy with `train_model.py`

**Questions?**
- ESP-NN documentation: https://github.com/espressif/esp-nn
- TFLite Micro: https://www.tensorflow.org/lite/microcontrollers
- ESP-IDF: https://docs.espressif.com/projects/esp-idf/

---

**Status**: ✅ Ready to start ML training!  
**Next**: Run `python tools/quick_start_ml.py`

