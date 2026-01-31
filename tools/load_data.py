"""
Load and inspect ProxiTof CSV data
"""
import pandas as pd
import numpy as np
import glob
import os

def load_csv_files(data_dir='../data/raw'):
    """Load all CSV files from directory"""
    csv_files = glob.glob(os.path.join(data_dir, '*.csv'))
    
    if not csv_files:
        print(f"❌ No CSV files found in {data_dir}")
        return None
    
    print(f"Found {len(csv_files)} CSV files")
    
    # Load all files
    dfs = []
    for csv_file in csv_files:
        print(f"Loading {os.path.basename(csv_file)}...")
        try:
            df = pd.read_csv(csv_file)
            dfs.append(df)
        except Exception as e:
            print(f"  ⚠️  Error loading {csv_file}: {e}")
    
    # Combine all dataframes
    df_combined = pd.concat(dfs, ignore_index=True)
    
    print(f"\n✅ Loaded {len(df_combined)} total samples")
    return df_combined

def inspect_data(df):
    """Print data statistics"""
    print("\n" + "="*60)
    print("DATA INSPECTION")
    print("="*60)
    
    # Basic info
    print(f"\nTotal samples: {len(df)}")
    print(f"Columns: {len(df.columns)}")
    print(f"Memory usage: {df.memory_usage(deep=True).sum() / 1024**2:.2f} MB")
    
    # Check for required columns
    required_cols = ['timestamp_ms', 'flag_manual']
    tof_cols = [f'tof_{i:02d}' for i in range(16)]
    imu_cols = ['roll', 'pitch', 'yaw', 'accel_x', 'accel_y', 'accel_z', 
                'gyro_x', 'gyro_y', 'gyro_z']
    
    all_required = required_cols + tof_cols + imu_cols
    missing = [col for col in all_required if col not in df.columns]
    
    if missing:
        print(f"\n⚠️  Missing columns: {missing}")
    else:
        print("\n✅ All required columns present")
    
    # Label distribution
    print("\n" + "-"*60)
    print("LABEL DISTRIBUTION")
    print("-"*60)
    
    if 'flag_manual' in df.columns:
        n_near_miss = df['flag_manual'].sum()
        n_safe = len(df) - n_near_miss
        
        print(f"Near-miss (flag_manual=1): {n_near_miss} ({100*n_near_miss/len(df):.2f}%)")
        print(f"Safe (flag_manual=0):      {n_safe} ({100*n_safe/len(df):.2f}%)")
        print(f"Ratio (safe:near-miss):    {n_safe/n_near_miss:.1f}:1" if n_near_miss > 0 else "N/A")
        
        if n_near_miss < 100:
            print(f"\n⚠️  WARNING: Only {n_near_miss} near-miss examples!")
            print("   Recommendation: Collect at least 100-200 near-miss examples")
    
    # Sensor data ranges
    print("\n" + "-"*60)
    print("SENSOR DATA RANGES")
    print("-"*60)
    
    if all(col in df.columns for col in tof_cols):
        tof_data = df[tof_cols].values
        print(f"ToF distances:  min={tof_data.min():.0f}mm, max={tof_data.max():.0f}mm, mean={tof_data.mean():.0f}mm")
    
    if 'roll' in df.columns:
        print(f"Roll:           min={df['roll'].min():.1f}°, max={df['roll'].max():.1f}°")
    if 'pitch' in df.columns:
        print(f"Pitch:          min={df['pitch'].min():.1f}°, max={df['pitch'].max():.1f}°")
    if 'accel_x' in df.columns:
        accel_mag = np.sqrt(df['accel_x']**2 + df['accel_y']**2 + df['accel_z']**2)
        print(f"Acceleration:   min={accel_mag.min():.2f}m/s², max={accel_mag.max():.2f}m/s²")
    
    # Missing values
    print("\n" + "-"*60)
    print("DATA QUALITY")
    print("-"*60)
    
    missing_count = df.isnull().sum().sum()
    if missing_count > 0:
        print(f"⚠️  Missing values: {missing_count}")
        print("\nColumns with missing values:")
        for col in df.columns:
            n_missing = df[col].isnull().sum()
            if n_missing > 0:
                print(f"  {col}: {n_missing} ({100*n_missing/len(df):.2f}%)")
    else:
        print("✅ No missing values")
    
    # Duplicates
    n_duplicates = df.duplicated().sum()
    if n_duplicates > 0:
        print(f"\n⚠️  Duplicate rows: {n_duplicates}")
    else:
        print("✅ No duplicate rows")
    
    return df

if __name__ == '__main__':
    # Load data
    df = load_csv_files('../data/raw')
    
    if df is not None:
        # Inspect
        df = inspect_data(df)
        
        # Save combined dataset
        output_file = '../data/processed/combined_data.csv'
        os.makedirs(os.path.dirname(output_file), exist_ok=True)
        df.to_csv(output_file, index=False)
        print(f"\n✅ Combined dataset saved to {output_file}")
