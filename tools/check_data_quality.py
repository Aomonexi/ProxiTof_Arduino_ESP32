"""
Data Quality Checker for ProxiTof
Validates CSV files before ML training

Usage:
    python check_data_quality.py ../data/raw
"""
import os
import sys
import pandas as pd
import numpy as np

def check_csv_file(filepath):
    """Check a single CSV file for quality issues"""
    
    issues = []
    warnings = []
    
    filename = os.path.basename(filepath)
    
    try:
        df = pd.read_csv(filepath)
    except Exception as e:
        return {'file': filename, 'status': 'ERROR', 'message': str(e)}
    
    # Check 1: Required columns
    required_cols = ['timestamp', 'flag_manual']
    tof_cols = [f'tof_{r}_{c}' for r in range(4) for c in range(4)]
    # Also check old format
    tof_cols_alt = [f'tof_{r}{c}' for r in range(4) for c in range(4)]
    imu_cols = ['roll', 'pitch', 'yaw', 'accel_x', 'accel_y', 'accel_z', 'gyro_x', 'gyro_y', 'gyro_z']
    
    missing_required = [c for c in required_cols if c not in df.columns]
    if missing_required:
        issues.append(f"Missing required columns: {missing_required}")
    
    # Check ToF columns (either format)
    has_tof = any(c in df.columns for c in tof_cols) or any(c in df.columns for c in tof_cols_alt)
    if not has_tof:
        issues.append("Missing ToF columns (tof_X_Y or tof_XY format)")
    
    missing_imu = [c for c in imu_cols if c not in df.columns]
    if missing_imu:
        issues.append(f"Missing IMU columns: {missing_imu}")
    
    # Check 2: Empty file
    if len(df) == 0:
        issues.append("File is empty (0 rows)")
        return {'file': filename, 'status': 'FAIL', 'issues': issues, 'warnings': warnings}
    
    # Check 3: Missing values
    null_counts = df.isnull().sum()
    cols_with_nulls = null_counts[null_counts > 0]
    if len(cols_with_nulls) > 0:
        for col, count in cols_with_nulls.items():
            pct = count / len(df) * 100
            if pct > 10:
                issues.append(f"Column '{col}' has {pct:.1f}% missing values")
            elif pct > 1:
                warnings.append(f"Column '{col}' has {pct:.1f}% missing values")
    
    # Check 4: ToF sensor anomalies
    tof_data = df[[c for c in df.columns if c.startswith('tof_')]]
    if len(tof_data.columns) > 0:
        # All zeros (sensor disconnected)
        all_zero_rows = (tof_data == 0).all(axis=1).sum()
        if all_zero_rows > len(df) * 0.1:
            issues.append(f"{all_zero_rows} rows ({all_zero_rows/len(df)*100:.1f}%) have all-zero ToF readings")
        
        # Constant values (sensor stuck)
        for col in tof_data.columns:
            if tof_data[col].std() == 0:
                warnings.append(f"Column '{col}' has constant value (sensor stuck?)")
        
        # Out of range values
        max_range = 4000  # VL53L8CX max range ~4m
        out_of_range = (tof_data > max_range).any(axis=1).sum()
        if out_of_range > 0:
            warnings.append(f"{out_of_range} rows have ToF values > {max_range}mm")
    
    # Check 5: IMU anomalies
    if 'accel_x' in df.columns:
        # Check for reasonable acceleration values (should be around 1g when stationary)
        accel_mag = np.sqrt(df['accel_x']**2 + df['accel_y']**2 + df['accel_z']**2)
        if accel_mag.mean() < 0.5 or accel_mag.mean() > 2.0:
            warnings.append(f"Unusual acceleration magnitude: mean={accel_mag.mean():.2f}g (expected ~1g)")
    
    # Check 6: Timestamp issues
    if 'timestamp' in df.columns:
        # Non-monotonic timestamps
        non_monotonic = (df['timestamp'].diff() < 0).sum()
        if non_monotonic > 0:
            warnings.append(f"{non_monotonic} timestamp reversals detected")
        
        # Large gaps
        gaps = df['timestamp'].diff()
        large_gaps = (gaps > 1000).sum()  # > 1 second
        if large_gaps > 0:
            warnings.append(f"{large_gaps} gaps > 1 second in timestamps")
    
    # Check 7: Flag statistics
    if 'flag_manual' in df.columns:
        manual_flags = df['flag_manual'].sum()
        if manual_flags == 0:
            warnings.append("No manual flags in this file (no near-miss events marked)")
    
    # Determine status
    if issues:
        status = 'FAIL'
    elif warnings:
        status = 'WARN'
    else:
        status = 'PASS'
    
    return {
        'file': filename,
        'status': status,
        'rows': len(df),
        'issues': issues,
        'warnings': warnings,
        'manual_flags': df['flag_manual'].sum() if 'flag_manual' in df.columns else 0
    }

def check_directory(data_dir):
    """Check all CSV files in directory"""
    
    print("="*60)
    print("PROXITOF DATA QUALITY CHECK")
    print("="*60)
    print(f"Directory: {data_dir}\n")
    
    csv_files = [f for f in os.listdir(data_dir) if f.endswith('.csv')]
    
    if not csv_files:
        print(f"❌ No CSV files found in {data_dir}")
        return False
    
    print(f"Found {len(csv_files)} CSV files\n")
    
    results = []
    total_rows = 0
    total_flags = 0
    
    for f in sorted(csv_files):
        filepath = os.path.join(data_dir, f)
        result = check_csv_file(filepath)
        results.append(result)
        
        # Print result
        status_icon = {'PASS': '✅', 'WARN': '⚠️', 'FAIL': '❌', 'ERROR': '💥'}
        icon = status_icon.get(result['status'], '?')
        
        print(f"{icon} {result['file']}")
        
        if 'rows' in result:
            total_rows += result['rows']
            print(f"   Rows: {result['rows']}, Manual flags: {result.get('manual_flags', 0)}")
            total_flags += result.get('manual_flags', 0)
        
        if result.get('issues'):
            for issue in result['issues']:
                print(f"   ❌ {issue}")
        
        if result.get('warnings'):
            for warning in result['warnings']:
                print(f"   ⚠️  {warning}")
        
        print()
    
    # Summary
    print("="*60)
    print("SUMMARY")
    print("="*60)
    
    passed = sum(1 for r in results if r['status'] == 'PASS')
    warned = sum(1 for r in results if r['status'] == 'WARN')
    failed = sum(1 for r in results if r['status'] in ['FAIL', 'ERROR'])
    
    print(f"Files checked: {len(results)}")
    print(f"  ✅ Passed: {passed}")
    print(f"  ⚠️  Warnings: {warned}")
    print(f"  ❌ Failed: {failed}")
    print(f"\nTotal rows: {total_rows:,}")
    print(f"Total manual flags: {total_flags}")
    
    if total_flags > 0:
        print(f"Flag rate: {total_flags/total_rows*100:.3f}%")
    
    # Recommendations
    print("\n" + "="*60)
    print("RECOMMENDATIONS")
    print("="*60)
    
    if failed > 0:
        print("❌ Fix failed files before training")
    
    if total_flags < 50:
        print(f"⚠️  Only {total_flags} manual flags - collect more near-miss examples (target: 100+)")
    elif total_flags < 100:
        print(f"⚠️  {total_flags} manual flags - acceptable but more data recommended")
    else:
        print(f"✅ {total_flags} manual flags - good amount for training")
    
    if total_rows < 10000:
        print(f"⚠️  Only {total_rows:,} total samples - consider collecting more data")
    else:
        print(f"✅ {total_rows:,} total samples - sufficient for training")
    
    all_ok = failed == 0
    if all_ok:
        print("\n✅ Data quality check passed - ready for training!")
    else:
        print("\n❌ Fix issues before running training pipeline")
    
    return all_ok

if __name__ == '__main__':
    if len(sys.argv) < 2:
        print("Usage: python check_data_quality.py <data_directory>")
        print("Example: python check_data_quality.py ../data/raw")
        sys.exit(1)
    
    data_dir = sys.argv[1]
    
    if not os.path.exists(data_dir):
        print(f"❌ Directory not found: {data_dir}")
        sys.exit(1)
    
    success = check_directory(data_dir)
    sys.exit(0 if success else 1)
