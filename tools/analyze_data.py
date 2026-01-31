"""
Data Analysis Tool for ProxiTof Thesis
Generates statistics, visualizations, and thesis-ready tables

Usage:
    python analyze_data.py ../data/raw
    python analyze_data.py ../data/raw --output ../docs/analysis_report.md
"""
import os
import sys
import argparse
import numpy as np
import pandas as pd
from datetime import datetime

def load_all_csv(data_dir):
    """Load all CSV files from directory"""
    csv_files = [f for f in os.listdir(data_dir) if f.endswith('.csv')]
    
    if not csv_files:
        print(f"❌ No CSV files found in {data_dir}")
        return None
    
    print(f"Found {len(csv_files)} CSV files")
    
    dfs = []
    for f in csv_files:
        try:
            df = pd.read_csv(os.path.join(data_dir, f))
            df['source_file'] = f
            dfs.append(df)
            print(f"  ✅ {f}: {len(df)} rows")
        except Exception as e:
            print(f"  ❌ {f}: {e}")
    
    if not dfs:
        return None
    
    combined = pd.concat(dfs, ignore_index=True)
    print(f"\nTotal: {len(combined)} rows from {len(dfs)} files")
    return combined

def analyze_basic_stats(df):
    """Generate basic statistics"""
    stats = {
        'total_samples': len(df),
        'total_files': df['source_file'].nunique() if 'source_file' in df.columns else 1,
        'duration_seconds': (df['timestamp'].max() - df['timestamp'].min()) / 1000 if 'timestamp' in df.columns else 0,
    }
    
    # Flag statistics
    if 'flag_manual' in df.columns:
        stats['manual_flags'] = df['flag_manual'].sum()
        stats['manual_flag_rate'] = df['flag_manual'].mean() * 100
    
    if 'flag_auto' in df.columns:
        stats['auto_flags'] = df['flag_auto'].sum()
        stats['auto_flag_rate'] = df['flag_auto'].mean() * 100
    
    return stats

def analyze_tof_data(df):
    """Analyze ToF sensor data"""
    tof_cols = [c for c in df.columns if c.startswith('tof_')]
    
    if not tof_cols:
        return None
    
    tof_data = df[tof_cols]
    
    stats = {
        'min_distance_mm': tof_data.min().min(),
        'max_distance_mm': tof_data.max().max(),
        'mean_distance_mm': tof_data.mean().mean(),
        'std_distance_mm': tof_data.std().mean(),
    }
    
    # Per-zone statistics (4x4 grid)
    zone_means = tof_data.mean()
    stats['zone_means'] = zone_means.to_dict()
    
    # Distance distribution
    all_distances = tof_data.values.flatten()
    all_distances = all_distances[all_distances > 0]  # Filter invalid
    
    stats['percentile_10'] = np.percentile(all_distances, 10)
    stats['percentile_50'] = np.percentile(all_distances, 50)
    stats['percentile_90'] = np.percentile(all_distances, 90)
    
    return stats

def analyze_imu_data(df):
    """Analyze IMU sensor data"""
    imu_cols = {
        'orientation': ['roll', 'pitch', 'yaw'],
        'acceleration': ['accel_x', 'accel_y', 'accel_z'],
        'gyroscope': ['gyro_x', 'gyro_y', 'gyro_z']
    }
    
    stats = {}
    
    for category, cols in imu_cols.items():
        available_cols = [c for c in cols if c in df.columns]
        if available_cols:
            data = df[available_cols]
            stats[category] = {
                'mean': data.mean().to_dict(),
                'std': data.std().to_dict(),
                'min': data.min().to_dict(),
                'max': data.max().to_dict()
            }
    
    return stats

def analyze_near_miss_events(df):
    """Analyze near-miss events"""
    events = []
    
    # Find manual flag events
    if 'flag_manual' in df.columns:
        manual_events = df[df['flag_manual'] == 1]
        for idx, row in manual_events.iterrows():
            event = {
                'type': 'manual',
                'timestamp': row.get('timestamp', idx),
                'index': idx
            }
            
            # Get ToF data at event
            tof_cols = [c for c in df.columns if c.startswith('tof_')]
            if tof_cols:
                tof_values = row[tof_cols].values
                event['min_distance'] = tof_values[tof_values > 0].min() if any(tof_values > 0) else 0
            
            events.append(event)
    
    # Find auto flag events
    if 'flag_auto' in df.columns:
        auto_events = df[df['flag_auto'] == 1]
        for idx, row in auto_events.iterrows():
            event = {
                'type': 'auto',
                'timestamp': row.get('timestamp', idx),
                'index': idx
            }
            
            tof_cols = [c for c in df.columns if c.startswith('tof_')]
            if tof_cols:
                tof_values = row[tof_cols].values
                event['min_distance'] = tof_values[tof_values > 0].min() if any(tof_values > 0) else 0
            
            events.append(event)
    
    return events

def generate_thesis_table(stats, title):
    """Generate markdown table for thesis"""
    lines = [f"\n### {title}\n"]
    lines.append("| Metric | Value |")
    lines.append("|--------|-------|")
    
    for key, value in stats.items():
        if isinstance(value, float):
            lines.append(f"| {key.replace('_', ' ').title()} | {value:.2f} |")
        elif isinstance(value, int):
            lines.append(f"| {key.replace('_', ' ').title()} | {value:,} |")
        elif isinstance(value, dict):
            continue  # Skip nested dicts in main table
        else:
            lines.append(f"| {key.replace('_', ' ').title()} | {value} |")
    
    return '\n'.join(lines)

def generate_report(df, output_path=None):
    """Generate complete analysis report"""
    
    report = []
    report.append("# ProxiTof Data Analysis Report")
    report.append(f"\n**Generated**: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
    report.append(f"\n**Data Source**: {df['source_file'].nunique() if 'source_file' in df.columns else 1} files")
    
    # Basic statistics
    basic_stats = analyze_basic_stats(df)
    report.append(generate_thesis_table(basic_stats, "Dataset Overview"))
    
    # ToF analysis
    tof_stats = analyze_tof_data(df)
    if tof_stats:
        report.append(generate_thesis_table(tof_stats, "ToF Sensor Statistics"))
    
    # IMU analysis
    imu_stats = analyze_imu_data(df)
    if imu_stats:
        report.append("\n### IMU Sensor Statistics\n")
        for category, data in imu_stats.items():
            report.append(f"\n**{category.title()}**\n")
            report.append("| Axis | Mean | Std | Min | Max |")
            report.append("|------|------|-----|-----|-----|")
            for axis in data['mean'].keys():
                report.append(f"| {axis} | {data['mean'][axis]:.3f} | {data['std'][axis]:.3f} | {data['min'][axis]:.3f} | {data['max'][axis]:.3f} |")
    
    # Near-miss events
    events = analyze_near_miss_events(df)
    if events:
        report.append(f"\n### Near-Miss Events\n")
        report.append(f"**Total Events**: {len(events)}")
        
        manual_events = [e for e in events if e['type'] == 'manual']
        auto_events = [e for e in events if e['type'] == 'auto']
        
        report.append(f"\n- Manual flags: {len(manual_events)}")
        report.append(f"- Auto flags: {len(auto_events)}")
        
        if manual_events:
            distances = [e.get('min_distance', 0) for e in manual_events if e.get('min_distance', 0) > 0]
            if distances:
                report.append(f"\n**Manual Event Distance Statistics**:")
                report.append(f"- Mean: {np.mean(distances):.1f} mm")
                report.append(f"- Min: {np.min(distances):.1f} mm")
                report.append(f"- Max: {np.max(distances):.1f} mm")
    
    # Data quality
    report.append("\n### Data Quality\n")
    
    missing = df.isnull().sum()
    if missing.sum() > 0:
        report.append("**Missing Values**:")
        for col, count in missing[missing > 0].items():
            report.append(f"- {col}: {count} ({count/len(df)*100:.2f}%)")
    else:
        report.append("✅ No missing values detected")
    
    # Feature columns check
    expected_tof = 16
    expected_imu = 9
    
    tof_cols = [c for c in df.columns if c.startswith('tof_')]
    imu_cols = ['roll', 'pitch', 'yaw', 'accel_x', 'accel_y', 'accel_z', 'gyro_x', 'gyro_y', 'gyro_z']
    available_imu = [c for c in imu_cols if c in df.columns]
    
    report.append(f"\n**Feature Columns**:")
    report.append(f"- ToF features: {len(tof_cols)}/{expected_tof}")
    report.append(f"- IMU features: {len(available_imu)}/{expected_imu}")
    
    if len(tof_cols) == expected_tof and len(available_imu) == expected_imu:
        report.append("\n✅ All 25 features available for ML training")
    else:
        report.append("\n⚠️ Some features missing - check CSV format")
    
    # Thesis recommendations
    report.append("\n### Thesis Recommendations\n")
    
    if basic_stats.get('manual_flags', 0) < 50:
        report.append(f"⚠️ Only {basic_stats.get('manual_flags', 0)} manual flags - recommend collecting more near-miss examples (target: 100+)")
    else:
        report.append(f"✅ {basic_stats.get('manual_flags', 0)} manual flags - sufficient for initial training")
    
    safe_samples = basic_stats['total_samples'] - basic_stats.get('manual_flags', 0)
    if basic_stats.get('manual_flags', 0) > 0:
        ratio = safe_samples / basic_stats.get('manual_flags', 1)
        report.append(f"\n**Class Balance**: {ratio:.1f}:1 (safe:near-miss)")
        if ratio > 20:
            report.append("⚠️ Highly imbalanced - training script will apply class weighting")
        elif ratio < 5:
            report.append("⚠️ Low ratio - consider collecting more safe examples")
        else:
            report.append("✅ Good balance for training")
    
    report_text = '\n'.join(report)
    
    # Output
    if output_path:
        with open(output_path, 'w') as f:
            f.write(report_text)
        print(f"\n✅ Report saved to {output_path}")
    
    return report_text

def main():
    parser = argparse.ArgumentParser(description='Analyze ProxiTof data for thesis')
    parser.add_argument('data_dir', help='Directory containing CSV files')
    parser.add_argument('--output', '-o', help='Output markdown file path')
    
    args = parser.parse_args()
    
    print("="*60)
    print("PROXITOF DATA ANALYSIS")
    print("="*60)
    
    # Load data
    df = load_all_csv(args.data_dir)
    if df is None:
        sys.exit(1)
    
    # Generate report
    report = generate_report(df, args.output)
    
    # Print to console
    print("\n" + "="*60)
    print(report)
    
    print("\n" + "="*60)
    print("ANALYSIS COMPLETE")
    print("="*60)

if __name__ == '__main__':
    main()
