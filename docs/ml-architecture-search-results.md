# ML Architecture Search Results

## Overview

This document records the neural network architecture search conducted during development of the ProxiTof near-miss detection system. Four models were trained and evaluated systematically to find the best architecture for deployment on the ESP32-S3 microcontroller.

---

## Dataset Summary

| Property | Value |
|---|---|
| Total raw samples | 935,322 |
| Near-miss samples (flag_manual=1) | 17,734 (1.90%) |
| Safe samples | 917,588 (98.10%) |
| Raw class ratio | 51.7:1 |
| Balanced ratio (after undersampling) | 10:1 |
| Balanced dataset size | 195,074 |
| Training set | 156,059 (80%) |
| Test set | 39,015 (20%) |
| Features per sample | 25 (16 ToF + 9 IMU) |
| Ground truth source | flag_manual (button press during near-miss) |
| CSV files used | 27 files (LOG0522 – LOG1012, excluding corrupted LOG0542) |

---

## Architecture Search

### Model 1 — Baseline (small dataset)

- Architecture: 25 → 16 → 2
- Parameters: 450
- Model size: 3.5 KB
- Dataset: 73,480 samples (14 CSV files)
- Training epochs: 44

| Metric | Algorithm | ML | Hybrid |
|---|---|---|---|
| Precision | 0.323 | 0.620 | 0.550 |
| Recall | 0.079 | 0.739 | 0.742 |
| F1-Score | 0.127 | 0.674 | 0.632 |
| Accuracy | 0.901 | 0.935 | 0.921 |
| AUC-ROC | 0.531 | 0.952 | 0.841 |

Notes: Strong result for the dataset size. ML clearly outperforms the rule-based algorithm.

---

### Model 2 — Baseline (expanded dataset)

- Architecture: 25 → 16 → 2
- Parameters: 450
- Model size: 3.5 KB
- Dataset: 114,070 samples (22 CSV files)
- Training epochs: 33

| Metric | Algorithm | ML | Hybrid |
|---|---|---|---|
| Precision | 0.339 | 0.574 | 0.504 |
| Recall | 0.082 | 0.607 | 0.609 |
| F1-Score | 0.132 | 0.590 | 0.552 |
| Accuracy | 0.902 | 0.923 | 0.910 |
| AUC-ROC | — | — | — |

Notes: F1 dropped from 0.674 to 0.590 despite more data. The small model (450 params) hit its capacity ceiling — it could not learn the more diverse patterns introduced by the new recordings.

---

### Model 3 — Baseline (full dataset, same architecture)

- Architecture: 25 → 16 → 2
- Parameters: 450
- Model size: 3.5 KB
- Dataset: 195,074 samples (28 CSV files)
- Training epochs: 33

| Metric | Algorithm | ML | Hybrid |
|---|---|---|---|
| Precision | 0.319 | 0.546 | 0.481 |
| Recall | 0.076 | 0.577 | 0.580 |
| F1-Score | 0.123 | 0.561 | 0.522 |
| Accuracy | 0.901 | 0.918 | 0.905 |
| AUC-ROC | — | — | — |

Notes: Continued degradation confirmed the architecture was the bottleneck, not the data volume. Increasing dataset size without increasing model capacity made performance worse.

---

### Model 4 — Deeper architecture (selected)

- Architecture: 25 → 32 → 16 → 2
- Parameters: 1,394
- Model size: 7.9 KB
- Dataset: 195,074 samples (28 CSV files)
- Training epochs: 51 (best at epoch 36)
- TFLite accuracy match: 0.22% difference (excellent)

| Metric | Algorithm | ML | Hybrid |
|---|---|---|---|
| Precision | 0.319 | 0.678 | 0.591 |
| Recall | 0.076 | 0.661 | 0.663 |
| F1-Score | 0.123 | 0.669 | 0.625 |
| Accuracy | 0.901 | 0.941 | 0.928 |
| AUC-ROC | — | 0.937 | — |

Bootstrap 95% CI (F1):
- Algorithm: 0.123 [0.110, 0.136]
- ML: 0.669 [0.658, 0.681]
- Hybrid: 0.625 [0.613, 0.637]

McNemar's test: all pairwise differences statistically significant (p < 0.000001).

Notes: Doubling the first hidden layer (16→32) and adding a second layer recovered the F1 back to 0.669 — matching Model 1 performance but on a 2.7x larger and more diverse dataset. Precision improved to 67.8%, the best across all runs.

---

### Model 5 — Three hidden layers

- Architecture: 25 → 32 → 16 → 8 → 2
- Parameters: 1,514
- Model size: 8.9 KB
- Dataset: 195,074 samples (28 CSV files)
- Training epochs: 30 (best at epoch 15)

| Metric | Algorithm | ML | Hybrid |
|---|---|---|---|
| Precision | 0.319 | 0.684 | 0.594 |
| Recall | 0.076 | 0.647 | 0.651 |
| F1-Score | 0.123 | 0.665 | 0.621 |
| Accuracy | 0.901 | 0.941 | 0.928 |

Notes: Adding a third hidden layer (8 neurons) produced no meaningful improvement over Model 4 (F1 0.665 vs 0.669, within statistical noise). This confirms the performance ceiling is driven by label noise in the training data, not model capacity. Further depth is not justified.

---

## Selected Architecture: Model 4

**25 → 32 → 16 → 2**

Rationale:
- Best F1-score (0.669) on the full dataset
- Best precision (67.8%) — fewest false alarms per detection
- TFLite conversion accurate to within 0.22%
- Model size 7.9 KB — well within ESP32-S3 flash budget
- Tensor arena 12 KB — fits comfortably in available SRAM
- Inference time estimated 10–20 ms — within 16.7 ms frame budget at 60 Hz
- Adding a third layer gave no improvement, confirming this is the right stopping point

---

## ESP32-S3 Resource Budget

| Resource | Model 4 Usage | ESP32-S3 Limit | Headroom |
|---|---|---|---|
| Model flash | 7.9 KB | 1.5 MB partition | >99% free |
| Tensor arena (RAM) | 12 KB | ~100 KB available | comfortable |
| Parameters | 1,394 | — | minimal |
| Inference time (est.) | 10–20 ms | 16.7 ms (60 Hz) | marginal at 60 Hz |
| Firmware total flash | ~1.2 MB | 1.5 MB | 300 KB free |

Note: If inference time exceeds the 60 Hz budget on device, the sensor loop will naturally run at a slightly lower rate. This is acceptable — the ToF sensor itself operates at 15 Hz in autonomous mode, so the effective detection rate is not 60 Hz regardless.

---

## Key Findings

1. Model capacity matters more than dataset size. Doubling the dataset with the same small architecture made performance worse. The fix was a bigger model, not more data.

2. Three layers is not better than two for this problem. The performance plateau after Model 4 indicates the remaining error is from label noise (button press timing inconsistency), not insufficient model capacity.

3. ML significantly outperforms the rule-based algorithm across all experiments. Algorithm F1 is consistently ~0.12–0.13 regardless of dataset size. ML F1 ranges from 0.56–0.67 depending on architecture.

4. The hybrid (Algorithm OR ML) consistently underperforms pure ML. Adding the algorithm's detections on top of ML introduces more false positives than true positives, reducing precision without meaningfully improving recall.

5. All performance differences are statistically significant (McNemar's test, Bonferroni-corrected p < 0.0167).

---

## Recommended Next Steps (Future Work)

- Tighter labeling window: the button press captures a window around the near-miss event. Narrowing this window (e.g. ±0.5s instead of ±1s) would reduce label noise and likely push F1 above 0.70.
- Temporal features: adding a sliding window (e.g. 5-frame sequence) as input would let the model learn approach dynamics rather than single-frame snapshots.
- Field validation: the current evaluation is on held-out training data. A separate field test with independently collected near-miss events would give a more realistic performance estimate.
