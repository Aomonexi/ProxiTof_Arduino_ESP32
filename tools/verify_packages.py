"""
Verify ML package compatibility for ProxiTof training
"""
import sys

def check_packages():
    print("="*60)
    print("PROXITOF ML PACKAGE VERIFICATION")
    print("="*60)
    print()
    
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
        warnings.append("Python 3.12+ may have compatibility issues, recommend 3.11")
    else:
        print("  ✅ Python version OK")
    print()
    
    # Check TensorFlow
    try:
        import tensorflow as tf
        tf_version = tf.__version__
        print(f"TensorFlow: {tf_version}")
        
        major = int(tf_version.split('.')[0])
        if major >= 3:
            errors.append(f"TensorFlow {tf_version} NOT compatible! ESP-NN requires 2.x")
            print(f"  ❌ TensorFlow 3.x not compatible with ESP32!")
            print(f"     Run: pip uninstall tensorflow")
            print(f"     Then: pip install 'tensorflow>=2.15,<3.0'")
        elif major == 2:
            minor = int(tf_version.split('.')[1])
            if minor < 15:
                warnings.append(f"TensorFlow {tf_version} is old, recommend 2.15+")
                print(f"  ⚠️  TensorFlow {tf_version} is old, recommend upgrading to 2.15+")
            else:
                print("  ✅ TensorFlow version OK (2.15+ required for ESP-NN)")
        else:
            errors.append(f"TensorFlow {tf_version} too old! Need 2.15+")
    except ImportError:
        errors.append("TensorFlow not installed")
        print("TensorFlow: NOT INSTALLED")
        print("  ❌ Install with: pip install 'tensorflow>=2.15,<3.0'")
    print()
    
    # Check NumPy
    try:
        import numpy as np
        print(f"NumPy: {np.__version__}")
        print("  ✅ NumPy installed")
    except ImportError:
        errors.append("NumPy not installed")
        print("NumPy: NOT INSTALLED")
        print("  ❌ Install with: pip install numpy")
    print()
    
    # Check Pandas
    try:
        import pandas as pd
        print(f"Pandas: {pd.__version__}")
        print("  ✅ Pandas installed")
    except ImportError:
        errors.append("Pandas not installed")
        print("Pandas: NOT INSTALLED")
        print("  ❌ Install with: pip install pandas")
    print()
    
    # Check Scikit-learn
    try:
        import sklearn
        print(f"Scikit-learn: {sklearn.__version__}")
        print("  ✅ Scikit-learn installed")
    except ImportError:
        errors.append("Scikit-learn not installed")
        print("Scikit-learn: NOT INSTALLED")
        print("  ❌ Install with: pip install scikit-learn")
    print()
    
    # Check Matplotlib
    try:
        import matplotlib
        print(f"Matplotlib: {matplotlib.__version__}")
        print("  ✅ Matplotlib installed")
    except ImportError:
        warnings.append("Matplotlib not installed (optional, for plotting)")
        print("Matplotlib: NOT INSTALLED")
        print("  ⚠️  Optional: pip install matplotlib")
    print()
    
    # Check Flatbuffers
    try:
        import flatbuffers
        print(f"Flatbuffers: {flatbuffers.__version__}")
        print("  ✅ Flatbuffers installed")
    except ImportError:
        warnings.append("Flatbuffers not installed (needed for TFLite conversion)")
        print("Flatbuffers: NOT INSTALLED")
        print("  ⚠️  Required for TFLite: pip install flatbuffers")
    print()
    
    # Test TensorFlow functionality
    if not errors or "TensorFlow" not in str(errors):
        try:
            print("Testing TensorFlow functionality...")
            import tensorflow as tf
            
            # Test model creation
            model = tf.keras.Sequential([
                tf.keras.layers.Dense(10, activation='relu', input_shape=(5,)),
                tf.keras.layers.Dense(2, activation='softmax')
            ])
            print("  ✅ Keras model creation works")
            
            # Test TFLite conversion
            converter = tf.lite.TFLiteConverter.from_keras_model(model)
            tflite_model = converter.convert()
            print("  ✅ TFLite conversion works")
            print()
        except Exception as e:
            errors.append(f"TensorFlow functionality test failed: {e}")
            print(f"  ❌ TensorFlow test failed: {e}")
            print()
    
    # Summary
    print("="*60)
    print("SUMMARY")
    print("="*60)
    
    if errors:
        print("\n❌ ERRORS FOUND:")
        for i, error in enumerate(errors, 1):
            print(f"  {i}. {error}")
        print("\n⚠️  Fix errors before proceeding with ML training!")
        print("\nQuick fix:")
        print("  pip install -r requirements_ml.txt")
        return False
    elif warnings:
        print("\n⚠️  WARNINGS:")
        for i, warning in enumerate(warnings, 1):
            print(f"  {i}. {warning}")
        print("\n✅ You can proceed, but consider fixing warnings.")
        print("\nTo fix warnings:")
        print("  pip install -r requirements_ml.txt")
        return True
    else:
        print("\n✅ ALL PACKAGES OK!")
        print("\n🎉 You're ready to start ML training!")
        print("\nNext steps:")
        print("  1. Collect data (press ACTION button during near-misses)")
        print("  2. Copy CSV files from SD card to data/raw/")
        print("  3. Run: python tools/quick_start_ml.py")
        return True

if __name__ == '__main__':
    print()
    success = check_packages()
    print()
    sys.exit(0 if success else 1)
