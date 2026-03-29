"""
Thesis Chapter 7 — Table Generator
====================================
Generates all five thesis tables in one report file.
Does NOT retrain or re-evaluate — reads from existing output files.

Prerequisites (run these first):
  python evaluate_results.py          -> produces performance_metrics.csv
  python evaluate_sliding_window.py   -> produces sw_evaluation_report.txt

Output:
  data/processed/thesis_chapter7_tables.txt

Usage:
  cd tools
  ..\venv_ml\Scripts\activate
  python generate_thesis_tables.py
"""

import os
import sys
import numpy as np
import pandas as pd

ROOT  = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
PROC  = os.path.join(ROOT, "data", "processed")
OUT   = os.path.join(PROC, "thesis_chapter7_tables.txt")


# ── helpers ────────────────────────────────────────────────────────────────

def load_sf_metrics():
    path = os.path.join(PROC, "performance_metrics.csv")
    if not os.path.exists(path):
        sys.exit("ERROR: performance_metrics.csv not found — run evaluate_results.py first")
    return pd.read_csv(path)


def load_sw_metrics():
    """Parse sw_evaluation_report.txt for the sliding window numbers."""
    path = os.path.join(PROC, "sw_evaluation_report.txt")
    if not os.path.exists(path):
        sys.exit("ERROR: sw_evaluation_report.txt not found — run evaluate_sliding_window.py first")
    result = {}
    with open(path, encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            for key in ["precision", "recall", "f1", "accuracy", "auc_roc", "fpr"]:
                if line.startswith(key):
                    result[key] = float(line.split(":")[1].strip())
            if line.startswith("F1 95% CI"):
                # e.g.  F1 95% CI  : [0.7652, 0.7955]
                bracket = line.split("[")[1].rstrip("]")
                lo, hi = bracket.split(",")
                result["ci_lo"] = float(lo.strip())
                result["ci_hi"] = float(hi.strip())
            if line.startswith("TP="):
                parts = line.replace("TP=","").replace("TN=","").replace("FP=","").replace("FN=","").split()
                result["tp"] = int(parts[0])
                result["tn"] = int(parts[1])
                result["fp"] = int(parts[2])
                result["fn"] = int(parts[3])
            if "Test windows" in line:
                result["n_test"] = int(line.split(":")[1].strip())
            if "Near-miss" in line and "n_test" in result and "n_pos" not in result:
                try:
                    result["n_pos"] = int(line.split(":")[1].strip())
                except Exception:
                    pass
    return result


def load_dataset_info():
    """Load dataset stats from X_train / y_train / X_test / y_test."""
    paths = {k: os.path.join(PROC, f"{k}.npy")
             for k in ["X_train", "X_test", "y_train", "y_test"]}
    missing = [k for k, p in paths.items() if not os.path.exists(p)]
    if missing:
        sys.exit(f"ERROR: missing {missing} — run evaluate_results.py first")

    X_train = np.load(paths["X_train"])
    X_test  = np.load(paths["X_test"])
    y_train = np.load(paths["y_train"])
    y_test  = np.load(paths["y_test"])

    total        = len(X_train) + len(X_test)
    total_nm     = int(y_train.sum()) + int(y_test.sum())
    total_safe   = total - total_nm
    n_test       = len(X_test)
    nm_test      = int(y_test.sum())
    safe_test    = n_test - nm_test
    ratio        = safe_test / nm_test if nm_test > 0 else 0
    duration_s   = total / 60.0          # ~60 Hz logging rate
    duration_min = duration_s / 60.0

    return {
        "n_test":       n_test,
        "nm_test":      nm_test,
        "safe_test":    safe_test,
        "ratio":        ratio,
        "features":     X_train.shape[1],
        "total":        total,
        "total_nm":     total_nm,
        "total_safe":   total_safe,
        "duration_s":   duration_s,
        "duration_min": duration_min,
    }


# ── table writers ──────────────────────────────────────────────────────────

def table_71(f, ds):
    f.write("Table 7.1  Dataset Overview\n")
    f.write("-" * 50 + "\n")
    rows = [
        ("Total test samples",              f"{ds['n_test']:,}"),
        ("Near-miss samples (test)",        f"{ds['nm_test']:,}"),
        ("Safe samples (test)",             f"{ds['safe_test']:,}"),
        ("Class ratio  safe : near-miss",   f"{ds['ratio']:.1f} : 1"),
        ("Features per sample",             f"{ds['features']}  (16 ToF + 9 IMU)"),
        ("Ground-truth source",             "flag_manual  (button press by rider)"),
        ("Total samples  (train + test)",   f"{ds['total']:,}"),
        ("Total near-miss events",          f"{ds['total_nm']:,}"),
        ("Approximate dataset duration",    f"{ds['duration_s']:.0f} s  ({ds['duration_min']:.1f} min)"),
        ("Train / test split",              "80 / 20"),
    ]
    for label, value in rows:
        f.write(f"  {label:<40s}  {value}\n")
    f.write("\n")


def table_72(f, metrics_df):
    f.write("Table 7.2  Detection Performance — Deployed Modes\n")
    f.write("-" * 80 + "\n")
    header = f"  {'Mode':<12s}  {'Precision':>10s}  {'Recall':>8s}  {'F1':>8s}  "
    header += f"{'Specificity':>12s}  {'FPR':>8s}  {'Accuracy':>10s}  {'AUC-ROC':>8s}"
    f.write(header + "\n")
    f.write("  " + "-" * 76 + "\n")
    for _, row in metrics_df.iterrows():
        auc = f"{row['auc_roc']:8.4f}" if pd.notna(row.get("auc_roc")) else "     N/A"
        line = (f"  {row['mode']:<12s}  {row['precision']:10.4f}  {row['recall']:8.4f}"
                f"  {row['f1_score']:8.4f}  {row['specificity']:12.4f}"
                f"  {row['fpr']:8.4f}  {row['accuracy']:10.4f}  {auc}")
        f.write(line + "\n")
    f.write("\n")
    f.write("  Confusion matrices  (TP / TN / FP / FN)\n")
    for _, row in metrics_df.iterrows():
        f.write(f"    {row['mode']:<12s}  TP={int(row['tp']):5d}  TN={int(row['tn']):5d}"
                f"  FP={int(row['fp']):5d}  FN={int(row['fn']):5d}\n")
    f.write("\n")


def table_73(f, metrics_df):
    f.write("Table 7.3  Statistical Comparison — Deployed Modes\n")
    f.write("-" * 70 + "\n")

    from scipy.stats import chi2 as chi2_dist

    y_true = None
    pred_path = os.path.join(PROC, "predictions.csv")
    if os.path.exists(pred_path):
        preds = pd.read_csv(pred_path)
        y_true    = preds["y_true"].values
        pred_algo = preds["pred_algo"].values
        pred_ml   = preds["pred_ml"].values
        pred_hyb  = preds["pred_hybrid"].values

        def mcnemar(pa, pb):
            a_right = (pa == y_true)
            b_right = (pb == y_true)
            b_cnt = int((a_right & ~b_right).sum())
            c_cnt = int((~a_right & b_right).sum())
            if b_cnt + c_cnt == 0:
                return 0.0, 1.0
            stat = (abs(b_cnt - c_cnt) - 1) ** 2 / (b_cnt + c_cnt)
            p    = 1 - chi2_dist.cdf(stat, df=1)
            return stat, p

        alpha = 0.05 / 3
        comparisons = [
            ("Algorithm vs ML",     pred_algo, pred_ml),
            ("Algorithm vs Hybrid", pred_algo, pred_hyb),
            ("ML vs Hybrid",        pred_ml,   pred_hyb),
        ]
        f.write(f"  {'Comparison':<25s}  {'Chi2':>10s}  {'p-value':>10s}  {'Significant':>12s}\n")
        f.write("  " + "-" * 62 + "\n")
        for name, pa, pb in comparisons:
            chi2, p = mcnemar(pa, pb)
            sig = "Yes" if p < alpha else "No"
            f.write(f"  {name:<25s}  {chi2:10.4f}  {p:10.6f}  {sig:>12s}\n")
        f.write(f"\n  Bonferroni-corrected alpha: {alpha:.4f}  (0.05 / 3 comparisons)\n")

        # Bootstrap CI
        from sklearn.metrics import f1_score
        f.write("\n  F1-Score  95% Bootstrap Confidence Intervals  (n=1000 resamples)\n")
        f.write(f"  {'Mode':<12s}  {'F1':>8s}  {'95% CI':>20s}\n")
        f.write("  " + "-" * 44 + "\n")
        rng = np.random.RandomState(42)
        for name, pred in [("Algorithm", pred_algo), ("ML", pred_ml), ("Hybrid", pred_hyb)]:
            scores = []
            for _ in range(1000):
                idx = rng.randint(0, len(y_true), len(y_true))
                yt, yp = y_true[idx], pred[idx]
                if len(np.unique(yt)) < 2:
                    continue
                scores.append(f1_score(yt, yp, zero_division=0))
            lo, hi = np.percentile(scores, [2.5, 97.5])
            mean   = np.mean(scores)
            f.write(f"  {name:<12s}  {mean:8.4f}  [{lo:.4f}, {hi:.4f}]\n")
    else:
        f.write("  predictions.csv not found — run evaluate_results.py to generate\n")
    f.write("\n")


def table_74(f):
    f.write("Table 7.4  Embedded Performance — Deployed Single-Frame Model\n")
    f.write("-" * 60 + "\n")
    rows = [
        ("Model architecture",          "25 -> 32 -> 16 -> 2  (fully connected)"),
        ("Number of parameters",        "1,394"),
        ("Model size (TFLite)",         "7.9 KB  (7,900 bytes)"),
        ("Tensor arena",                "12 KB"),
        ("Total ML RAM",                "~14 KB  (arena + TFLite overhead)"),
        ("Inference time (ESP-NN)",     "5–15 ms  (estimated; ESP-NN SIMD)"),
        ("Target frame rate",           "60 Hz  (16.7 ms per frame budget)"),
        ("CPU budget interpretation",   "30–90% of frame budget for ML"),
        ("Detection threshold",         "P(near-miss) > 0.70"),
        ("Flash usage (firmware)",      "~1.27 MB / 1.5 MB partition  (17% free)"),
        ("ESP-NN acceleration",         "FullyConnected + ReLU  (SIMD vectorised)"),
    ]
    for label, value in rows:
        f.write(f"  {label:<35s}  {value}\n")
    f.write("\n")


def table_75(f, metrics_df, sw):
    f.write("Table 7.5  Single-Frame MLP (deployed) vs Sliding-Window 1D-CNN (offline)\n")
    f.write("-" * 80 + "\n")

    sf_row = metrics_df[metrics_df["mode"] == "ML"].iloc[0]

    # Architecture comparison
    f.write("  Architecture\n")
    arch_rows = [
        ("Input shape",         "(25,)  — single frame",          "(10, 25)  — 10-frame window"),
        ("Model type",          "Fully-connected MLP",             "1D-CNN"),
        ("Parameters",          "1,394",                           "4,290"),
        ("Model size (TFLite)", "7.9 KB",                          "~20 KB  (estimated)"),
        ("Temporal context",    "None  (single snapshot)",         "~0.167 s  (10 frames at ~60 Hz)"),
        ("Deployed on ESP32",   "Yes",                             "No  (future work)"),
    ]
    f.write(f"  {'Property':<28s}  {'Single-Frame MLP':>22s}  {'Sliding Window CNN':>22s}\n")
    f.write("  " + "-" * 76 + "\n")
    for label, sf_val, sw_val in arch_rows:
        f.write(f"  {label:<28s}  {sf_val:>22s}  {sw_val:>22s}\n")

    f.write("\n  Performance\n")
    f.write(f"  {'Metric':<14s}  {'Single-Frame MLP':>18s}  {'Sliding Window CNN':>20s}  {'Delta':>8s}\n")
    f.write("  " + "-" * 64 + "\n")
    perf_rows = [
        ("Precision",   float(sf_row["precision"]),  sw["precision"]),
        ("Recall",      float(sf_row["recall"]),     sw["recall"]),
        ("F1-Score",    float(sf_row["f1_score"]),   sw["f1"]),
        ("Accuracy",    float(sf_row["accuracy"]),   sw["accuracy"]),
        ("AUC-ROC",     float(sf_row["auc_roc"]),    sw["auc_roc"]),
        ("FPR",         float(sf_row["fpr"]),         sw["fpr"]),
    ]
    for label, sf_val, sw_val in perf_rows:
        delta = sw_val - sf_val
        sign  = "+" if delta >= 0 else ""
        f.write(f"  {label:<14s}  {sf_val:>18.4f}  {sw_val:>20.4f}  {sign+f'{delta:.4f}':>8s}\n")

    ci_lo = sw.get("ci_lo", 0)
    ci_hi = sw.get("ci_hi", 0)
    f.write(f"\n  Sliding Window F1  95% CI: [{ci_lo:.4f}, {ci_hi:.4f}]\n")
    f.write(f"  Confusion matrix (SW): TP={sw.get('tp',0)}  TN={sw.get('tn',0)}"
            f"  FP={sw.get('fp',0)}  FN={sw.get('fn',0)}\n")
    f.write("\n  Note: Sliding-window model trained and evaluated offline only.\n")
    f.write("        Firmware deployment is identified as future work.\n")
    f.write("        See docs/sliding-window-comparative-study.md\n")
    f.write("\n")


# ── main ───────────────────────────────────────────────────────────────────

def main():
    print("\n" + "="*60)
    print("THESIS CHAPTER 7 — TABLE GENERATOR")
    print("="*60)

    print("Loading data...")
    ds         = load_dataset_info()
    metrics_df = load_sf_metrics()
    sw         = load_sw_metrics()

    os.makedirs(PROC, exist_ok=True)
    with open(OUT, "w", encoding="utf-8") as f:
        f.write("=" * 70 + "\n")
        f.write("PROXITOF — THESIS CHAPTER 7 TABLES\n")
        f.write("Generated by tools/generate_thesis_tables.py\n")
        f.write("=" * 70 + "\n\n")

        table_71(f, ds)
        table_72(f, metrics_df)
        table_73(f, metrics_df)
        table_74(f)
        table_75(f, metrics_df, sw)

        f.write("=" * 70 + "\n")
        f.write("END\n")
        f.write("=" * 70 + "\n")

    print(f"\nSaved: {OUT}")
    print("\nPreview:\n")
    with open(OUT, encoding="utf-8") as f:
        print(f.read())


if __name__ == "__main__":
    main()
