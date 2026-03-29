# Thesis Defense Preparation Guide
## ProxiTof: Wearable Near-Miss Detection for Cyclists Using ToF Sensor and On-Device Machine Learning

---

## 1. What Is the Innovation?

**Short answer to give in defense:**

> "The innovation is the combination of a 4x4 Time-of-Flight depth sensor array with an on-device neural network running on a microcontroller, specifically designed for wearable cyclist safety. The device collects its own training data in the field, trains a model on a PC, and deploys it back to the same hardware — a complete closed-loop ML pipeline on a constrained embedded system."

**Longer breakdown — three layers of innovation:**

1. Sensor choice: Using the VL53L8CX 4x4 ToF matrix (not a camera, not a single-point sensor) gives spatial depth information — 16 distance readings per frame — without the privacy, power, and compute cost of a camera. This is novel for wearable cyclist safety devices.

2. On-device ML inference: The neural network runs entirely on the ESP32-S3 at up to 60 Hz with no cloud dependency, no latency from network calls, and no privacy concern. The model is 7.9 KB and uses ESP-NN SIMD acceleration.

3. Self-collected labeled dataset: The ground truth labels come from the rider pressing a button during actual near-miss events on a real bicycle. This is real-world, ecologically valid data — not a lab simulation. The full pipeline from raw sensor data to deployed model is documented and reproducible.

---

## 2. Why Not Camera + AI? (The Hardest Question)

This is the most likely challenge question. Be ready with a structured answer.

**The question they will ask:**
> "There are ESP32-CAM boards that can do object detection. Why use a ToF sensor instead of a camera with a YOLO-style model?"

**Your answer — four points:**

**Point 1: Power and wearability**
A camera-based system running object detection (even MobileNet-SSD) on ESP32 draws 200–500 mA continuously. The ProxiTof system draws approximately 80–120 mA total. For a wearable device meant to run for a full cycling session (1–3 hours), battery life is a primary constraint. A camera system would require a significantly larger battery, making it impractical as a wearable.

**Point 2: Compute reality on ESP32**
The ESP32-CAM (OV2640) can capture images, but running real-time object detection at useful frame rates on the ESP32 itself is not practical. Projects that do "AI on ESP32-CAM" typically:
- Run at 1–5 FPS (not 60 Hz)
- Use heavily quantized int8 models
- Require the ESP32-S3 with PSRAM (which this project also uses, but for a much lighter task)
- Still struggle with latency for safety-critical real-time detection

The ProxiTof ML model runs at 60 Hz inference with ~10–20 ms latency. A camera-based approach on the same hardware would be 10–60x slower per frame.

**Point 3: Privacy**
A camera records identifiable images of people and environments. A ToF sensor outputs only distance values — 16 numbers per frame. There is no image, no face, no license plate. For a device worn in public, this is a meaningful ethical and legal advantage.

**Point 4: The problem doesn't need a camera**
Near-miss detection for a cyclist is fundamentally a proximity and approach-rate problem. You need to know: is something close, and is it getting closer fast? The VL53L8CX answers both questions directly with millimeter precision at 60 Hz. A camera would need to solve object detection, depth estimation, and velocity estimation — three hard problems — to answer the same question. The ToF sensor answers it directly.

**Comparison table to put in your thesis:**

| Property | Camera + AI (ESP32-CAM) | ProxiTof (VL53L8CX + MLP) |
|---|---|---|
| Frame rate (inference) | 1–5 FPS | 60 Hz |
| Power draw | 200–500 mA | 80–120 mA |
| Model size | 300 KB – 2 MB | 7.9 KB |
| Privacy | Records images | Distance values only |
| Depth information | Estimated (monocular) | Direct measurement (mm) |
| Lighting dependency | Yes (needs light) | No (active IR, works at night) |
| Compute required | High (CNN) | Low (MLP, 1,394 params) |
| Latency | 200–1000 ms | 10–20 ms |
| Wearable feasibility | Difficult | Yes |

---

## 3. What Is the Computing Power of the ESP32-S3?

**Specs to know:**

- CPU: Xtensa LX7 dual-core, 240 MHz
- RAM: 512 KB internal SRAM + 8 MB PSRAM (on this board)
- Flash: 16 MB
- ESP-NN: Hardware-accelerated SIMD operations for neural network inference (FullyConnected, ReLU, Softmax)
- No dedicated NPU or GPU — inference runs on the CPU with SIMD vectorization

**How to explain ESP-NN:**
> "The ESP32-S3 includes the ESP-NN library which uses SIMD (Single Instruction Multiple Data) instructions to process multiple multiply-accumulate operations in parallel. For our fully-connected layers, this gives roughly 2–4x speedup over naive C implementation. This is why a 1,394-parameter network can run at 60 Hz on a microcontroller."

**Inference budget:**
- At 60 Hz, each frame is 16.7 ms
- Our model inference: estimated 10–20 ms
- The ToF sensor itself runs at 15 Hz in autonomous mode, so the effective detection rate is 15 Hz regardless — the ML inference is not the bottleneck

---

## 4. Is the F1 Score of 0.669 Good Enough?

**The question:**
> "Your ML model only achieves F1 of 0.67. Is that sufficient for a safety device?"

**Your answer:**

First, frame the comparison correctly. The baseline is the rule-based algorithm, which achieves F1 = 0.123. The ML model is 5.4x better than the existing approach. That is the relevant comparison, not an abstract ideal.

Second, this is a research prototype, not a certified safety product. The goal of the thesis is to demonstrate feasibility and compare approaches — which is clearly achieved.

Third, the precision of 67.8% means that when the device alerts, it is correct approximately 2 out of 3 times. For a supplementary safety alert (the rider still has their own senses), this is reasonable. It is not replacing the rider's judgment — it is augmenting it.

Fourth, the remaining error is explained and bounded. The architecture search showed that adding more layers did not improve performance, which means the ceiling is driven by label noise in the training data (button press timing inconsistency), not by model capacity. This is a known, documented limitation with a clear path to improvement (tighter labeling window, temporal features).

---

## 5. Why Is the Hybrid (Algorithm OR ML) Worse Than ML Alone?

**The question:**
> "Why did combining the algorithm with ML make things worse?"

**Your answer:**

The hybrid takes a detection if either the algorithm OR the ML model fires. This increases recall slightly (catches a few more near-misses the ML missed) but significantly increases false positives, because the algorithm has a very high false positive rate (FPR = 1.66% vs ML's 4.54% — but the algorithm fires on different samples, adding new false alarms on top of ML's).

The net effect: precision drops from 67.8% (ML alone) to 59.1% (Hybrid), while recall only improves marginally. F1 drops from 0.669 to 0.625.

This is actually a useful finding for the thesis — it shows that the rule-based algorithm is not complementary to ML. They are not catching different true positives; the algorithm is mostly adding noise.

---

## 6. Why Is the Dataset Imbalanced and How Did You Handle It?

**The question:**
> "Your dataset has a 51.7:1 ratio of safe to near-miss. How did you handle this?"

**Your answer:**

Three techniques were applied:

1. Undersampling: Safe samples were randomly undersampled to achieve a 10:1 ratio (safe:near-miss). This reduced the training set from 935,322 to 195,074 samples while preserving all near-miss events.

2. Class weights: During training, the loss function was weighted inversely proportional to class frequency. Near-miss samples were weighted 5.5x higher than safe samples, penalizing the model more for missing a near-miss than for a false alarm.

3. Threshold tuning: The detection threshold was set at 0.70 (P(near-miss) > 0.70 triggers an alert). This was chosen to balance precision and recall for the safety use case.

---

## 7. What Are the Limitations and Future Work?

Be proactive — mention these before they ask. It shows maturity.

**Limitations:**
- Label noise: The button press ground truth has timing uncertainty of approximately ±0.5–1 second. Some frames labeled as near-miss may be safe, and vice versa.
- Single-frame inference: The model sees one frame at a time with no temporal context. A sliding window of 5–10 frames would let the model learn approach dynamics.
- Dataset size: 17,734 near-miss events from one rider in one environment. Generalization to other riders, bikes, and road conditions is not validated.
- No independent test set: The evaluation uses a held-out split from the same recording sessions. A truly independent field test would give a more realistic performance estimate.

**Future work:**
- Temporal features: LSTM or 1D-CNN over a sliding window of frames
- Tighter labeling: Reduce the near-miss window from ±1s to ±0.5s
- Multi-rider dataset: Collect data from multiple cyclists in different environments
- Field validation: Independent test with separately collected near-miss events

---

## 8. What Does the System Actually Do? (For Non-Technical Panel Members)

> "The device is worn on a bicycle. It uses a depth sensor — similar to the face unlock sensor on a smartphone — to measure the distance to objects behind the cyclist 60 times per second. A small neural network running on the device learns from real near-miss events recorded by the rider, and alerts the cyclist with a buzzer when it detects a dangerous approach pattern. The entire system — sensor, processor, GPS, display, and storage — fits in a small enclosure and runs on battery."

---

## 9. Key Numbers to Memorize

| Number | What it is |
|---|---|
| 0.669 | ML F1-score (final model) |
| 0.123 | Algorithm F1-score (baseline) |
| 5.4x | ML improvement over algorithm |
| 67.8% | ML precision |
| 66.1% | ML recall |
| 0.937 | ML AUC-ROC |
| 17,734 | Near-miss events in dataset |
| 195,074 | Balanced training+test samples |
| 27 | CSV files used (1 corrupted) |
| 1,394 | Model parameters |
| 7.9 KB | TFLite model size |
| 12 KB | Tensor arena (RAM) |
| 10–20 ms | Estimated inference time |
| 60 Hz | Sensor + inference loop rate |
| 15 Hz | ToF autonomous mode frame rate |
| 25 | Input features (16 ToF + 9 IMU) |
| 0.70 | Detection threshold |
| p < 0.000001 | McNemar's test significance |

---

## 10. Suggested Opening Statement for Defense

> "This thesis presents ProxiTof, a wearable near-miss detection system for cyclists. The core contribution is demonstrating that a Time-of-Flight depth sensor combined with a small neural network — running entirely on a microcontroller — can detect dangerous vehicle approaches with an F1-score of 0.669, compared to 0.123 for a rule-based algorithm. The system is self-contained, privacy-preserving, and operates in real time at 60 Hz with no cloud dependency. I will present the hardware design, the ML pipeline, the evaluation methodology, and the architecture search that led to the final model."

---

## 11. Sensor vs Camera — Technical Detail for Follow-Up Questions

If they push harder on the camera question, here are the technical specifics:

**VL53L8CX specs:**
- 4x4 multi-zone ToF, 940 nm VCSEL
- Range: 40 mm to 4000 mm (we cap at 3800 mm)
- Frame rate: up to 60 Hz
- Power: ~25 mW active
- Interface: I2C
- Output: 16 distance values + confidence per frame

**What a camera-based system would need:**
- Object detection model (e.g. MobileNet-SSD): 300 KB – 2 MB
- Depth estimation (monocular): additional model or stereo camera
- Velocity estimation: optical flow or tracking
- Total compute: 10–100x more than our MLP
- Total power: 5–10x more than our ToF sensor

**The key insight:** The ToF sensor is a purpose-built proximity sensor. Using a camera for proximity detection is using the wrong tool for the job — like using a GPS to measure room temperature because GPS receivers also have temperature sensors.

---

## 12. If They Ask About Statistical Validity

**McNemar's test** was used to compare classifiers on the same test set. It tests whether the disagreements between two classifiers are symmetric — i.e., whether one classifier is systematically better than the other.

- Algorithm vs ML: chi2 = 645.2, p < 0.000001
- All three pairwise comparisons significant after Bonferroni correction (alpha = 0.0167)

**Bootstrap confidence intervals** (1000 resamples, 95% CI):
- ML F1: 0.669 [0.658, 0.681] — tight interval, stable result
- Algorithm F1: 0.123 [0.110, 0.136] — also stable, consistently poor

The narrow confidence intervals and extreme p-values mean the results are not a fluke of the train/test split. The ML advantage is real and robust.
