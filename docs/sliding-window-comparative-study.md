# Sliding Window Comparative Study
## Temporal Context for Near-Miss Detection — Offline Experiment

---

## Purpose of This Document

The ProxiTof device currently runs a single-frame MLP (25 → 32 → 16 → 2) on the ESP32-S3. This document explains:

1. Why temporal context (sliding window) is theoretically better
2. Why we tested it as an offline experiment rather than deploying it
3. How to interpret the results in the thesis
4. What the results mean for future work

---

## What Is the Difference?

### Single-Frame MLP (deployed on device)

The model sees one snapshot: 25 numbers from a single moment in time.

```
Frame at time T:
  [tof_0_0 ... tof_3_3, roll, pitch, yaw, ax, ay, az, gx, gy, gz]
       -> MLP (25 -> 32 -> 16 -> 2) -> P(near-miss)
```

The model has no memory. It cannot tell whether an object is approaching or retreating. Two frames with identical distances — one where a car is approaching at 100 km/h, one where a car is parked — look identical to this model.

### Sliding Window 1D-CNN (offline experiment)

The model sees 10 consecutive frames (~0.167 seconds at ~60 Hz recorded rate).

```
Frames T-9 to T:
  [[tof..., imu...],   <- 0.67s ago
   [tof..., imu...],
   ...
   [tof..., imu...]]   <- now
       -> 1D-CNN -> P(near-miss)
```

The model can now see distances decreasing over time, approach rate, and the shape of the approach curve. A parked car produces flat distance readings. An approaching car produces a decreasing ramp. These are completely different sequences even if the final frame is identical.

---

## Why Sliding Window Is Theoretically Better

### 1. Near-miss is a temporal event, not a static state

A near-miss is defined by motion — something approaching rapidly. A single frame captures position but not velocity or acceleration. The sliding window captures the full kinematic signature of an approach event.

### 2. The button press timing problem is reduced

The rider presses the button slightly before the near-miss peak (anticipatory press). Individual frames labeled flag_manual=1 include frames where the vehicle is still at a safe distance — they look like normal safe frames to a single-frame model.

With a sliding window, the label applies to the whole sequence. A window containing the button press also contains the approach trajectory. The model learns: "distances decreasing from ~3m to ~0.5m over 10 frames = near-miss." This is a much more distinctive pattern than "distance = 0.5m at one moment."

### 3. Reduces label noise

Because the window captures context around the button press, the ambiguity of individual frame labels is averaged out. Even if 3 out of 10 frames in a near-miss window look like safe frames individually, the sequence as a whole is distinctive.

### 4. IMU temporal patterns become useful

A single IMU reading is noisy and hard to interpret. Over 10 frames, patterns emerge: the rider braking (deceleration ramp), the rider swerving (roll change), the vehicle passing (sudden distance change followed by recovery). These temporal IMU signatures are invisible to the single-frame model.

---

## Why We Did Not Deploy It on the ESP32-S3

### Firmware complexity

The single-frame model requires no state between frames. The sliding window model requires a circular buffer of the last 10 frames maintained in firmware, fed to the model as a 2D input (10x25), and updated every frame. This is a non-trivial firmware change across sensor_manager.cpp and ml_inference.cpp.

### Time constraint

The firmware change was identified late in the project timeline. Implementing, testing, and validating it on-device within the thesis submission window was not feasible without risking stability of the working system.

### The offline experiment is sufficient for the thesis

The research question — "does temporal context improve near-miss detection?" — can be answered with an offline experiment on the same dataset. Deploying it to the device would answer a different question ("does it work in real time on ESP32-S3?"), which is future work.

---

## How to Present This in the Thesis

### In the Methods chapter

> "Two model variants were evaluated: a single-frame MLP deployed on the ESP32-S3, and a sliding window 1D-CNN evaluated offline as a comparative experiment. The single-frame model processes each sensor frame independently (25 features). The sliding window model processes sequences of 10 consecutive frames (10 x 25 = 250 features) using a 1D-CNN architecture. Both models were trained on the same dataset with identical preprocessing and evaluation protocols."

### In the Results chapter

> "Table X presents the performance comparison between the single-frame MLP (deployed) and the sliding window 1D-CNN (offline). The sliding window model achieves [F1 result], compared to [F1 result] for the single-frame model. This result supports the hypothesis that temporal context improves near-miss detection. The sliding window model was not deployed to the ESP32-S3 firmware in this work; firmware implementation is identified as future work."

### In the Discussion / Future Work chapter

> "The offline evaluation of the sliding window 1D-CNN demonstrates that temporal context provides measurable improvement in detection performance. The primary limitation of the current deployed system — that a single frame cannot distinguish an approaching vehicle from a stationary one at the same distance — is directly addressed by the sliding window approach. Future implementation requires a circular frame buffer in the sensor manager, a modified TFLite input tensor, and on-device latency validation."

---

## Technical Specification

### Window parameters

| Parameter | Value | Rationale |
|---|---|---|
| Window size | 10 frames | ~0.167s at ~60 Hz recorded rate — captures full approach |
| Step size | 1 frame | Maximum overlap, maximum training samples |
| Label rule | ANY frame = near-miss -> window = near-miss | Captures approach leading up to event |
| Features | 10 x 25 = 250 per window | Same features as single-frame, stacked |

### Model architecture

```
Input: (10, 25)
  -> Conv1D(32 filters, kernel=3, ReLU)
  -> Dropout(0.2)
  -> Conv1D(16 filters, kernel=3, ReLU)
  -> GlobalAveragePooling1D
  -> Dense(16, ReLU)
  -> Dense(2, Softmax)
  -> P(safe), P(near-miss)
```

### Why 1D-CNN over LSTM?

| Property | 1D-CNN | LSTM |
|---|---|---|
| Training speed | Fast | Slow |
| Fixed window | Natural fit | Overkill |
| ESP32 deployment | Feasible | Difficult (stateful) |
| Performance | Comparable for short windows | Better for long sequences |

---

## Firmware Implementation Plan (Future Work)

For completeness, here is what would be needed to deploy the sliding window model:

**1. Circular buffer in sensor_manager.cpp**

Maintain a ring buffer of the last 10 normalized feature vectors. Push a new frame on every sensor read. When the buffer is full, extract the ordered window and pass it to ml_inference.

**2. Modified ml_inference.cpp**

- Input tensor shape: [1, 10, 25] instead of [1, 25]
- Tensor arena: increase to ~20 KB for Conv1D activations
- Feed the flattened window to the input tensor

**3. New model_data.h**

Generated from sliding_window_model.tflite using the same convert_to_tflite.py pipeline.

**4. Latency validation**

Conv1D on ESP32-S3 with ESP-NN is not as well-optimized as FullyConnected. Inference time needs to be measured on-device to confirm it fits within the 16.7 ms frame budget.
