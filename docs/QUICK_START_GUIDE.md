# Quick Start Guide - ML Training

**Date**: January 31, 2026  
**For**: First-time ML training setup

---

## Step 1: Verify Installation ✅

You just installed the requirements. Now verify everything works:

```bash
python tools/verify_packages.py
```

**Expected output:**
```
Python: 3.12.x
  ✅ Python version OK

TensorFlow: 2.16.x
  ✅ TensorFlow version OK

NumPy: 1.26.x
  ✅ NumPy installed

Pandas: 2.2.x
  ✅ Pandas installed

Scikit-learn: 1.4.x
  ✅ Scikit-learn installed

Flatbuffers: 24.x.x
  ✅ Flatbuffers installed

==============================================================
✅ ALL PACKAGES OK!

You're ready to start ML training!
```

**If you see errors**: Check `docs/ML_PACKAGE_COMPATIBILITY.md` for troubleshooting

---

## Step 2: Collect Training Data 🚴

**You need**: 100-200 near-miss examples minimum

**How to collect**:
1. Ride your bike with the ProxiTof device
2. Press the ACTION button when you experience a near-miss:
   - Close vehicle pass
   - Emergency braking
   - Sudden swerve
   - Door zone conflict
   - Any situation where you feel danger
3. Hold the button during the entire event
4. LED turns BLUE while held
5. System logs data with flag_manual=1

**Tips**:
- Collect diverse examples (different times, weather, traffic)
- Also collect normal riding (system does this automatically)
- Aim for 2-3 hours of riding per week
- 10-20 near-miss events per session

---

## Step 3: Extract CSV Files from Device 📁

### Method 1: USB Transfer Mode (Recommended)

**On Device**:
1. Navigate to **Mode 7: USB Transfer**
2. Press **ACTION button** to activate USB mode
3. Display shows: "USB MODE ACTIVE"
4. Connect USB cable to **LEFT port** (USB-OTG)
5. Device appears as flash drive on your computer

**On Computer**:

**Windows:**
```bash
# Create folder for raw data
mkdir data\raw

# Copy CSV files (E: is your device drive letter)
copy E:\*.csv data\raw\
```

**macOS:**
```bash
# Create folder for raw data
mkdir -p data/raw

# Copy CSV files
cp /Volumes/PROXITOF/*.csv data/raw/
```

**Linux:**
```bash
# Create folder for raw data
mkdir -p data/raw

# Copy CSV files (adjust path to your mount point)
cp /media/user/PROXITOF/*.csv data/raw/
```

**After copying**:
1. Eject the drive properly on your computer
2. Disconnect USB cable
3. Press ACTION button on device to exit USB mode

### Method 2: Remove SD Card (Alternative)

1. Power off device
2. Remove SD card
3. Insert into computer's SD card reader
4. Copy `*.csv` files to `data/raw/` folder
5. Eject SD card
6. Insert back into device

---

## Step 4: Check Your Data 📊

**Verify you have CSV files**:

```bash
# Windows
dir data\raw\*.csv

# macOS/Linux
ls data/raw/*.csv
```

**Expected output**:
```
log_20260131_120000.csv
log_20260131_130000.csv
log_20260131_140000.csv
...
```

**File structure**:
```
your_project/
├── data/
│   └── raw/              ← Put your CSV files here!
│       ├── log_20260131_120000.csv
│       ├── log_20260131_130000.csv
│       └── ...
├── tools/
│   ├── load_data.py
│   ├── quick_start_ml.py
│   └── ...
└── requirements_ml.txt
```

---

## Step 5: Run Training Pipeline 🚀

**One command does everything**:

```bash
cd tools
python quick_start_ml.py
```

**What it does automatically**:
1. ✅ Loads all CSV files from `data/raw/`
2. ✅ Inspects data quality
3. ✅ Extracts 25 features
4. ✅ Balances dataset
5. ✅ Normalizes features
6. ✅ Trains neural network
7. ✅ Evaluates performance
8. ✅ Converts to TFLite
9. ✅ Generates C header for ESP32

**Time**: 5-10 minutes

**What you'll see**:
```
==============================================================
PROXITOF ML TRAINING PIPELINE
==============================================================

Checking requirements...
✅ TensorFlow 2.16.1
✅ NumPy 1.26.4
✅ Pandas 2.2.0
✅ Scikit-learn 1.4.0

Checking data files...
✅ Found 5 CSV files

==============================================================
STEP 1: LOADING DATA
==============================================================

Found 5 CSV files
Loading log_20260131_120000.csv...
Loading log_20260131_130000.csv...
...

✅ Loaded 15234 total samples

DATA INSPECTION
----------------------------------------------------------
Total samples: 15234
Near-miss (flag_manual=1): 156 (1.02%)
Safe (flag_manual=0):      15078 (98.98%)

✅ Step 1 complete

==============================================================
STEP 2: PREPARING TRAINING DATA
==============================================================

FEATURE EXTRACTION
----------------------------------------------------------
Feature shape: (15234, 25)
Label shape: (15234,)
Feature columns: 25
  - ToF: 16 (4x4 grid)
  - IMU: 9

Label source: flag_manual ONLY
  ⚠️  flag_auto is EXCLUDED (it's the model output!)

DATASET BALANCING
----------------------------------------------------------
Original dataset:
  Near-miss: 156 (1.02%)
  Safe:      15078 (98.98%)
  Ratio:     96.7:1

Balanced dataset (target ratio 10:1):
  Near-miss: 156 (9.09%)
  Safe:      1560 (90.91%)
  Ratio:     10.0:1
  Total:     1716 samples

✅ Step 2 complete

==============================================================
STEP 3: TRAINING MODEL
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

Training for up to 100 epochs...
Epoch 1/100
43/43 [==============================] - 1s - loss: 0.6234 - accuracy: 0.6789
...
Epoch 45/100
43/43 [==============================] - 0s - loss: 0.1234 - accuracy: 0.9567

MODEL EVALUATION
----------------------------------------------------------
Accuracy:          95.93%
Detection Rate:    76.47%
False Alarm Rate:  1.94%
Precision:         81.25%
F1 Score:          0.7879

✅ Step 3 complete

==============================================================
STEP 4: CONVERTING TO TFLITE
==============================================================

✅ TFLite model saved
Model size: 2,456 bytes (2.40 KB)

✅ TFLite model accuracy: 95.85%
✅ C header file saved

✅ Step 4 complete

==============================================================
🎉 ML TRAINING PIPELINE COMPLETE!
==============================================================

Generated files:
  ✅ models/near_miss_model.h5 - Trained Keras model
  ✅ models/near_miss_model.tflite - TFLite model
  ✅ models/training_history.png - Training plot
  ✅ components/ml_inference/model_data.h - C header for ESP32

Next steps:
  1. Copy model_data.h to your ESP32 project
  2. Implement ML inference component
  3. Flash firmware: idf.py flash monitor
  4. Test in field and tune threshold
```

---

## Step 6: Check Generated Files 📂

**After training completes, you'll have**:

```
your_project/
├── data/
│   ├── raw/                          ← Your CSV files
│   │   └── *.csv
│   └── processed/                    ← Generated by training
│       ├── combined_data.csv         ← All CSV files merged
│       ├── X_train.npy               ← Training features
│       ├── X_test.npy                ← Test features
│       ├── y_train.npy               ← Training labels
│       ├── y_test.npy                ← Test labels
│       ├── scaler.pkl                ← Feature scaler (important!)
│       └── feature_names.txt         ← Feature list
│
├── models/                           ← Generated models
│   ├── near_miss_model.h5            ← Keras model (10KB)
│   ├── near_miss_model.tflite        ← TFLite model (2.4KB)
│   ├── best_model.h5                 ← Best checkpoint
│   └── training_history.png          ← Training plot
│
└── components/ml_inference/          ← For ESP32
    └── model_data.h                  ← C header file ⭐
```

**Most important file**: `components/ml_inference/model_data.h`  
This is what you'll use on ESP32!

---

## Troubleshooting

### Issue: "No CSV files found"

**Problem**: Script can't find your data

**Solution**:
```bash
# Check if folder exists
ls data/raw/

# If not, create it
mkdir -p data/raw

# Copy CSV files
cp /path/to/your/csvfiles/*.csv data/raw/

# Verify
ls data/raw/*.csv
```

### Issue: "Not enough near-miss examples"

**Output**:
```
⚠️  WARNING: Only 45 near-miss examples!
   Recommendation: Collect at least 100-200 near-miss examples
```

**Solution**: Collect more data! Ride more and press button during near-misses.

**Minimum requirements**:
- 100-200 near-miss examples (flag_manual=1)
- 1000-2000 safe examples (flag_manual=0)

### Issue: "Column not found"

**Problem**: CSV format doesn't match expected format

**Solution**: This shouldn't happen (we fixed the bug), but if it does:
1. Check one CSV file to see column names
2. Compare with expected format in `docs/CSV_FORMAT_CORRECTION.md`
3. Report the issue

### Issue: "Poor model performance"

**Output**:
```
⚠️  WARNING: Detection rate is low (<80%)
```

**Solutions**:
1. Collect more diverse training data
2. Collect more near-miss examples
3. Review data quality (check for corrupted files)
4. Try training again with different random seed

---

## What to Do Next

### Option 1: Deploy to ESP32 (Recommended)

See: `docs/ESPNN_IMPLEMENTATION_GUIDE.md`

**Quick steps**:
1. Copy `components/ml_inference/model_data.h` to your ESP32 project
2. Implement ML inference component (code provided in guide)
3. Build: `idf.py build`
4. Flash: `idf.py flash monitor`
5. Test in field!

### Option 2: Collect More Data and Retrain

If performance isn't good enough:
1. Collect more near-miss examples
2. Copy new CSV files to `data/raw/`
3. Run `python tools/quick_start_ml.py` again
4. Compare performance

### Option 3: Analyze Results

**View training plot**:
- Open `models/training_history.png`
- Check if model converged
- Look for overfitting (gap between training and validation)

**Review metrics**:
- Accuracy: Should be >90%
- Detection Rate: Should be >75%
- False Alarm Rate: Should be <5%
- F1 Score: Should be >0.75

---

## Summary

### The Simple Workflow

1. **Install packages** ✅ (You did this!)
   ```bash
   pip install -r requirements_ml.txt
   ```

2. **Verify installation** ✅ (Do this now!)
   ```bash
   python tools/verify_packages.py
   ```

3. **Collect data** 🚴 (Ride bike, press button)
   - Need: 100-200 near-miss examples

4. **Copy CSV files** 📁 (USB mode or SD card)
   ```bash
   # Put files in data/raw/ folder
   cp /path/to/*.csv data/raw/
   ```

5. **Run training** 🚀 (One command!)
   ```bash
   cd tools
   python quick_start_ml.py
   ```

6. **Deploy to ESP32** 🎯 (See ESPNN guide)
   - Copy `model_data.h` to project
   - Implement inference
   - Flash and test

---

## Quick Reference

**Folder structure**:
```
data/raw/          ← Put your CSV files here
data/processed/    ← Generated automatically
models/            ← Generated models
components/ml_inference/model_data.h  ← Use this on ESP32
```

**Commands**:
```bash
# Verify installation
python tools/verify_packages.py

# Train model (one command does everything!)
cd tools
python tools/quick_start_ml.py

# Or run steps individually:
python tools/load_data.py              # Step 1: Load data
python tools/prepare_training_data.py  # Step 2: Prepare
python tools/train_model.py            # Step 3: Train
python tools/convert_to_tflite.py      # Step 4: Convert
```

**Need help?**
- Installation issues: `docs/ML_PACKAGE_COMPATIBILITY.md`
- Training details: `docs/QUICK_START_ML_EXPLAINED.md`
- ESP32 deployment: `docs/ESPNN_IMPLEMENTATION_GUIDE.md`
- Complete guide: `docs/ML_TRAINING_COMPLETE_GUIDE.md`
- Thesis documentation: `docs/note-11-ml-training-methodology-and-validation.md`

---

**Status**: Ready to train!  
**Next step**: Verify installation, then collect data  
**Questions?**: Check the docs folder for detailed guides
