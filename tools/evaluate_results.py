"""
ProxiTof Chapter 7 Evaluation Script
=====================================
Compares Algorithm, ML, and Hybrid detection modes against ground truth.

Produces:
  - Dataset summary table
  - Confusion matrices for each mode
  - Performance comparison table (precision, recall, F1, specificity, FPR, accuracy)
  - Statistical comparison (McNemar's test, bootstrap confidence intervals)
  - Figures (bar charts, confusion matrix heatmaps)

Usage:
  1. Activate venv:  .\venv_ml\Scripts\activate
  2. Run training:   python tools/quick_start_ml.py
  3. Run evaluation: python tools/evaluate_results.py

Inputs:
  - data/processed/X_test.npy, y_test.npy  (from training split)
  - data/processed/scaler.pkl              (feature scaler)
  - components/ml_inference/include/model_data.h (or saved .h5 model)

Outputs:
  - data/processed/evaluation_report.txt
  - data/processed/confusion_matrices.png
  - data/processed/performance_comparison.png
  - data/processed/confidence_intervals.png
"""

import os
import sys
import numpy as np
import pandas as pd
import pickle
import warnings
warnings.filterwarnings('ignore')

# Add parent directory to path
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

PROCESSED_DIR = os.path.join(os.path.dirname(__file__), '..', 'data', 'processed')
RAW_DIR = os.path.join(os.path.dirname(__file__), '..', 'data', 'raw')
MODEL_DIR = os.path.join(os.path.dirname(__file__), '..', 'models')


# =============================================================================
# ALGORITHM DETECTION (Rule-Based) - Mirrors data_processor.cpp
# =============================================================================

# Thresholds from main.c (verified against actual code)
MIN_VALID_DISTANCE = 20              # mm
CRITICAL_PROXIMITY_DISTANCE = 100    # mm
CRITICAL_PROXIMITY_APPROACH = 500.0  # mm/s
COMBINED_DETECTION_MAX_DIST = 150    # mm
RAPID_APPROACH_THRESHOLD = 1000.0    # mm/s

def compute_derived_features(X, dt=1.0/60.0):
    """
    Compute derived features from raw sensor data.
    X columns: [tof_00..tof_33 (16), roll, pitch, yaw, ax, ay, az, gx, gy, gz]
    dt: time between frames (1/60 sec at 60Hz)
    """
    n_samples = X.shape[0]
    
    # Min distance across 16 ToF zones
    tof_data = X[:, :16]
    min_distance = np.min(tof_data, axis=1)
    
    # Approach rate (derivative of min distance)
    approach_rate = np.zeros(n_samples)
    approach_rate[1:] = (min_distance[1:] - min_distance[:-1]) / dt
    
    # Jerk magnitude (derivative of acceleration magnitude)
    accel = X[:, 19:22]  # accel_x, accel_y, accel_z
    jerk = np.zeros(n_samples)
    if n_samples > 1:
        delta_accel = np.diff(accel, axis=0) / dt
        jerk[1:] = np.linalg.norm(delta_accel, axis=1)
    
    # Angular velocity magnitude
    gyro = X[:, 22:25]  # gyro_x, gyro_y, gyro_z
    angular_velocity = np.linalg.norm(gyro, axis=1)
    
    return min_distance, approach_rate, jerk, angular_velocity


def algorithm_predict(X_raw):
    """
    Rule-based detection matching data_processor.cpp detectNearMiss().
    Input: raw (unnormalized) features, shape (n_samples, 25)
    Output: binary predictions, shape (n_samples,)
    """
    min_dist, approach_rate, jerk, ang_vel = compute_derived_features(X_raw)
    
    predictions = np.zeros(len(X_raw), dtype=int)
    
    for i in range(len(X_raw)):
        d = min_dist[i]
        v = approach_rate[i]
        
        # Validity check
        if d < MIN_VALID_DISTANCE:
            continue
        
        # Tier 1: Critical proximity
        if (d >= 20 and d < CRITICAL_PROXIMITY_DISTANCE) and (v < -CRITICAL_PROXIMITY_APPROACH):
            predictions[i] = 1
            continue
        
        # Tier 2: Proximity alert
        if (d < COMBINED_DETECTION_MAX_DIST) and (v < -RAPID_APPROACH_THRESHOLD):
            predictions[i] = 1
            continue
    
    return predictions


# =============================================================================
# ML DETECTION - Uses trained TFLite or Keras model
# =============================================================================

ML_THRESHOLD = 0.70  # From main.c: ml_detect = (ml_probability > 0.70f)

def ml_predict(X_normalized, model_path=None):
    """
    ML-based detection using trained model.
    Input: normalized features, shape (n_samples, 25)
    Output: binary predictions and probabilities
    """
    import tensorflow as tf
    
    # Try to load model
    if model_path is None:
        # Look for saved model from training
        candidates = [
            os.path.join(MODEL_DIR, 'near_miss_model.keras'),
            os.path.join(MODEL_DIR, 'near_miss_model.h5'),
            os.path.join(MODEL_DIR, 'model.keras'),
            os.path.join(MODEL_DIR, 'model.h5'),
        ]
        for c in candidates:
            if os.path.exists(c):
                model_path = c
                break
    
    if model_path is None or not os.path.exists(model_path):
        print("ERROR: No trained model found. Run quick_start_ml.py first.")
        return None, None
    
    print(f"Loading model: {model_path}")
    model = tf.keras.models.load_model(model_path)
    
    # Predict probabilities
    probs = model.predict(X_normalized, verbose=0)
    
    # Handle both single-output and two-class output
    if probs.shape[1] == 2:
        near_miss_prob = probs[:, 1]  # Class 1 = near-miss
    else:
        near_miss_prob = probs[:, 0]
    
    predictions = (near_miss_prob > ML_THRESHOLD).astype(int)
    
    return predictions, near_miss_prob


# =============================================================================
# METRICS COMPUTATION
# =============================================================================

def compute_metrics(y_true, y_pred, mode_name=""):
    """Compute all classification metrics for one detection mode."""
    from sklearn.metrics import (
        precision_score, recall_score, f1_score, 
        accuracy_score, confusion_matrix
    )
    
    tn, fp, fn, tp = confusion_matrix(y_true, y_pred, labels=[0, 1]).ravel()
    
    precision = precision_score(y_true, y_pred, zero_division=0)
    recall = recall_score(y_true, y_pred, zero_division=0)
    f1 = f1_score(y_true, y_pred, zero_division=0)
    accuracy = accuracy_score(y_true, y_pred)
    specificity = tn / (tn + fp) if (tn + fp) > 0 else 0
    fpr = fp / (fp + tn) if (fp + tn) > 0 else 0
    
    return {
        'mode': mode_name,
        'tp': tp, 'tn': tn, 'fp': fp, 'fn': fn,
        'precision': precision,
        'recall': recall,
        'f1_score': f1,
        'specificity': specificity,
        'fpr': fpr,
        'accuracy': accuracy,
        'auc_roc': None,  # Set later if probabilities available
    }


# =============================================================================
# STATISTICAL TESTS
# =============================================================================

def mcnemar_test(y_true, pred_a, pred_b):
    """
    McNemar's test comparing two classifiers.
    Returns chi2 statistic and p-value.
    """
    from scipy.stats import chi2
    
    # Build contingency table
    # b = A correct, B wrong
    # c = A wrong, B correct
    a_correct = (pred_a == y_true)
    b_correct = (pred_b == y_true)
    
    b_count = np.sum(a_correct & ~b_correct)  # A right, B wrong
    c_count = np.sum(~a_correct & b_correct)  # A wrong, B right
    
    # McNemar's test with continuity correction
    if (b_count + c_count) == 0:
        return 0.0, 1.0
    
    chi2_stat = (abs(b_count - c_count) - 1) ** 2 / (b_count + c_count)
    p_value = 1 - chi2.cdf(chi2_stat, df=1)
    
    return chi2_stat, p_value


def bootstrap_f1_ci(y_true, y_pred, n_bootstrap=1000, ci=0.95, seed=42):
    """
    Bootstrap 95% confidence interval for F1-score.
    """
    from sklearn.metrics import f1_score
    
    rng = np.random.RandomState(seed)
    n = len(y_true)
    f1_scores = []
    
    for _ in range(n_bootstrap):
        idx = rng.randint(0, n, size=n)
        y_t = y_true[idx]
        y_p = y_pred[idx]
        
        # Skip if only one class in bootstrap sample
        if len(np.unique(y_t)) < 2:
            continue
        
        f1_scores.append(f1_score(y_t, y_p, zero_division=0))
    
    f1_scores = np.array(f1_scores)
    alpha = (1 - ci) / 2
    lower = np.percentile(f1_scores, alpha * 100)
    upper = np.percentile(f1_scores, (1 - alpha) * 100)
    mean = np.mean(f1_scores)
    
    return mean, lower, upper


# =============================================================================
# PLOTTING
# =============================================================================

def plot_confusion_matrices(results, y_true, predictions_dict, output_path):
    """Plot confusion matrix heatmaps for all three modes."""
    import matplotlib.pyplot as plt
    from sklearn.metrics import confusion_matrix
    import matplotlib
    matplotlib.use('Agg')
    
    fig, axes = plt.subplots(1, 3, figsize=(15, 4))
    
    for idx, (mode_name, y_pred) in enumerate(predictions_dict.items()):
        cm = confusion_matrix(y_true, y_pred, labels=[0, 1])
        ax = axes[idx]
        
        im = ax.imshow(cm, interpolation='nearest', cmap='Blues')
        ax.set_title(f'{mode_name}', fontsize=12, fontweight='bold')
        ax.set_xlabel('Predicted')
        ax.set_ylabel('Actual')
        ax.set_xticks([0, 1])
        ax.set_yticks([0, 1])
        ax.set_xticklabels(['Safe', 'Near-Miss'])
        ax.set_yticklabels(['Safe', 'Near-Miss'])
        
        # Add text annotations
        for i in range(2):
            for j in range(2):
                ax.text(j, i, str(cm[i, j]), ha='center', va='center',
                       fontsize=14, fontweight='bold',
                       color='white' if cm[i, j] > cm.max()/2 else 'black')
    
    plt.tight_layout()
    plt.savefig(output_path, dpi=150, bbox_inches='tight')
    plt.close()
    print(f"  Saved: {output_path}")


def plot_performance_comparison(results_list, output_path):
    """Bar chart comparing precision, recall, F1 for all modes."""
    import matplotlib.pyplot as plt
    import matplotlib
    matplotlib.use('Agg')
    
    modes = [r['mode'] for r in results_list]
    metrics = ['precision', 'recall', 'f1_score']
    labels = ['Precision', 'Recall', 'F1-Score']
    
    x = np.arange(len(modes))
    width = 0.25
    
    fig, ax = plt.subplots(figsize=(8, 5))
    
    for i, (metric, label) in enumerate(zip(metrics, labels)):
        values = [r[metric] for r in results_list]
        bars = ax.bar(x + i * width, values, width, label=label)
        # Add value labels on bars
        for bar, val in zip(bars, values):
            ax.text(bar.get_x() + bar.get_width()/2, bar.get_height() + 0.01,
                   f'{val:.3f}', ha='center', va='bottom', fontsize=9)
    
    ax.set_xlabel('Detection Mode')
    ax.set_ylabel('Score')
    ax.set_title('Detection Performance Comparison')
    ax.set_xticks(x + width)
    ax.set_xticklabels(modes)
    ax.set_ylim(0, 1.15)
    ax.legend()
    ax.grid(axis='y', alpha=0.3)
    
    plt.tight_layout()
    plt.savefig(output_path, dpi=150, bbox_inches='tight')
    plt.close()
    print(f"  Saved: {output_path}")


def plot_confidence_intervals(ci_data, output_path):
    """Bar chart with 95% CI error bars for F1-score."""
    import matplotlib.pyplot as plt
    import matplotlib
    matplotlib.use('Agg')
    
    modes = [d['mode'] for d in ci_data]
    means = [d['mean'] for d in ci_data]
    lowers = [d['lower'] for d in ci_data]
    uppers = [d['upper'] for d in ci_data]
    
    errors = [[m - l for m, l in zip(means, lowers)],
              [u - m for m, u in zip(means, uppers)]]
    
    fig, ax = plt.subplots(figsize=(6, 5))
    bars = ax.bar(modes, means, yerr=errors, capsize=8, color=['#4C72B0', '#55A868', '#C44E52'])
    
    for bar, m, l, u in zip(bars, means, lowers, uppers):
        ax.text(bar.get_x() + bar.get_width()/2, bar.get_height() + 0.03,
               f'{m:.3f}\n[{l:.3f}, {u:.3f}]', ha='center', va='bottom', fontsize=9)
    
    ax.set_ylabel('F1-Score')
    ax.set_title('F1-Score with 95% Bootstrap Confidence Intervals')
    ax.set_ylim(0, 1.2)
    ax.grid(axis='y', alpha=0.3)
    
    plt.tight_layout()
    plt.savefig(output_path, dpi=150, bbox_inches='tight')
    plt.close()
    print(f"  Saved: {output_path}")


# =============================================================================
# MAIN EVALUATION
# =============================================================================

def generate_report(results_list, stat_results, ci_data, dataset_info, output_path):
    """Generate text report for thesis Chapter 7."""
    
    with open(output_path, 'w') as f:
        f.write("=" * 70 + "\n")
        f.write("PROXITOF EVALUATION REPORT - Chapter 7 Results\n")
        f.write("=" * 70 + "\n\n")
        
        # Table 7.1: Dataset Overview
        f.write("Table 7.1: Dataset Overview\n")
        f.write("-" * 40 + "\n")
        for key, val in dataset_info.items():
            f.write(f"  {key:30s}: {val}\n")
        f.write("\n")
        
        # Table 7.2: Main Performance Comparison
        f.write("Table 7.2: Detection Performance Comparison\n")
        f.write("-" * 80 + "\n")
        header = f"{'Mode':12s} {'Prec':>8s} {'Recall':>8s} {'F1':>8s} {'Spec':>8s} {'FPR':>8s} {'Acc':>8s} {'AUC':>8s}"
        f.write(header + "\n")
        f.write("-" * 80 + "\n")
        for r in results_list:
            auc_str = f"{r['auc_roc']:8.4f}" if r.get('auc_roc') is not None else "     N/A"
            line = f"{r['mode']:12s} {r['precision']:8.4f} {r['recall']:8.4f} {r['f1_score']:8.4f} "
            line += f"{r['specificity']:8.4f} {r['fpr']:8.4f} {r['accuracy']:8.4f} {auc_str}"
            f.write(line + "\n")
        f.write("\n")
        
        # Confusion matrices
        f.write("Confusion Matrices (TP, TN, FP, FN)\n")
        f.write("-" * 50 + "\n")
        for r in results_list:
            f.write(f"  {r['mode']:12s}: TP={r['tp']:5d}  TN={r['tn']:5d}  FP={r['fp']:5d}  FN={r['fn']:5d}\n")
        f.write("\n")
        
        # Table 7.3: Statistical Comparison
        f.write("Table 7.3: Statistical Comparison (McNemar's Test)\n")
        f.write("-" * 60 + "\n")
        header = f"{'Comparison':25s} {'Chi2':>10s} {'p-value':>10s} {'Significant':>12s}"
        f.write(header + "\n")
        f.write("-" * 60 + "\n")
        alpha = 0.05 / 3  # Bonferroni correction for 3 comparisons
        for s in stat_results:
            sig = "Yes" if s['p_value'] < alpha else "No"
            f.write(f"{s['comparison']:25s} {s['chi2']:10.4f} {s['p_value']:10.6f} {sig:>12s}\n")
        f.write(f"\n  Bonferroni-corrected alpha: {alpha:.4f} (0.05 / 3 comparisons)\n\n")
        
        # Table 7.4: Bootstrap Confidence Intervals
        f.write("Table 7.4: F1-Score with 95% Bootstrap Confidence Intervals\n")
        f.write("-" * 50 + "\n")
        for d in ci_data:
            f.write(f"  {d['mode']:12s}: F1 = {d['mean']:.4f}  [{d['lower']:.4f}, {d['upper']:.4f}]\n")
        f.write("\n")
        
        # Table 7.5: Embedded Performance (known values from code)
        f.write("Table 7.5: Embedded Performance\n")
        f.write("-" * 40 + "\n")
        f.write(f"  {'Model architecture':30s}: 25 -> 32 -> 16 -> 2 (dense)\n")
        f.write(f"  {'Model parameters':30s}: ~1,394\n")
        f.write(f"  {'Model size (TFLite)':30s}: 7.9 KB\n")
        f.write(f"  {'Tensor arena':30s}: 12 KB\n")
        f.write(f"  {'Total ML RAM':30s}: ~14 KB\n")
        f.write(f"  {'Inference time (ESP-NN)':30s}: 5-15 ms\n")
        f.write(f"  {'Target frame rate':30s}: 60 Hz (16.7 ms budget)\n")
        f.write(f"  {'CPU overhead (ML)':30s}: 30-90% of frame budget\n")
        f.write(f"  {'CPU overhead (Algo only)':30s}: <1% of frame budget\n")
        f.write(f"  {'ESP-NN acceleration':30s}: SIMD (FullyConnected, ReLU)\n")
        f.write(f"  {'Detection threshold':30s}: P(near-miss) > 0.70\n")
        f.write(f"  {'Flash usage (firmware)':30s}: ~1.2 MB / 1.5 MB partition\n")
        f.write("\n")
        
        f.write("=" * 70 + "\n")
        f.write("END OF REPORT\n")
        f.write("=" * 70 + "\n")
    
    print(f"  Saved: {output_path}")


def main():
    print("\n" + "=" * 60)
    print("PROXITOF EVALUATION - Chapter 7 Results Generator")
    print("=" * 60)
    
    # =========================================================================
    # Step 1: Load test data
    # =========================================================================
    print("\n[1/6] Loading test data...")
    
    X_test_path = os.path.join(PROCESSED_DIR, 'X_test.npy')
    y_test_path = os.path.join(PROCESSED_DIR, 'y_test.npy')
    scaler_path = os.path.join(PROCESSED_DIR, 'scaler.pkl')
    
    if not all(os.path.exists(p) for p in [X_test_path, y_test_path, scaler_path]):
        print("ERROR: Test data not found. Run quick_start_ml.py first.")
        print(f"  Expected: {X_test_path}")
        print(f"  Expected: {y_test_path}")
        print(f"  Expected: {scaler_path}")
        return
    
    X_test_norm = np.load(X_test_path)
    y_test = np.load(y_test_path)
    
    with open(scaler_path, 'rb') as f:
        scaler = pickle.load(f)
    
    # Inverse transform to get raw values for algorithm detection
    X_test_raw = scaler.inverse_transform(X_test_norm)
    
    n_total = len(y_test)
    n_positive = int(y_test.sum())
    n_negative = n_total - n_positive
    
    print(f"  Test samples: {n_total}")
    print(f"  Near-miss (1): {n_positive}")
    print(f"  Safe (0): {n_negative}")
    print(f"  Class ratio: {n_negative}:{n_positive} (safe:near-miss)")
    
    # Also load full dataset stats if available
    dataset_info = {
        'Total test samples': str(n_total),
        'Near-miss samples': str(n_positive),
        'Safe samples': str(n_negative),
        'Class ratio (safe:nm)': f"{n_negative/max(n_positive,1):.1f}:1",
        'Features per sample': '25 (16 ToF + 9 IMU)',
        'Ground truth source': 'flag_manual (button press)',
    }
    
    # Try to get full dataset info
    try:
        X_train = np.load(os.path.join(PROCESSED_DIR, 'X_train.npy'))
        y_train = np.load(os.path.join(PROCESSED_DIR, 'y_train.npy'))
        total_samples = len(X_train) + len(X_test_norm)
        total_nm = int(y_train.sum()) + n_positive
        total_safe = total_samples - total_nm
        total_seconds = total_samples / 60.0
        dataset_info['Total samples (train+test)'] = str(total_samples)
        dataset_info['Total near-miss (all)'] = str(total_nm)
        dataset_info['Total safe (all)'] = str(total_safe)
        dataset_info['Approx duration'] = f"{total_seconds:.0f}s ({total_seconds/60:.1f} min)"
        dataset_info['Train/test split'] = '80/20'
    except:
        pass
    
    # =========================================================================
    # Step 2: Run Algorithm detection
    # =========================================================================
    print("\n[2/6] Running Algorithm detection (rule-based)...")
    pred_algo = algorithm_predict(X_test_raw)
    print(f"  Algorithm detections: {pred_algo.sum()} / {n_total}")
    
    # =========================================================================
    # Step 3: Run ML detection
    # =========================================================================
    print("\n[3/6] Running ML detection (neural network)...")
    pred_ml, ml_probs = ml_predict(X_test_norm)
    
    if pred_ml is None:
        print("  ML detection unavailable. Using zeros.")
        pred_ml = np.zeros(n_total, dtype=int)
        ml_probs = np.zeros(n_total)
    else:
        print(f"  ML detections: {pred_ml.sum()} / {n_total}")
        print(f"  Mean probability: {ml_probs.mean():.4f}")
    
    # =========================================================================
    # Step 4: Compute Hybrid (Algorithm OR ML)
    # =========================================================================
    print("\n[4/6] Computing Hybrid detection (Algorithm OR ML)...")
    pred_hybrid = ((pred_algo == 1) | (pred_ml == 1)).astype(int)
    print(f"  Hybrid detections: {pred_hybrid.sum()} / {n_total}")
    
    # =========================================================================
    # Step 5: Compute all metrics
    # =========================================================================
    print("\n[5/6] Computing metrics...")
    
    results_algo = compute_metrics(y_test, pred_algo, "Algorithm")
    results_ml = compute_metrics(y_test, pred_ml, "ML")
    results_hybrid = compute_metrics(y_test, pred_hybrid, "Hybrid")
    
    # Compute AUC-ROC where probabilities are available
    from sklearn.metrics import roc_auc_score
    try:
        # ML has continuous probabilities — proper AUC-ROC
        results_ml['auc_roc'] = roc_auc_score(y_test, ml_probs)
        # Algorithm and Hybrid only have binary outputs — AUC = accuracy-like measure
        results_algo['auc_roc'] = roc_auc_score(y_test, pred_algo)
        results_hybrid['auc_roc'] = roc_auc_score(y_test, pred_hybrid)
    except ValueError:
        print("  Warning: AUC-ROC could not be computed (single class in test set?)")
    
    results_list = [results_algo, results_ml, results_hybrid]
    
    # Print summary
    print(f"\n  {'Mode':12s} {'Precision':>10s} {'Recall':>10s} {'F1':>10s} {'Accuracy':>10s}")
    print(f"  {'-'*52}")
    for r in results_list:
        print(f"  {r['mode']:12s} {r['precision']:10.4f} {r['recall']:10.4f} "
              f"{r['f1_score']:10.4f} {r['accuracy']:10.4f}")
    
    # Statistical tests
    print("\n  McNemar's Tests:")
    stat_results = []
    comparisons = [
        ("Algorithm vs ML", pred_algo, pred_ml),
        ("Algorithm vs Hybrid", pred_algo, pred_hybrid),
        ("ML vs Hybrid", pred_ml, pred_hybrid),
    ]
    for name, pa, pb in comparisons:
        chi2_stat, p_val = mcnemar_test(y_test, pa, pb)
        stat_results.append({'comparison': name, 'chi2': chi2_stat, 'p_value': p_val})
        sig = "***" if p_val < 0.05/3 else ""
        print(f"    {name:25s}: chi2={chi2_stat:.4f}, p={p_val:.6f} {sig}")
    
    # Bootstrap confidence intervals
    print("\n  Bootstrap 95% CI for F1-score:")
    ci_data = []
    for name, pred in [("Algorithm", pred_algo), ("ML", pred_ml), ("Hybrid", pred_hybrid)]:
        mean, lower, upper = bootstrap_f1_ci(y_test, pred)
        ci_data.append({'mode': name, 'mean': mean, 'lower': lower, 'upper': upper})
        print(f"    {name:12s}: {mean:.4f} [{lower:.4f}, {upper:.4f}]")
    
    # =========================================================================
    # Step 6: Generate outputs
    # =========================================================================
    print("\n[6/6] Generating outputs...")
    
    os.makedirs(PROCESSED_DIR, exist_ok=True)
    
    # Confusion matrices figure
    predictions_dict = {"Algorithm": pred_algo, "ML": pred_ml, "Hybrid": pred_hybrid}
    plot_confusion_matrices(results_list, y_test, predictions_dict,
                           os.path.join(PROCESSED_DIR, 'confusion_matrices.png'))
    
    # Performance comparison bar chart
    plot_performance_comparison(results_list,
                               os.path.join(PROCESSED_DIR, 'performance_comparison.png'))
    
    # Confidence interval chart
    plot_confidence_intervals(ci_data,
                             os.path.join(PROCESSED_DIR, 'confidence_intervals.png'))
    
    # Text report
    generate_report(results_list, stat_results, ci_data, dataset_info,
                    os.path.join(PROCESSED_DIR, 'evaluation_report.txt'))
    
    # Save raw results as CSV for further analysis
    results_df = pd.DataFrame(results_list)
    results_df.to_csv(os.path.join(PROCESSED_DIR, 'performance_metrics.csv'), index=False)
    print(f"  Saved: {os.path.join(PROCESSED_DIR, 'performance_metrics.csv')}")
    
    # Save predictions for detailed analysis
    pred_df = pd.DataFrame({
        'y_true': y_test,
        'pred_algo': pred_algo,
        'pred_ml': pred_ml,
        'pred_hybrid': pred_hybrid,
        'ml_probability': ml_probs,
    })
    pred_df.to_csv(os.path.join(PROCESSED_DIR, 'predictions.csv'), index=False)
    print(f"  Saved: {os.path.join(PROCESSED_DIR, 'predictions.csv')}")
    
    print("\n" + "=" * 60)
    print("EVALUATION COMPLETE")
    print("=" * 60)
    print(f"\nOutputs in: {os.path.abspath(PROCESSED_DIR)}")
    print("  - evaluation_report.txt     (copy into thesis Chapter 7)")
    print("  - confusion_matrices.png    (Figure: confusion matrices)")
    print("  - performance_comparison.png (Figure: bar chart)")
    print("  - confidence_intervals.png  (Figure: CI error bars)")
    print("  - performance_metrics.csv   (raw metrics table)")
    print("  - predictions.csv           (per-sample predictions)")


if __name__ == '__main__':
    main()
