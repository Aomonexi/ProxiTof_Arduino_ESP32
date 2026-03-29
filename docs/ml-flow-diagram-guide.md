# ML Flow — Diagram Guide

This document describes every flow and connection so you can draw the diagrams.
Two diagrams are described: the Training Flow and the Inference (Deployment) Flow.

---

## Diagram 1: Training Flow (runs on PC)

This is what happens when you run `quick_start_ml.py`.

```
┌─────────────────────────────────────────────────────────────────┐
│                        DATA COLLECTION                          │
│                                                                 │
│   Cyclist rides with device  →  Sensor records 60 rows/second  │
│   Button pressed at near-miss  →  flag_manual = 1              │
│   Saved to SD card as CSV file                                  │
└─────────────────────┬───────────────────────────────────────────┘
                      │
                      ▼
┌─────────────────────────────────────────────────────────────────┐
│                        LOAD & MERGE                             │
│                                                                 │
│   14 CSV files  →  521,824 rows total                          │
│   Each row = 25 sensor values + 1 label (0 or 1)               │
│   6,680 near-miss rows  |  515,144 safe rows                   │
└─────────────────────┬───────────────────────────────────────────┘
                      │
                      ▼
┌─────────────────────────────────────────────────────────────────┐
│                     PREPARE DATA                                │
│                                                                 │
│   Step 1 — Normalise                                            │
│     Each of the 25 features scaled to mean=0, std=1            │
│     (so ToF distances and gyroscope values are on same scale)   │
│                                                                 │
│   Step 2 — Balance                                              │
│     Keep all 6,680 near-miss rows                               │
│     Randomly keep 66,800 safe rows  (10:1 ratio)               │
│     Total: 73,480 rows                                          │
│                                                                 │
│   Step 3 — Split                                                │
│     80% → Training set   (58,784 rows)                         │
│     20% → Test set       (14,696 rows)  ← never used in training│
└─────────────────────┬───────────────────────────────────────────┘
                      │
                      ▼
┌─────────────────────────────────────────────────────────────────┐
│                     NEURAL NETWORK                              │
│                                                                 │
│   INPUT LAYER                                                   │
│   ┌──────────────────────────────────────────────────────┐     │
│   │  x1=tof_00  x2=tof_01  ...  x16=tof_33              │     │
│   │  x17=roll   x18=pitch  x19=yaw                       │     │
│   │  x20=ax     x21=ay     x22=az                        │     │
│   │  x23=gx     x24=gy     x25=gz                        │     │
│   │                                    25 values total    │     │
│   └──────────────────────┬───────────────────────────────┘     │
│                          │  (each input connects to all 16)    │
│                          ▼                                      │
│   HIDDEN LAYER  (16 neurons, each with 25 weights + 1 bias)    │
│   ┌──────────────────────────────────────────────────────┐     │
│   │  Neuron 1:  z = x·W1 + b1  →  ReLU(z)              │     │
│   │  Neuron 2:  z = x·W2 + b2  →  ReLU(z)              │     │
│   │  ...                                                  │     │
│   │  Neuron 16: z = x·W16 + b16 → ReLU(z)              │     │
│   │                                                       │     │
│   │  ReLU: if z > 0, keep z.  if z ≤ 0, output 0.       │     │
│   │  (neuron only fires if its pattern is present)        │     │
│   └──────────────────────┬───────────────────────────────┘     │
│                          │  (each of 16 connects to both)      │
│                          ▼                                      │
│   OUTPUT LAYER  (2 neurons, each with 16 weights + 1 bias)     │
│   ┌──────────────────────────────────────────────────────┐     │
│   │  Neuron A:  raw score for "safe"                     │     │
│   │  Neuron B:  raw score for "near-miss"                │     │
│   │                                                       │     │
│   │  Softmax converts both scores to probabilities:       │     │
│   │    P(safe) + P(near-miss) = 1.0  always              │     │
│   └──────────────────────┬───────────────────────────────┘     │
│                          │                                      │
│                          ▼                                      │
│   OUTPUT                                                        │
│   ┌──────────────────────────────────────────────────────┐     │
│   │  P(safe)      = 0.72                                 │     │
│   │  P(near-miss) = 0.28   ← prediction                 │     │
│   └──────────────────────┬───────────────────────────────┘     │
└─────────────────────────┬┴─────────────────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────────────────┐
│                     COMPARE TO LABEL                            │
│                                                                 │
│   Real label (from button press):  1  (near-miss)              │
│   Network prediction:              0.28  (wrong)               │
│                                                                 │
│   Loss = -log(0.28) = 1.27   ← high = bad prediction          │
└─────────────────────┬───────────────────────────────────────────┘
                      │
                      ▼
┌─────────────────────────────────────────────────────────────────┐
│                   BACKPROPAGATION                               │
│                                                                 │
│   Work backwards through the network                           │
│   For each of the 450 weights, calculate:                       │
│     gradient = how much did this weight cause the error?        │
│                                                                 │
│   Uses calculus (chain rule) — TensorFlow does this            │
│   automatically                                                 │
└─────────────────────┬───────────────────────────────────────────┘
                      │
                      ▼
┌─────────────────────────────────────────────────────────────────┐
│                    UPDATE WEIGHTS                               │
│                                                                 │
│   For every weight:                                             │
│     new_weight = old_weight - (0.001 × gradient)               │
│                                                                 │
│   All 450 weights nudged slightly toward better predictions     │
└─────────────────────┬───────────────────────────────────────────┘
                      │
                      ▼
┌─────────────────────────────────────────────────────────────────┐
│                  REPEAT (LOOP)                                  │
│                                                                 │
│   Process 32 samples at a time (one batch)                     │
│   1,837 batches = one epoch (all training data seen once)      │
│   Repeat for 44 epochs  →  2.6 million total corrections       │
│                                                                 │
│   After each epoch: test on validation set                      │
│   If no improvement for 15 epochs: stop early                  │
└─────────────────────┬───────────────────────────────────────────┘
                      │
                      ▼
┌─────────────────────────────────────────────────────────────────┐
│                   CONVERT & SAVE                                │
│                                                                 │
│   Keras model (.h5)  →  TFLite model (.tflite, 3.5 KB)        │
│   TFLite model  →  C array in model_data.h                     │
│   Scaler parameters (mean, std for 25 features)  →  model_data.h│
└─────────────────────────────────────────────────────────────────┘
```

---

## Diagram 2: Inference Flow (runs on ESP32, 60 times per second)

This is what happens every 16.7 ms on the device after flashing.

```
┌─────────────────────────────────────────────────────────────────┐
│                      SENSORS                                    │
│                                                                 │
│   VL53L8CX ToF  →  16 distance values (mm)                    │
│   BNO08x IMU    →  roll, pitch, yaw, ax, ay, az, gx, gy, gz   │
│                                    25 raw values total          │
└─────────────────────┬───────────────────────────────────────────┘
                      │
                      ▼
┌─────────────────────────────────────────────────────────────────┐
│                    NORMALISE                                    │
│                                                                 │
│   For each of the 25 values:                                    │
│     normalised = (raw_value - mean) / std                       │
│                                                                 │
│   mean and std are the saved scaler parameters                  │
│   (embedded in model_data.h, calculated during training)        │
└─────────────────────┬───────────────────────────────────────────┘
                      │
                      ▼
┌─────────────────────────────────────────────────────────────────┐
│                  NEURAL NETWORK (TFLite)                        │
│                                                                 │
│   25 normalised inputs                                          │
│        ↓                                                        │
│   Hidden layer: Z1 = X · W1ᵀ + b1,  A1 = ReLU(Z1)            │
│        ↓  (16 values)                                           │
│   Output layer: Z2 = A1 · W2ᵀ + b2,  A2 = softmax(Z2)        │
│        ↓  (2 values)                                            │
│   [P(safe),  P(near-miss)]                                      │
│                                                                 │
│   Runs in 5–15 ms using ESP-NN SIMD acceleration               │
└─────────────────────┬───────────────────────────────────────────┘
                      │
                      ▼
┌─────────────────────────────────────────────────────────────────┐
│                   THRESHOLD CHECK                               │
│                                                                 │
│   If P(near-miss) > 0.70  →  NEAR-MISS DETECTED               │
│   If P(near-miss) ≤ 0.70  →  SAFE                             │
└──────────┬──────────────────────────────┬───────────────────────┘
           │ NEAR-MISS                    │ SAFE
           ▼                              ▼
┌──────────────────────┐      ┌──────────────────────────────────┐
│  ALERT               │      │  No action                       │
│  - Buzzer sounds     │      │  Continue monitoring             │
│  - OLED shows alert  │      │  Next frame in 16.7 ms           │
│  - flag_auto = 1     │      └──────────────────────────────────┘
│    logged to CSV     │
└──────────────────────┘
```

---

## Diagram 3: The Single Neuron (for the detail box in your diagram)

Use this to show what happens inside one neuron.

```
  Input 1 (tof_00) ──×── weight w1 ──┐
  Input 2 (tof_01) ──×── weight w2 ──┤
  Input 3 (tof_02) ──×── weight w3 ──┤
  ...                                 ├──→ SUM ──→ + bias ──→ ReLU ──→ output
  Input 25 (gz)    ──×── weight w25 ─┘

  SUM  = w1×x1 + w2×x2 + ... + w25×x25
  z    = SUM + b
  out  = ReLU(z) = max(0, z)

  If z > 0:  output = z   (neuron fires, passes signal forward)
  If z ≤ 0:  output = 0   (neuron silent, pattern not found)
```

---

## Diagram 4: Weight Update (for the learning loop box)

```
  Before training:   w = random value (e.g. -0.2)

  After one correction:
    gradient = how much w caused the error  (e.g. 3.4)
    w_new = w_old - (learning_rate × gradient)
    w_new = -0.2  - (0.001      × 3.4)
    w_new = -0.2034

  After 2.6 million corrections:
    w has settled to a value that minimises prediction error
```

---

## Key Numbers to Label on Your Diagram

| Location | Value | What it means |
|----------|-------|---------------|
| Input layer | 25 | Number of sensor features |
| Hidden layer | 16 | Number of neurons |
| Output layer | 2 | Safe / Near-miss |
| Total weights | 450 | Adjustable parameters |
| Batch size | 32 | Samples processed together |
| Epochs | 44 | Times through all training data |
| Total corrections | 2.6 million | Weight updates during training |
| Learning rate | 0.001 | Step size per correction |
| Detection threshold | 0.70 | Minimum P(near-miss) to fire alert |
| Model size | 3.5 KB | TFLite file on ESP32 |
| Inference time | 5–15 ms | Time to run one prediction |
| Frame budget | 16.7 ms | Time available at 60 Hz |

---

## What to Draw

**Training diagram** — a vertical flowchart with these boxes:
1. Data Collection (CSV files, button press)
2. Load & Merge (521,824 rows)
3. Prepare Data (normalise → balance → split)
4. Neural Network (input → hidden → output)
5. Compare to Label (loss calculation)
6. Backpropagation (gradient calculation)
7. Update Weights (gradient descent)
8. Loop arrow back to step 4 (repeat 2.6M times)
9. Convert & Save (TFLite → model_data.h)

**Inference diagram** — a vertical flowchart with these boxes:
1. Sensors (ToF + IMU → 25 values)
2. Normalise (apply saved scaler)
3. Neural Network (25 → 16 → 2)
4. Threshold Check (> 0.70?)
5. Two branches: Alert (buzzer + OLED) or Safe (continue)

**Neuron detail** — a small inset diagram showing one neuron:
- Multiple inputs with weights on the arrows
- Summation circle
- Bias added
- ReLU box
- Single output
