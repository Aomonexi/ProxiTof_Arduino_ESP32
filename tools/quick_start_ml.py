"""
Quick Start ML Training Pipeline
Runs the complete workflow from data loading to model conversion
"""
import os
import sys

def check_requirements():
    """Check if required packages are installed"""
    print("Checking requirements...")
    
    try:
        import tensorflow as tf
        import numpy as np
        import pandas as pd
        import sklearn
        print(f"✅ TensorFlow {tf.__version__}")
        print(f"✅ NumPy {np.__version__}")
        print(f"✅ Pandas {pd.__version__}")
        print(f"✅ Scikit-learn {sklearn.__version__}")
        return True
    except ImportError as e:
        print(f"❌ Missing package: {e}")
        print("\nPlease install requirements:")
        print("  pip install -r requirements_ml.txt")
        return False

def check_data():
    """Check if data files exist"""
    print("\nChecking data files...")
    
    raw_dir = '../data/raw'
    if not os.path.exists(raw_dir):
        print(f"❌ Data directory not found: {raw_dir}")
        print("\nPlease:")
        print("  1. Create directory: mkdir -p ../data/raw")
        print("  2. Copy CSV files from SD card to ../data/raw/")
        return False
    
    csv_files = [f for f in os.listdir(raw_dir) if f.endswith('.csv')]
    if not csv_files:
        print(f"❌ No CSV files found in {raw_dir}")
        print("\nPlease copy CSV files from SD card:")
        print("  1. Activate USB mode on device (Mode 7, press ACTION)")
        print("  2. Connect USB cable to LEFT port")
        print("  3. Copy *.csv files to ../data/raw/")
        return False
    
    print(f"✅ Found {len(csv_files)} CSV files")
    return True

def run_pipeline():
    """Run complete ML training pipeline"""
    
    print("\n" + "="*60)
    print("PROXITOF ML TRAINING PIPELINE")
    print("="*60)
    
    # Check requirements
    if not check_requirements():
        return False
    
    # Check data
    if not check_data():
        return False
    
    # Step 1: Load data
    print("\n" + "="*60)
    print("STEP 1: LOADING DATA")
    print("="*60)
    
    try:
        from load_data import load_csv_files, inspect_data
        df = load_csv_files('../data/raw')
        if df is None:
            return False
        df = inspect_data(df)
        
        # Save combined data
        os.makedirs('../data/processed', exist_ok=True)
        df.to_csv('../data/processed/combined_data.csv', index=False)
        print("\n✅ Step 1 complete")
    except Exception as e:
        print(f"\n❌ Step 1 failed: {e}")
        return False
    
    # Step 2: Prepare training data
    print("\n" + "="*60)
    print("STEP 2: PREPARING TRAINING DATA")
    print("="*60)
    
    try:
        from prepare_training_data import prepare_dataset
        prepare_dataset(
            csv_file='../data/processed/combined_data.csv',
            output_dir='../data/processed',
            balance_ratio=10,
            test_size=0.2,
            random_state=42
        )
        print("\n✅ Step 2 complete")
    except Exception as e:
        print(f"\n❌ Step 2 failed: {e}")
        return False
    
    # Step 3: Train model
    print("\n" + "="*60)
    print("STEP 3: TRAINING MODEL")
    print("="*60)
    
    try:
        import numpy as np
        from train_model import train_model, evaluate_model, plot_training_history
        
        # Load data
        X_train = np.load('../data/processed/X_train.npy')
        X_test = np.load('../data/processed/X_test.npy')
        y_train = np.load('../data/processed/y_train.npy')
        y_test = np.load('../data/processed/y_test.npy')
        
        # Train
        model, history = train_model(X_train, y_train, X_test, y_test, 
                                     epochs=100, batch_size=32)
        
        # Evaluate
        metrics = evaluate_model(model, X_test, y_test)
        
        # Plot
        plot_training_history(history)
        
        # Save
        os.makedirs('../models', exist_ok=True)
        model.save('../models/near_miss_model.h5')
        
        print("\n✅ Step 3 complete")
    except Exception as e:
        print(f"\n❌ Step 3 failed: {e}")
        import traceback
        traceback.print_exc()
        return False
    
    # Step 4: Convert to TFLite
    print("\n" + "="*60)
    print("STEP 4: CONVERTING TO TFLITE")
    print("="*60)
    
    try:
        from convert_to_tflite import convert_to_tflite, test_tflite_model, generate_c_header
        
        # Convert (float32)
        convert_to_tflite('../models/near_miss_model.h5', 
                         '../models/near_miss_model.tflite', 
                         quantize=False)
        
        # Test
        test_tflite_model('../models/near_miss_model.tflite')
        
        # Generate C header
        generate_c_header('../models/near_miss_model.tflite',
                         '../components/ml_inference/model_data.h')
        
        print("\n✅ Step 4 complete")
    except Exception as e:
        print(f"\n❌ Step 4 failed: {e}")
        import traceback
        traceback.print_exc()
        return False
    
    # Success!
    print("\n" + "="*60)
    print("🎉 ML TRAINING PIPELINE COMPLETE!")
    print("="*60)
    
    print("\nGenerated files:")
    print("  ✅ ../models/near_miss_model.h5 - Trained Keras model")
    print("  ✅ ../models/near_miss_model.tflite - TFLite model")
    print("  ✅ ../models/training_history.png - Training plot")
    print("  ✅ ../components/ml_inference/model_data.h - C header for ESP32")
    
    print("\nNext steps:")
    print("  1. Copy model_data.h to your ESP32 project")
    print("  2. Implement ML inference component (see docs/ML_TRAINING_COMPLETE_GUIDE.md)")
    print("  3. Flash firmware: idf.py flash monitor")
    print("  4. Test in field and tune threshold")
    
    return True

if __name__ == '__main__':
    success = run_pipeline()
    sys.exit(0 if success else 1)
