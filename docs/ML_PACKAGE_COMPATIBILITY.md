# ML Package Compatibility Guide

**Date**: January 31, 2026  
**Purpose**: Ensure correct package versions for ESP-NN compatibility

---

## Critical Compatibility Requirements

### TensorFlow Version

**REQUIRED**: TensorFlow 2.x (2.15 or later)  
**NOT COMPATIBLE**: TensorFlow 3.x

```bash
# ✅ CORRECT - Install latest TensorFlow 2.x
pip install "tensorflow>=2.15,<3.0"

# ❌ WRONG - TensorFlow 3.x not compatible with ESP32
pip install tensorflow  # May install 3.x if available
```

**Why TensorFlow 2.x?**
- ESP-NN is designed for TensorFlow Lite 2.x models
- TFLite Micro on ESP32 uses TensorFlow 2.x schema
- TensorFlow 3.x has breaking changes not yet supported by ESP-IDF

**Checking Your Version:**
```python
import tensorflow as tf
print(tf.__version__)  # Should show 2.15.x or 2.16.x, NOT 3.x
```

---

## Recommended Package Versions

### Core ML Libraries

```txt
tensorflow>=2.15,<3.0  # Latest 2.x, but not 3.x
numpy>=1.24            # Latest compatible with TF 2.x
pandas>=2.0            # Latest stable
scikit-learn>=1.3      # Latest stable
```

### Visualization

```txt
matplotlib>=3.8        # Latest stable
seaborn>=0.13          # Latest stable
```

### Utilities

```txt
flatbuffers>=23.5      # Required for TFLite conversion
tqdm>=4.66             # Progress bars
jupyter>=1.0           # Notebooks (optional)
scipy>=1.11            # Advanced analysis (optional)
```

---

## Installation Methods

### Method 1: Using requirements_ml.txt (Recommended)

```bash
pip install -r requirements_ml.txt
```

This ensures all packages are compatible with each other.

### Method 2: Manual Installation

```bash
# Install TensorFlow 2.x first
pip install "tensorflow>=2.15,<3.0"

# Then install other packages
pip install numpy pandas scikit-learn matplotlib seaborn flatbuffers tqdm
```

### Method 3: With Specific Versions (If Issues Occur)

```bash
# Known working combination
pip install tensorflow==2.15.0 numpy==1.24.3 pandas==2.1.4 scikit-learn==1.3.2
```

---

## Python Version Compatibility

| Python Version | TensorFlow 2.15+ | Recommended | Notes |
|----------------|------------------|-------------|-------|
| 3.8 | ✅ Yes | ⚠️ EOL soon | Works but outdated |
| 3.9 | ✅ Yes | ✅ Good | Stable |
| 3.10 | ✅ Yes | ✅ Good | Stable |
| 3.11 | ✅ Yes | ✅ Best | Recommended |
| 3.12 | ✅ Yes | ⚠️ Newer | Works with TF 2.16+ |

**Recommendation**: Python 3.11 is most tested, but 3.12 works with TensorFlow 2.16+

**For Python 3.12 users:**
- TensorFlow 2.16+ has Python 3.12 support
- Some packages may show warnings (usually safe to ignore)
- If issues occur, consider using Python 3.11

---

## Common Issues and Solutions

### Issue 1: TensorFlow 3.x Installed

**Symptoms:**
```
ImportError: cannot import name 'MicroMutableOpResolver'
```

**Solution:**
```bash
# Uninstall TensorFlow 3.x
pip uninstall tensorflow

# Install TensorFlow 2.x
pip install "tensorflow>=2.15,<3.0"
```

### Issue 2: NumPy Version Conflict

**Symptoms:**
```
ValueError: numpy.dtype size changed
```

**Solution:**
```bash
# Reinstall NumPy compatible with TensorFlow
pip install --upgrade "numpy>=1.24,<2.0"
```

### Issue 3: Flatbuffers Missing

**Symptoms:**
```
ModuleNotFoundError: No module named 'flatbuffers'
```

**Solution:**
```bash
pip install flatbuffers
```

### Issue 4: M1/M2 Mac Issues

**Symptoms:**
```
ERROR: Could not find a version that satisfies the requirement tensorflow
```

**Solution:**
```bash
# Use conda for M1/M2 Macs
conda create -n proxitof python=3.11
conda activate proxitof
conda install -c apple tensorflow-deps
pip install tensorflow-macos
pip install -r requirements_ml.txt
```

---

## Verification Script

Save as `verify_packages.py`:

```python
"""
Verify ML package compatibility
"""
import sys

def check_packages():
    print("Checking ML package compatibility...\n")
    
    errors = []
    warnings = []
    
    # Check Python version
    py_version = sys.version_info
    print(f"Python: {py_version.major}.{py_version.minor}.{py_version.micro}")
    
    if py_version.major != 3:
        errors.append("Python 3.x required")
    elif py_version.minor < 8:
        errors.append("Python 3.8+ required")
    elif py_version.minor > 11:
        warnings.append("Python 3.12+ may have compatibility issues")
    else:
        print("  ✅ Python version OK\n")
    
    # Check TensorFlow
    try:
        import tensorflow as tf
        tf_version = tf.__version__
        print(f"TensorFlow: {tf_version}")
        
        major = int(tf_version.split('.')[0])
        if major >= 3:
            errors.append(f"TensorFlow {tf_version} not compatible! Need 2.x")
        elif major == 2:
            minor = int(tf_version.split('.')[1])
            if minor < 15:
                warnings.append(f"TensorFlow {tf_version} is old, recommend 2.15+")
            else:
                print("  ✅ TensorFlow version OK\n")
        else:
            errors.append(f"TensorFlow {tf_version} too old! Need 2.15+")
    except ImportError:
        errors.append("TensorFlow not installed")
    
    # Check NumPy
    try:
        import numpy as np
        print(f"NumPy: {np.__version__}")
        print("  ✅ NumPy installed\n")
    except ImportError:
        errors.append("NumPy not installed")
    
    # Check Pandas
    try:
        import pandas as pd
        print(f"Pandas: {pd.__version__}")
        print("  ✅ Pandas installed\n")
    except ImportError:
        errors.append("Pandas not installed")
    
    # Check Scikit-learn
    try:
        import sklearn
        print(f"Scikit-learn: {sklearn.__version__}")
        print("  ✅ Scikit-learn installed\n")
    except ImportError:
        errors.append("Scikit-learn not installed")
    
    # Check Flatbuffers
    try:
        import flatbuffers
        print(f"Flatbuffers: {flatbuffers.__version__}")
        print("  ✅ Flatbuffers installed\n")
    except ImportError:
        warnings.append("Flatbuffers not installed (needed for TFLite conversion)")
    
    # Summary
    print("="*60)
    if errors:
        print("❌ ERRORS:")
        for error in errors:
            print(f"  - {error}")
        print("\nFix errors before proceeding!")
        return False
    elif warnings:
        print("⚠️  WARNINGS:")
        for warning in warnings:
            print(f"  - {warning}")
        print("\nYou can proceed, but consider fixing warnings.")
        return True
    else:
        print("✅ ALL PACKAGES OK!")
        print("\nYou're ready to start ML training!")
        return True

if __name__ == '__main__':
    success = check_packages()
    sys.exit(0 if success else 1)
```

Run verification:
```bash
python verify_packages.py
```

---

## Updating Packages

### Safe Update (Recommended)

```bash
# Update within compatible ranges
pip install --upgrade -r requirements_ml.txt
```

### Full Update (Risky)

```bash
# Update all packages to latest
pip install --upgrade tensorflow numpy pandas scikit-learn matplotlib seaborn

# Then verify compatibility
python verify_packages.py
```

---

## Virtual Environment Best Practices

### Create Isolated Environment

```bash
# Create virtual environment
python -m venv venv_proxitof

# Activate
# Windows:
venv_proxitof\Scripts\activate
# macOS/Linux:
source venv_proxitof/bin/activate

# Install packages
pip install -r requirements_ml.txt

# Verify
python verify_packages.py
```

### Why Use Virtual Environment?

- ✅ Isolates project dependencies
- ✅ Prevents version conflicts
- ✅ Easy to recreate environment
- ✅ Safe to experiment

---

## Summary

**Key Points:**
- ✅ Use TensorFlow 2.x (NOT 3.x)
- ✅ Python 3.11 recommended
- ✅ Use `requirements_ml.txt` for installation
- ✅ Verify with `verify_packages.py`
- ✅ Use virtual environment

**Quick Install:**
```bash
python -m venv venv_proxitof
source venv_proxitof/bin/activate  # or venv_proxitof\Scripts\activate on Windows
pip install -r requirements_ml.txt
python verify_packages.py
```

---

**Document Version:** 1.0  
**Last Updated:** January 31, 2026  
**Status:** ✅ Complete compatibility guide
