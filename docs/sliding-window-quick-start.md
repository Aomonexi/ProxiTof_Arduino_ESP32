# Sliding Window Model — Quick Start Guide

This is an OFFLINE comparative experiment. The deployed firmware on the ESP32-S3
continues to use the single-frame MLP. These scripts exist to evaluate whether
temporal context improves detection, and to provide thesis results.

---

## About TF_ENABLE_ONEDNN_OPTS

No, you do not need to set it every time. It only suppresses a TensorFlow
performance warning about Intel oneDNN optimizations. The scripts work fine
without it. If you want to suppress the warning permanently, add this to your
Windows environment variables:

  Variable name : TF_ENABLE_ONEDNN_OPTS
  Value         : 0

To set it for the current PowerShell session only:
  $env:TF_ENABLE_ONEDNN_OPTS=0

To set it permanently via PowerShell (run once):
  [System.Environment]::SetEnvironmentVariable("TF_ENABLE_ONEDNN_OPTS","0","User")

---

## Step 1 — Activate the virtual environment

Open PowerShell in the project root and run:

  venv_ml\Scripts\activate

You should see (venv_ml) in your prompt. You only need to do this once per
terminal session.

---

## Step 2 — Train the sliding window model

  cd tools
  python train_sliding_window.py

What it does:
- Loads all CSV files from data/raw/
- Extracts overlapping 10-frame windows (step=1, ~0.167s per window at ~60Hz recorded rate)
- Labels a window as near-miss if ANY frame in it has flag_manual=1
- Normalises, balances (10:1 safe:near-miss), splits 80/20
- Trains a 1D-CNN: Conv1D(32) -> Conv1D(16) -> GlobalAvgPool -> Dense(16) -> 2
- Saves model to models/sw_model.keras (does NOT touch near_miss_model.keras)
- Saves test data to data/processed/sw_X_test.npy and sw_y_test.npy

Expected runtime: 20-40 minutes depending on hardware.
Expected result: val_accuracy ~95-97%

---

## Step 3 — Evaluate the sliding window model

  python evaluate_sliding_window.py

What it does:
- Loads models/sw_model.keras
- Loads data/processed/sw_X_test.npy and sw_y_test.npy
- Loads data/processed/performance_metrics.csv (single-frame results)
- Computes precision, recall, F1, accuracy, AUC-ROC for the sliding window model
- Compares side-by-side with the single-frame MLP
- Saves outputs to data/processed/ (all prefixed sw_)

Outputs:
  data/processed/sw_evaluation_report.txt       <- copy into thesis
  data/processed/sw_vs_singleframe_comparison.png  <- comparison bar chart
  data/processed/sw_confusion_matrix.png        <- confusion matrix

---

## Step 4 — Run the full comparison (optional)

If you want a combined report of all three approaches (Algorithm, Single-Frame ML,
Sliding Window ML), run the standard evaluation first to make sure
performance_metrics.csv is up to date:

  python evaluate_results.py

Then run the sliding window evaluation:

  python evaluate_sliding_window.py

The sw_evaluation_report.txt will include the delta vs single-frame automatically.

---

## File Summary

| File | Purpose |
|---|---|
| tools/train_sliding_window.py | Train the 1D-CNN on 10-frame windows |
| tools/evaluate_sliding_window.py | Evaluate and compare with single-frame |
| tools/evaluate_results.py | Evaluate single-frame model (Algorithm/ML/Hybrid) |
| models/sw_model.keras | Trained sliding window model (offline only) |
| models/sw_best.keras | Best checkpoint during training |
| models/near_miss_model.keras | Deployed single-frame model (do not overwrite) |
| data/processed/sw_X_test.npy | Sliding window test features |
| data/processed/sw_y_test.npy | Sliding window test labels |
| data/processed/sw_evaluation_report.txt | Evaluation results |
| data/processed/sw_vs_singleframe_comparison.png | Bar chart comparison |
| data/processed/sw_confusion_matrix.png | Confusion matrix |

---

## Key Differences from Single-Frame Pipeline

| Property | Single-Frame MLP | Sliding Window CNN |
|---|---|---|
| Input shape | (25,) | (10, 25) |
| Model type | Dense MLP | 1D-CNN |
| Parameters | 1,394 | 4,290 |
| Temporal context | None | 10 frames (~0.67s) |
| Deployed on ESP32 | Yes | No (future work) |
| Training script | quick_start_ml.py | train_sliding_window.py |
| Eval script | evaluate_results.py | evaluate_sliding_window.py |
| Model file | near_miss_model.keras | sw_model.keras |
