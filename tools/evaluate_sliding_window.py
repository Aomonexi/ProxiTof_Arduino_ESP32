"""
Sliding Window Evaluation — Compares SW 1D-CNN vs Single-Frame MLP
===================================================================
Loads the trained sliding window model and the existing single-frame
results, then produces a side-by-side comparison.

Outputs (all sw_ prefixed, never overwrites single-frame files):
  data/processed/sw_evaluation_report.txt
  data/processed/sw_confusion_matrix.png
  data/processed/sw_vs_singleframe_comparison.png

Usage:
  cd tools
  ..\venv_ml\Scripts\activate
  python evaluate_sliding_window.py
"""

import os
import sys
import warnings
import numpy as np
import pandas as pd

warnings.filterwarnings("ignore")
os.environ["TF_CPP_MIN_LOG_LEVEL"] = "2"

ROOT   = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
PROC   = os.path.join(ROOT, "data", "processed")
MODELS = os.path.join(ROOT, "models")

THRESH = 0.70
WIN    = 10


def load_sw_model():
    import tensorflow as tf
    candidates = [
        os.path.join(MODELS, "sw_model.keras"),
        os.path.join(MODELS, "sw_best.keras"),
    ]
    for p in candidates:
        if os.path.exists(p):
            print(f"Loading model: {p}")
            return tf.keras.models.load_model(p)
    sys.exit("ERROR: no sliding window model found — run train_sliding_window.py first")


def load_test_data():
    Xte = np.load(os.path.join(PROC, "sw_X_test.npy"))
    yte = np.load(os.path.join(PROC, "sw_y_test.npy"))
    print(f"Test set: {len(Xte):,} windows  near-miss={yte.sum():,}")
    return Xte, yte


def load_sf_metrics():
    """Load single-frame ML row from performance_metrics.csv"""
    path = os.path.join(PROC, "performance_metrics.csv")
    if not os.path.exists(path):
        print("WARNING: performance_metrics.csv not found — run evaluate_results.py first")
        return None
    df  = pd.read_csv(path)
    row = df[df["mode"] == "ML"]
    if row.empty:
        return None
    r = row.iloc[0]
    return {
        "precision": float(r["precision"]),
        "recall":    float(r["recall"]),
        "f1":        float(r["f1_score"]),
        "accuracy":  float(r["accuracy"]),
        "auc_roc":   float(r["auc_roc"]) if r.get("auc_roc") is not None else None,
        "fpr":       float(r["fpr"]),
        "tp": int(r["tp"]), "tn": int(r["tn"]),
        "fp": int(r["fp"]), "fn": int(r["fn"]),
    }


def compute_metrics(model, Xte, yte):
    from sklearn.metrics import (precision_score, recall_score, f1_score,
                                  accuracy_score, confusion_matrix, roc_auc_score)
    probs = model.predict(Xte, verbose=0)
    prob1 = probs[:, 1]
    pred  = (prob1 > THRESH).astype(int)
    tn, fp, fn, tp = confusion_matrix(yte, pred, labels=[0, 1]).ravel()
    return {
        "precision": precision_score(yte, pred, zero_division=0),
        "recall":    recall_score(yte, pred, zero_division=0),
        "f1":        f1_score(yte, pred, zero_division=0),
        "accuracy":  accuracy_score(yte, pred),
        "auc_roc":   roc_auc_score(yte, prob1),
        "fpr":       fp / (fp + tn) if (fp + tn) > 0 else 0,
        "tp": tp, "tn": tn, "fp": fp, "fn": fn,
    }, pred


def bootstrap_ci(yte, pred, n=1000):
    from sklearn.metrics import f1_score
    rng = np.random.RandomState(42)
    scores = []
    for _ in range(n):
        idx = rng.randint(0, len(yte), len(yte))
        yt, yp = yte[idx], pred[idx]
        if len(np.unique(yt)) < 2:
            continue
        scores.append(f1_score(yt, yp, zero_division=0))
    return np.percentile(scores, [2.5, 97.5])


def plot_comparison(sf, sw):
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt

    keys    = ["precision", "recall", "f1", "accuracy"]
    labels  = ["Precision", "Recall", "F1-Score", "Accuracy"]
    x = np.arange(len(keys));  w = 0.35

    fig, ax = plt.subplots(figsize=(9, 5))
    if sf:
        b1 = ax.bar(x - w/2, [sf[k] for k in keys], w,
                    label="Single-Frame MLP (deployed)", color="#4C72B0")
        for b, v in zip(b1, [sf[k] for k in keys]):
            ax.text(b.get_x() + b.get_width()/2, b.get_height() + 0.01,
                    f"{v:.3f}", ha="center", va="bottom", fontsize=9)

    b2 = ax.bar(x + w/2, [sw[k] for k in keys], w,
                label="Sliding Window 1D-CNN (offline)", color="#55A868")
    for b, v in zip(b2, [sw[k] for k in keys]):
        ax.text(b.get_x() + b.get_width()/2, b.get_height() + 0.01,
                f"{v:.3f}", ha="center", va="bottom", fontsize=9)

    ax.set_xticks(x);  ax.set_xticklabels(labels)
    ax.set_ylim(0, 1.15);  ax.set_ylabel("Score")
    ax.set_title("Single-Frame MLP vs Sliding Window 1D-CNN")
    ax.legend();  ax.grid(axis="y", alpha=0.3)
    plt.tight_layout()
    out = os.path.join(PROC, "sw_vs_singleframe_comparison.png")
    plt.savefig(out, dpi=150, bbox_inches="tight");  plt.close()
    print(f"  Saved: {out}")


def plot_confusion(yte, pred):
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    from sklearn.metrics import confusion_matrix

    cm = confusion_matrix(yte, pred, labels=[0, 1])
    fig, ax = plt.subplots(figsize=(5, 4))
    ax.imshow(cm, cmap="Blues")
    ax.set_title("Sliding Window CNN — Confusion Matrix")
    ax.set_xlabel("Predicted");  ax.set_ylabel("Actual")
    ax.set_xticks([0, 1]);  ax.set_yticks([0, 1])
    ax.set_xticklabels(["Safe", "Near-Miss"])
    ax.set_yticklabels(["Safe", "Near-Miss"])
    for i in range(2):
        for j in range(2):
            ax.text(j, i, str(cm[i, j]), ha="center", va="center",
                    fontsize=14, fontweight="bold",
                    color="white" if cm[i, j] > cm.max()/2 else "black")
    plt.tight_layout()
    out = os.path.join(PROC, "sw_confusion_matrix.png")
    plt.savefig(out, dpi=150, bbox_inches="tight");  plt.close()
    print(f"  Saved: {out}")


def save_report(sw, sf, ci_lo, ci_hi, n_test, n_pos):
    out = os.path.join(PROC, "sw_evaluation_report.txt")
    with open(out, "w", encoding="utf-8") as f:
        f.write("=" * 70 + "\n")
        f.write("SLIDING WINDOW 1D-CNN - EVALUATION REPORT\n")
        f.write("Offline Comparative Experiment\n")
        f.write("=" * 70 + "\n\n")
        f.write(f"Window size  : {WIN} frames (~{WIN/60:.3f}s at ~60 Hz recorded rate)\n")
        f.write(f"Model        : 1D-CNN  Conv1D(32) → Conv1D(16) → GAP → Dense(16) → 2\n")
        f.write(f"Threshold    : {THRESH}\n")
        f.write(f"Test windows : {n_test}\n")
        f.write(f"Near-miss    : {n_pos}\n\n")

        f.write("Sliding Window Results\n" + "-"*40 + "\n")
        for m in ["precision", "recall", "f1", "accuracy", "auc_roc", "fpr"]:
            f.write(f"  {m:12s}: {sw[m]:.4f}\n")
        f.write(f"  F1 95% CI  : [{ci_lo:.4f}, {ci_hi:.4f}]\n")
        f.write(f"  TP={sw['tp']}  TN={sw['tn']}  FP={sw['fp']}  FN={sw['fn']}\n\n")

        if sf:
            f.write("Comparison: Single-Frame MLP vs Sliding Window CNN\n")
            f.write("-" * 60 + "\n")
            f.write(f"  {'Metric':12s} {'Single-Frame':>14s} {'Sliding Window':>16s} {'Delta':>8s}\n")
            f.write("-" * 52 + "\n")
            for m, lbl in [("precision","Precision"), ("recall","Recall"),
                            ("f1","F1-Score"), ("accuracy","Accuracy"), ("auc_roc","AUC-ROC")]:
                sv = sf.get(m);  wv = sw.get(m)
                if sv is not None and wv is not None:
                    d = wv - sv;  sign = "+" if d >= 0 else ""
                    f.write(f"  {lbl:12s} {sv:>14.4f} {wv:>16.4f} {sign+f'{d:.4f}':>8s}\n")

        f.write("\n")
        f.write("NOTE: This model is OFFLINE only.\n")
        f.write("      Deployed firmware uses the single-frame MLP.\n")
        f.write("      See: docs/sliding-window-comparative-study.md\n")
        f.write("=" * 70 + "\n")
    print(f"  Saved: {out}")


def main():
    print("\n" + "="*60)
    print("SLIDING WINDOW EVALUATION")
    print("="*60)

    model       = load_sw_model()
    Xte, yte    = load_test_data()
    sw, pred    = compute_metrics(model, Xte, yte)
    sf          = load_sf_metrics()
    ci_lo, ci_hi = bootstrap_ci(yte, pred)

    print("\n" + "="*60)
    print("RESULTS")
    print("="*60)
    print(f"\n{'Metric':20s} {'Single-Frame MLP':>18s} {'Sliding Window CNN':>20s}")
    print("-" * 60)
    for m in ["precision", "recall", "f1", "accuracy", "auc_roc", "fpr"]:
        sv = f"{sf[m]:.4f}" if sf and sf.get(m) is not None else "   N/A"
        print(f"  {m:18s} {sv:>18s} {sw[m]:>20.4f}")

    print(f"\n  TP={sw['tp']}  FP={sw['fp']}  FN={sw['fn']}  TN={sw['tn']}")
    print(f"  Bootstrap 95% CI (F1): {sw['f1']:.4f} [{ci_lo:.4f}, {ci_hi:.4f}]")

    print("\nGenerating plots and report …")
    os.makedirs(PROC, exist_ok=True)
    plot_comparison(sf, sw)
    plot_confusion(yte, pred)
    save_report(sw, sf, ci_lo, ci_hi, len(yte), int(yte.sum()))

    print("\n" + "="*60)
    print("EVALUATION COMPLETE")
    print("="*60)
    print(f"\nSliding Window F1  : {sw['f1']:.4f}  [{ci_lo:.4f}, {ci_hi:.4f}]")
    print(f"Sliding Window AUC : {sw['auc_roc']:.4f}")
    if sf:
        delta = sw["f1"] - sf["f1"]
        sign  = "+" if delta >= 0 else ""
        print(f"vs Single-Frame F1 : {sf['f1']:.4f}  (delta {sign}{delta:.4f})")
    print(f"\nOutputs in data/processed/:")
    print("  sw_evaluation_report.txt")
    print("  sw_vs_singleframe_comparison.png")
    print("  sw_confusion_matrix.png")


if __name__ == "__main__":
    main()
