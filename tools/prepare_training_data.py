"""
Prepare training data for ESP-NN model
- Extract features and labels
- Balance dataset
- Normalize features
- Split train/test
"""
import pandas as pd
import numpy as np
from sklearn.model_selection import train_test_split
from sklearn.preprocessing import StandardScaler
import pickle
import os

def extract_features_and_labels(df):
    """
    Extract features (X) and labels (y) from dataset
    
    Features: 25 sensor values
    - 16 ToF distances (tof_00 to tof_33 in 4x4 grid)
    - 3 Orientation (roll, pitch, yaw)
    - 3 Acceleration (accel_x, accel_y, accel_z)
    - 3 Gyroscope (gyro_x, gyro_y, gyro_z)
    
    Labels: 0 = safe, 1 = near-miss (from flag_manual ONLY)
    
    IMPORTANT: flag_auto is EXCLUDED - it's the model's output, not input!
    """
    
    # ToF columns (4x4 grid: row 0-3, col 0-3)
    # Format: tof_00, tof_01, tof_02, tof_03, tof_10, tof_11, ..., tof_33
    tof_cols = [f'tof_{row}{col}' for row in range(4) for col in range(4)]
    
    # IMU columns
    imu_cols = ['roll', 'pitch', 'yaw', 
                'accel_x', 'accel_y', 'accel_z',
                'gyro_x', 'gyro_y', 'gyro_z']
    
    feature_cols = tof_cols + imu_cols
    
    # Check if all columns exist
    missing = [col for col in feature_cols if col not in df.columns]
    if missing:
        raise ValueError(f"Missing columns: {missing}")
    
    # Extract features (25 total)
    X = df[feature_cols].values.astype(np.float32)
    
    # Extract labels (ONLY flag_manual, NOT flag_auto!)
    # flag_auto is the model's prediction, not training input!
    y = df['flag_manual'].values.astype(np.int32)
    
    print(f"\n{'='*60}")
    print("FEATURE EXTRACTION")
    print(f"{'='*60}")
    print(f"Feature shape: {X.shape}")
    print(f"Label shape: {y.shape}")
    print(f"Feature columns: {len(feature_cols)}")
    print(f"  - ToF: {len(tof_cols)} (4x4 grid)")
    print(f"  - IMU: {len(imu_cols)}")
    print(f"\nLabel source: flag_manual ONLY")
    print(f"  ⚠️  flag_auto is EXCLUDED (it's the model output!)")
    
    return X, y, feature_cols

def balance_dataset(X, y, ratio=10, random_state=42):
    """
    Balance dataset by undersampling majority class
    
    Args:
        ratio: Ratio of safe to near-miss samples (e.g., 10:1)
        random_state: Random seed for reproducibility
    """
    
    np.random.seed(random_state)
    
    # Separate classes
    near_miss_idx = np.where(y == 1)[0]
    safe_idx = np.where(y == 0)[0]
    
    n_near_miss = len(near_miss_idx)
    n_safe = len(safe_idx)
    
    print(f"\n{'='*60}")
    print("DATASET BALANCING")
    print(f"{'='*60}")
    print(f"Original dataset:")
    print(f"  Near-miss: {n_near_miss} ({100*n_near_miss/(n_near_miss+n_safe):.2f}%)")
    print(f"  Safe:      {n_safe} ({100*n_safe/(n_near_miss+n_safe):.2f}%)")
    print(f"  Ratio:     {n_safe/n_near_miss:.1f}:1")
    
    # Undersample safe class
    n_safe_keep = min(n_safe, n_near_miss * ratio)
    safe_idx_keep = np.random.choice(safe_idx, n_safe_keep, replace=False)
    
    # Combine
    keep_idx = np.concatenate([near_miss_idx, safe_idx_keep])
    np.random.shuffle(keep_idx)
    
    X_balanced = X[keep_idx]
    y_balanced = y[keep_idx]
    
    print(f"\nBalanced dataset (target ratio {ratio}:1):")
    print(f"  Near-miss: {y_balanced.sum()} ({100*y_balanced.sum()/len(y_balanced):.2f}%)")
    print(f"  Safe:      {len(y_balanced) - y_balanced.sum()} ({100*(len(y_balanced)-y_balanced.sum())/len(y_balanced):.2f}%)")
    print(f"  Ratio:     {(len(y_balanced) - y_balanced.sum()) / y_balanced.sum():.1f}:1")
    print(f"  Total:     {len(y_balanced)} samples")
    
    return X_balanced, y_balanced

def normalize_features(X_train, X_test):
    """
    Normalize features to zero mean and unit variance
    This is CRITICAL for neural network training!
    """
    
    scaler = StandardScaler()
    X_train_norm = scaler.fit_transform(X_train)
    X_test_norm = scaler.transform(X_test)
    
    print(f"\n{'='*60}")
    print("FEATURE NORMALIZATION")
    print(f"{'='*60}")
    print(f"Training set:")
    print(f"  Before: mean={X_train.mean():.2f}, std={X_train.std():.2f}")
    print(f"  After:  mean={X_train_norm.mean():.2f}, std={X_train_norm.std():.2f}")
    
    return X_train_norm, X_test_norm, scaler

def prepare_dataset(csv_file, output_dir='../data/processed', 
                   balance_ratio=10, test_size=0.2, random_state=42):
    """
    Complete data preparation pipeline
    """
    
    print(f"\n{'='*60}")
    print("PREPARING TRAINING DATA")
    print(f"{'='*60}")
    print(f"Input file: {csv_file}")
    print(f"Output directory: {output_dir}")
    
    # Load data
    print("\nLoading data...")
    df = pd.read_csv(csv_file)
    print(f"✅ Loaded {len(df)} samples")
    
    # Extract features and labels
    X, y, feature_cols = extract_features_and_labels(df)
    
    # Balance dataset
    X_balanced, y_balanced = balance_dataset(X, y, ratio=balance_ratio, 
                                            random_state=random_state)
    
    # Split train/test
    print(f"\n{'='*60}")
    print("TRAIN/TEST SPLIT")
    print(f"{'='*60}")
    X_train, X_test, y_train, y_test = train_test_split(
        X_balanced, y_balanced, 
        test_size=test_size, 
        random_state=random_state, 
        stratify=y_balanced
    )
    print(f"Training set:   {len(X_train)} samples ({100*(1-test_size):.0f}%)")
    print(f"Test set:       {len(X_test)} samples ({100*test_size:.0f}%)")
    
    # Normalize features
    X_train_norm, X_test_norm, scaler = normalize_features(X_train, X_test)
    
    # Save processed data
    os.makedirs(output_dir, exist_ok=True)
    
    np.save(os.path.join(output_dir, 'X_train.npy'), X_train_norm)
    np.save(os.path.join(output_dir, 'X_test.npy'), X_test_norm)
    np.save(os.path.join(output_dir, 'y_train.npy'), y_train)
    np.save(os.path.join(output_dir, 'y_test.npy'), y_test)
    
    # Save scaler (needed for deployment!)
    with open(os.path.join(output_dir, 'scaler.pkl'), 'wb') as f:
        pickle.dump(scaler, f)
    
    # Save scaler parameters as numpy arrays (for C header generation)
    np.savez(os.path.join(output_dir, 'scaler_params.npz'),
             mean=scaler.mean_,
             std=scaler.scale_)
    
    # Save feature names
    with open(os.path.join(output_dir, 'feature_names.txt'), 'w') as f:
        for col in feature_cols:
            f.write(f"{col}\n")
    
    print(f"\n{'='*60}")
    print("SAVED FILES")
    print(f"{'='*60}")
    print(f"✅ X_train.npy       - Training features ({X_train_norm.shape})")
    print(f"✅ X_test.npy        - Test features ({X_test_norm.shape})")
    print(f"✅ y_train.npy       - Training labels ({y_train.shape})")
    print(f"✅ y_test.npy        - Test labels ({y_test.shape})")
    print(f"✅ scaler.pkl        - Feature scaler (for deployment)")
    print(f"✅ scaler_params.npz - Scaler mean/std (for C header)")
    print(f"✅ feature_names.txt - Feature column names")
    
    print(f"\n🎉 Data preparation complete!")
    print(f"Next step: Run train_model.py to train the neural network")
    
    return X_train_norm, X_test_norm, y_train, y_test, scaler

if __name__ == '__main__':
    # Prepare dataset
    prepare_dataset(
        csv_file='../data/processed/combined_data.csv',
        output_dir='../data/processed',
        balance_ratio=10,
        test_size=0.2,
        random_state=42
    )
