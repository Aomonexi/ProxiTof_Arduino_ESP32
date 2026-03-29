# Near-Miss Detection: Mathematical Formulations and Algorithm Design

**Last Updated**: March 27, 2026

This document covers all mathematical formulations, filtering strategies, rule-based detection logic, state machine design, and threshold justifications used in the ProxiTof near-miss detection system. All formulas are verified against the actual codebase. This document is structured to support thesis Chapter 4.

---

## 4.1 Mathematical Formulations

### 4.1.1 Feature Extraction

The system computes 29 features per synchronized frame: 25 raw sensor values and 4 derived features.

**Raw features (25):**
- 16 ToF distance values (d₀ ... d₁₅) in mm
- 3 orientation values (roll, pitch, yaw) in degrees
- 3 linear acceleration values (aₓ, aᵧ, a_z) in m/s²
- 3 angular velocity values (ωₓ, ωᵧ, ω_z) in deg/s

**Derived features (4):**

**Minimum distance:**
```
d_min[n] = min(dᵢ[n])   for i = 0, 1, ..., 15
```
After filtering (see Section 4.1.2), this becomes `d̂_min[n]`.

Source: `data_processor.cpp → calculateMinDistance()`

**Approach rate** (first derivative of filtered minimum distance):
```
v[n] = (d̂_min[n] - d̂_min[n-1]) / Δt
```
Where Δt is the time between consecutive ToF frames (~16.7ms at 60Hz). Negative value = object approaching. Units: mm/s.

Source: `data_processor.cpp → calculateApproachRate()`

**Jerk magnitude** (first derivative of acceleration magnitude):
```
jₓ[n] = (aₓ[n] - aₓ[n-1]) / Δt
jᵧ[n] = (aᵧ[n] - aᵧ[n-1]) / Δt
j_z[n] = (a_z[n] - a_z[n-1]) / Δt

J[n] = √(jₓ² + jᵧ² + j_z²)
```
Where Δt is the time between IMU samples (~10ms at 100Hz). Units: m/s³.

Source: `data_processor.cpp → calculateJerkMagnitude()`

**Angular velocity magnitude:**
```
ω[n] = √(ωₓ² + ωᵧ² + ω_z²)
```
Units: deg/s.

Source: `data_processor.cpp → calculateAngularVelocity()`

---

### 4.1.2 Filtering

Three moving average filters are used in the system. All three use a window size of N=5.

#### Why Moving Average?

The simple moving average was chosen over more complex filters (Kalman, Butterworth, median) for the following reasons:

1. **Simplicity**: O(1) per sample with a circular buffer — no matrix operations, no state estimation
2. **Computational cost**: A single addition and division per sample. On the ESP32-S3 running at 240MHz with ToF at 60Hz and IMU at 100Hz, CPU budget for filtering is minimal
3. **Deterministic latency**: Fixed N-sample delay, no convergence time or tuning parameters beyond window size
4. **Sufficient for the noise profile**: IMU sensor noise is approximately Gaussian with short correlation time. A moving average effectively reduces variance by factor 1/N without introducing phase distortion at the frequencies of interest (near-miss events at 1–10Hz)
5. **Real-time embedded constraint**: The system runs on a single-core FreeRTOS task with no floating-point coprocessor for complex DSP. Heavier filters would compete with sensor I/O, OLED rendering, GPS parsing, and ML inference for CPU time

#### Why N=5?

| Filter | Sensor Rate | Window Duration | Rationale |
|--------|-------------|-----------------|-----------|
| Distance (data processor) | 60 Hz | ~83ms | Short enough to not lag a vehicle at 30km/h (~138mm/frame). Long enough to smooth 10–30mm tilt vibration. |
| Jerk (OLED display) | 100 Hz | ~50ms | A swerve reaction takes ~100–200ms, so 50ms captures the peak. Larger window would over-smooth. |
| Distance (OLED EMA) | 60 Hz | Exponential decay | EMA with α=0.7 gives effective window ~3 samples. Used only for display smoothness. |

---

#### Filter A: Adaptive Distance Filter (Data Processor)

Applied to minimum distance before approach rate calculation. This is the only filter in the detection pipeline.

**Step 1 — Circular buffer moving average:**
```
d̄[n] = (1/N) × Σ d_raw[n-i]   for i = 0 to N-1,  N = 5
```

**Step 2 — Change magnitude:**
```
Δd = |d_raw[n] - d̄[n]|
```

**Step 3 — Adaptive output:**
```
           ┌ d_raw[n]                            if Δd > 100mm   (bypass: fast object)
d̂[n] =    │ 0.7 × d_raw[n] + 0.3 × d̄[n]       if 20 < Δd ≤ 100mm  (weighted blend)
           └ d̄[n]                                if Δd ≤ 20mm    (use filtered: noise)
```

When Δd > 100mm (bypass), the entire buffer is reset to `d_raw[n]` to prevent lag on subsequent frames.

**Why adaptive?** A pure moving average would smooth out a real fast-approaching vehicle. At 60Hz, a motorcycle passing at 30km/h causes ~138mm distance change per frame — well above the 100mm bypass threshold. The filter only smooths small vibration noise (10–30mm) while preserving real events.

Source: `data_processor.cpp → applyDistanceFilter()`

---

#### Filter B: Jerk Moving Average (OLED Display Only)

Applied to jerk magnitude for display smoothing and the OLED's own state machine. **Not applied in the data processor or CSV logging.**

```
J̄[n] = (1/N) × Σ J[n-i]   for i = 0 to N-1,  N = 5
```

**Where it is used vs. not used:**
- OLED display text (J̄ shown on screen) ✓
- OLED state machine detection (threshold 50 m/s³ against J̄) ✓
- Data processor detection (threshold 7 m/s³ against raw J) — **not filtered**
- CSV log (raw J written) — **not filtered**
- ML inference input (raw acceleration, not jerk) — **not applicable**

**Why only on display?** The data processor and ML model should see the raw signal. The ML model learns its own internal filtering during training. The display needs a clean number for human readability.

Source: `oled_display.c → calculate_jerk_moving_average()`

---

#### Filter C: EMA Distance Filter (OLED Depth Map Only)

Applied per-zone (16 independent filters) for the dithered depth map visualization. **Not used in detection or logging.**

```
           ┌ α × d_raw[n] + (1-α) × d_ema[n-1]     if d_raw ≤ 4000mm (valid)
d_ema[n] = │
           └ 0.1 × 4001 + 0.9 × d_ema[n-1]         if d_raw > 4000mm (invalid)
```

Where α = 0.7 (EMA_ALPHA). The invalid-reading path slowly fades to empty rather than jumping, preventing display flicker when a zone briefly loses signal.

Source: `oled_display.c → fb_draw_depth_map()`

---

### 4.1.3 Sensor Synchronization

ToF (60Hz) and IMU (100Hz) produce data at different rates. The data processor synchronizes them within a 10ms window.

```
Δt_sync = |t_tof[n] - t_imu[n]|

if Δt_sync ≤ 10ms:
  synced = (tof[n], imu[n])         ← direct pairing

if Δt_sync > 10ms AND imu[n-1] exists:
  synced = (tof[n], imu[n-1])       ← use last IMU (still recent)

if Δt_sync > 10ms AND no previous IMU:
  synced = FAIL                      ← cannot synchronize
```

The ToF timestamp is always used as the reference because ToF is the lower-rate sensor. Since IMU updates ~1.67× more frequently, the most recent IMU reading is always within ~10ms of any ToF frame.

Source: `data_processor.cpp → synchronizeData()`

---

### 4.1.4 Feature Count Clarification

| Context | Feature Count | What's Included |
|---------|--------------|-----------------|
| `feature_vector_t` (in-memory) | 29 | 25 raw + 4 derived (min_distance, approach_rate, jerk, angular_velocity) + flags |
| ML inference input | 25 | 16 ToF + 9 IMU (raw only, z-score normalized) |
| CSV log on SD card | 26–31 | timestamp + 16 ToF + 9 IMU + optional GPS (5 fields) + 2 flags |

The 4 derived features (min_distance, approach_rate, jerk, angular_velocity) are computed in memory by the data processor and used for rule-based detection, but they are **not currently written to the CSV file**. The `log_entry_t` struct contains only raw sensor values, GPS fields, and flags. Derived features can be recomputed from the raw CSV data during offline analysis.

The ML model receives 25 raw features (not derived features). It learns its own internal representations during training.

---

## 4.2 Rule-Based Detection

Two independent rule-based detection systems run in parallel. Both use the same raw sensor data but apply different thresholds and logic structures.

### 4.2.1 Data Processor Detection (Primary)

This is the formal detection pipeline. It drives buzzer alerts, CSV `flag_auto`, LED color, and emergency SD card flush.

**Validity gate:**
```
if d̂_min[n] < 20mm → REJECT (sensor blocked, not a real object)
```

**Tier 1 — Critical Proximity (ToF only):**
```
DETECT = (20mm ≤ d̂_min[n] < 100mm) AND (v[n] < -500 mm/s)
```
Immediate danger. No IMU data required.

**Tier 2 — Proximity Alert (ToF only):**
```
DETECT = (d̂_min[n] < 150mm) AND (v[n] < -1000 mm/s)
```
Fast approach in the near-miss zone. No IMU data required.

**Combined detection (internal state tracking):**
```
rapidApproachActive = true   if v[n] < -1000 mm/s
                     false   if no rapid approach for 1000ms

highJerkActive = true        if J[n] > 7.0 m/s³
                false        if no high jerk for 1000ms

COMBINED = rapidApproachActive AND highJerkActive
           AND |t_rapid - t_jerk| ≤ 500ms
```

In the current implementation, Tier 1 and Tier 2 already trigger without requiring jerk, so the combined check provides additional confirmation for severity classification rather than being a separate trigger.

Source: `data_processor.cpp → detectNearMiss()`

---

### 4.2.2 OLED State Machine Detection (Secondary)

This drives the on-screen near-miss indicator and reason text. It uses a time-based state machine with different thresholds tuned for display feedback.

**Conditions evaluated during an active approach:**

| Condition | Formula | Threshold |
|-----------|---------|-----------|
| Critical proximity | d_min_during_approach < T_crit | T_crit = 200mm |
| Sustained rapid approach | (500ms < duration < 5000ms) AND (v[n] < -T_rapid) | T_rapid = 800 mm/s |
| High jerk during approach | J̄[n] > T_jerk AND (now - t_jerk < 2000ms) | T_jerk = 50 m/s³ (filtered) |
| Evasive rotation | ω[n] > T_rot | T_rot = 300 deg/s |

```
NEAR_MISS_DISPLAY = Condition1 OR Condition2 OR Condition3 OR Condition4
```

**Standalone proximity (outside state machine):**
```
if NOT approach_active AND d_min[n] < 150mm → NEAR_MISS_DISPLAY = true
```

Source: `oled_display.c → oled_display_update()`

---

## 4.3 State Machine Design

### 4.3.1 OLED Near-Miss State Machine

The OLED detection uses a 4-state lifecycle that tracks an approach event from start to resolution:

```
┌─────────────────────────────────────────────────────────┐
│                                                         │
│  ┌──────────┐    v < -800    ┌──────────────────────┐   │
│  │  IDLE    │ ──────────────>│  APPROACH ACTIVE     │   │
│  │          │                │                      │   │
│  └──────────┘                │  Track:              │   │
│       ▲                      │  - d_min (running)   │   │
│       │                      │  - J̄_max             │   │
│       │                      │  - duration          │   │
│       │                      └──────────┬───────────┘   │
│       │                                 │               │
│       │                      ┌──────────▼───────────┐   │
│       │                      │  EVALUATE            │   │
│       │                      │                      │   │
│       │                      │  Check 4 conditions  │   │
│       │                      │  (any = near-miss)   │   │
│       │                      └──────────┬───────────┘   │
│       │                                 │               │
│       │   v > -100 OR                   │               │
│       │   duration > 5s                 │               │
│       │                      ┌──────────▼───────────┐   │
│       └──────────────────────│  RESET               │   │
│                              │  approach_active=false│   │
│                              └──────────────────────┘   │
│                                                         │
└─────────────────────────────────────────────────────────┘
```

**State transitions:**

| From | To | Condition |
|------|----|-----------|
| IDLE | APPROACH ACTIVE | v[n] < -800 mm/s |
| APPROACH ACTIVE | EVALUATE | Every frame while active |
| EVALUATE | RESET | v[n] > -100 mm/s OR duration > 5000ms |
| RESET | IDLE | Immediate |

**During APPROACH ACTIVE, tracked variables:**
- `d_min_during_approach`: running minimum of d_min across all frames since approach started
- `jerk_detected`: flag set if J̄[n] > 50 m/s³ at any point
- `t_jerk`: timestamp of last high jerk
- `J_max`: maximum filtered jerk during approach
- `t_start`: timestamp when approach began

---

### 4.3.2 Detection Mode State Machine

The system supports three detection modes, cycled via the ACTION button in Near-Miss mode (Mode 5):

```
Mode 0: Hybrid     → DETECT = Algorithm OR ML
Mode 1: Algorithm  → DETECT = Algorithm only
Mode 2: Neural Net → DETECT = ML only
```

Near-miss event persistence (both paths):
```
if DETECT:
  near_miss_active = true
  t_event = now

if NOT DETECT AND (now - t_event > 2000ms):
  near_miss_active = false
```

The 2-second persistence prevents rapid on/off flickering when detection is marginal.

---

## 4.4 Threshold Selection and Justification

### 4.4.1 Data Processor Thresholds

| Threshold | Value | Justification |
|-----------|-------|---------------|
| MIN_VALID_DISTANCE | 20mm | VL53L8CX minimum ranging distance. Below this, the IR detector saturates and returns invalid readings. Filters out sensor blockage (finger on lens, device on table). |
| CRITICAL_PROXIMITY_DISTANCE | 100mm | 10cm is within arm's reach — an object this close while approaching is immediate danger regardless of speed. Based on cycling safety literature where <15cm is considered a close pass. |
| CRITICAL_PROXIMITY_APPROACH | 500 mm/s | 0.5 m/s approach at <100mm means impact within 0.2 seconds. Combined with the close distance, this is unambiguously dangerous. Lower than Tier 2 threshold because the distance is already critical. |
| RAPID_APPROACH_THRESHOLD | 1000 mm/s | 1.0 m/s = 3.6 km/h relative approach speed. A motorcycle passing at 30km/h while cyclist rides at 20km/h has ~10km/h = 2.8 m/s relative speed. 1.0 m/s is conservative enough to catch real events while filtering out slow drift (walking past, gradual lane changes). Originally 500 mm/s, increased to reduce false positives from normal cycling vibration. |
| COMBINED_DETECTION_MAX_DIST | 150mm | 15cm defines the "near-miss zone" — close enough to be dangerous but not yet critical. Based on cycling infrastructure guidelines where <1.5m is a close pass and <0.15m is a near-miss. |
| HIGH_JERK_THRESHOLD | 7.0 m/s³ | Distinguishes sudden body reactions from normal cycling motion. Normal pedaling: <2 m/s³. Bumpy road: 2–5 m/s³ (high frequency, filtered). Turning: gradual, <3 m/s³. Near-miss swerve: >7 m/s³ (sudden, high amplitude). Originally 5.0, increased after field testing showed false positives from rough road surfaces. |
| COMBINED_WINDOW | 500ms | Time window for co-occurrence of rapid approach and high jerk. A near-miss event (vehicle passes, cyclist reacts) typically completes within 0.5 seconds. Longer window would associate unrelated events. |

### 4.4.2 OLED State Machine Thresholds

| Threshold | Value | Why Different from Data Processor |
|-----------|-------|-----------------------------------|
| Rapid approach start | 800 mm/s | Lower than data processor (1000) to start tracking earlier. The state machine needs to begin tracking before the event peaks. |
| Critical proximity | 200mm | Higher than data processor (100) because the state machine tracks minimum distance over the entire approach duration, not just the current frame. |
| High jerk (filtered) | 50 m/s³ | Much higher than data processor (7.0) because this threshold is applied to the 5-sample moving average filtered jerk. The moving average reduces peak values significantly — a raw 7 m/s³ spike becomes ~1.4 m/s³ after averaging with 4 zero-ish samples. The 50 m/s³ filtered threshold corresponds to a sustained high-jerk event, not a single spike. |
| Approach end | 100 mm/s | Object is no longer approaching meaningfully. Resets the state machine. |
| Min approach duration | 500ms | Filters out momentary sensor glitches. A real vehicle pass takes at least 0.5 seconds. |
| Max approach duration | 5000ms | Safety timeout. If approach hasn't resolved in 5 seconds, something is wrong (sensor stuck, stationary object). |
| Jerk relevance window | 2000ms | Jerk detected during approach is relevant for up to 2 seconds. After that, it's likely unrelated to the current approach event. |
| High rotation | 300 deg/s | Evasive maneuver threshold. Normal cycling turning: <100 deg/s. Sudden swerve to avoid collision: >300 deg/s. |

### 4.4.3 Threshold Evolution

Several thresholds were adjusted during development based on field testing:

| Parameter | Original | Current | Reason for Change |
|-----------|----------|---------|-------------------|
| Rapid approach | 500 mm/s | 1000 mm/s | Too many false positives from cycling vibration and tilt |
| High jerk | 2.0 m/s³ | 7.0 m/s³ | Normal walking/running triggered detection |
| Critical proximity | 300mm | 100mm | 30cm is too far — normal cycling near walls triggered it |
| High rotation | 180 deg/s | 300 deg/s | Normal turns triggered detection |
| Combined window | None | 500ms | Without temporal constraint, unrelated events were combined |
| Min valid distance | 0mm | 20mm | Sensor blockage (0mm readings) caused false critical alerts |

---

## Pipeline Figures and Algorithms

### Figure 1: Near-Miss Detection System Pipeline

This figure illustrates the complete data flow from sensor acquisition through detection to system output. The pipeline operates continuously at the ToF frame rate (60 Hz), with sensor synchronization bridging the rate difference between ToF and IMU.

```
  ┌───────────────────────────────────────────────────────────────────┐
  │                    SENSOR ACQUISITION LAYER                       │
  │                                                                   │
  │  ┌─────────────┐    ┌─────────────┐    ┌──────────────────────┐  │
  │  │ VL53L8CX    │    │ BNO08x      │    │ Neo-M9N GPS          │  │
  │  │ ToF Sensor  │    │ IMU Sensor  │    │ (FreeRTOS task)      │  │
  │  │             │    │             │    │                      │  │
  │  │ I2C, 60 Hz  │    │ SPI2, 100Hz │    │ UART1, 1 Hz          │  │
  │  │ 16 zones    │    │ 9-DOF       │    │ NMEA parsing         │  │
  │  │ (mm)        │    │ (deg, m/s², │    │ (lat, lon, speed,    │  │
  │  │             │    │  deg/s)     │    │  heading, sats)      │  │
  │  └──────┬──────┘    └──────┬──────┘    └──────────┬───────────┘  │
  └─────────┼──────────────────┼──────────────────────┼──────────────┘
            │                  │                      │
            ▼                  ▼                      │
  ┌─────────────────────────────────────┐             │
  │  SENSOR SYNCHRONIZATION             │             │
  │                                     │             │
  │  Δt = |t_tof - t_imu|              │             │
  │  if Δt ≤ 10 ms: pair directly      │             │
  │  if Δt > 10 ms: use last IMU       │             │
  │  Reference timestamp: t_tof        │             │
  └──────────────┬──────────────────────┘             │
                 │                                    │
                 │  synced_data_t                      │
                 │  (25 raw features)                  │
                 ▼                                    │
  ┌─────────────────────────────────────┐             │
  │  FEATURE EXTRACTION                 │             │
  │                                     │             │
  │  d̂_min = AdaptiveFilter(min(d_i))  │             │
  │  v     = (d̂_min[n] - d̂_min[n-1])/Δt│             │
  │  J     = ‖Δa‖ / Δt                 │             │
  │  ω     = ‖gyro‖                    │             │
  │                                     │             │
  │  Output: feature_vector_t           │             │
  │  (25 raw + 4 derived = 29 features) │             │
  └──────────────┬──────────────────────┘             │
                 │                                    │
         ┌───────┴────────┐                           │
         ▼                ▼                           │
  ┌──────────────┐  ┌──────────────┐                  │
  │  Rule-Based  │  │  ML-Based    │                  │
  │  Detection   │  │  Detection   │                  │
  │              │  │              │                  │
  │  Input:      │  │  Input:      │                  │
  │  4 derived   │  │  25 raw      │                  │
  │  features    │  │  (z-score    │                  │
  │              │  │  normalized) │                  │
  │  Tier 1:     │  │              │                  │
  │  d<100,      │  │  TFLite      │                  │
  │  v<-500      │  │  25→16→2     │                  │
  │              │  │              │                  │
  │  Tier 2:     │  │  P(nm)>0.70  │                  │
  │  d<150,      │  │  → detect    │                  │
  │  v<-1000     │  │              │                  │
  └──────┬───────┘  └──────┬───────┘                  │
         │                 │                          │
         ▼                 ▼                          │
  ┌─────────────────────────────────────┐             │
  │  DETECTION MODE COMBINATION         │             │
  │                                     │             │
  │  Mode 0 (Hybrid):  A ∨ M           │             │
  │  Mode 1 (Algo):    A               │             │
  │  Mode 2 (ML):      M               │             │
  │                                     │             │
  │  Persistence: 2 s after last detect │             │
  └──────────────┬──────────────────────┘             │
                 │                                    │
  ┌──────────────┼──────────────┬─────────────────────┤
  │              │              │                     │
  ▼              ▼              ▼                     ▼
  ┌────────┐  ┌────────┐  ┌──────────┐  ┌────────────────────┐
  │ Buzzer │  │  LED   │  │   OLED   │  │  SD Card CSV Log   │
  │ PWM    │  │ WS2812 │  │  SSD1306 │  │                    │
  │ alert  │  │ color  │  │  display │  │  25 raw sensor     │
  │ pattern│  │ state  │  │  depth   │  │  + GPS (optional)  │
  │        │  │        │  │  map     │  │  + event flags     │
  └────────┘  └────────┘  └──────────┘  └────────────────────┘
```

*Figure 1. Near-miss detection system pipeline. Sensor data flows from acquisition (top) through synchronization, feature extraction, and dual-path detection (rule-based and ML) to system outputs (bottom). GPS data is merged at the logging stage for geo-tagging. The pipeline executes at the ToF frame rate of 60 Hz.*

---

### Figure 2: OLED Near-Miss State Machine

This figure shows the time-based state machine used for the OLED display's near-miss detection, which operates independently from the data processor detection.

```
                    ┌──────────────────────────────────┐
                    │                                  │
                    ▼                                  │
              ┌───────────┐                            │
              │           │                            │
              │   IDLE    │                            │
              │           │                            │
              └─────┬─────┘                            │
                    │                                  │
                    │ v[n] < -800 mm/s                 │
                    │ AND NOT active                   │
                    ▼                                  │
              ┌───────────────────┐                    │
              │                   │                    │
              │  APPROACH ACTIVE  │                    │
              │                   │                    │
              │  Initialize:      │                    │
              │  t_start = now    │                    │
              │  d_track = d_min  │                    │
              │  jerk_flag = F    │                    │
              │                   │                    │
              └─────┬─────────────┘                    │
                    │                                  │
                    │ Every frame                      │
                    ▼                                  │
              ┌───────────────────┐                    │
              │                   │                    │
              │  EVALUATE         │                    │
              │                   │                    │
              │  Update:          │                    │
              │  d_track = min()  │                    │
              │  Check J̄ > 50    │                    │
              │                   │                    │
              │  Test conditions: │                    │
              │  C1: d_track<200  │                    │
              │  C2: sustained    │                    │
              │      approach     │                    │
              │  C3: jerk within  │                    │
              │      2 s window   │                    │
              │  C4: ω > 300°/s  │                    │
              │                   │                    │
              │  ALERT if any Ci  │                    │
              └─────┬─────────────┘                    │
                    │                                  │
                    │ v[n] > -100 mm/s                 │
                    │ OR duration > 5 s                │
                    ▼                                  │
              ┌───────────────────┐                    │
              │                   │                    │
              │  RESET            │────────────────────┘
              │  active = false   │
              │                   │
              └───────────────────┘
```

*Figure 2. OLED near-miss state machine. The state machine tracks the lifecycle of an approach event: detection of rapid approach (IDLE → ACTIVE), continuous evaluation of four near-miss conditions (EVALUATE), and reset when the approach ends or times out (RESET → IDLE). An alert is triggered if any of the four conditions is satisfied during an active approach.*

---

### Algorithm 1: Main Detection Loop

```
Algorithm 1: Near-Miss Detection Main Loop
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

Input:  Continuous sensor streams (ToF at 60 Hz, IMU at 100 Hz, GPS at 1 Hz)
Output: Near-miss events (buzzer alert, LED indicator, CSV flag, OLED display)

 1:  loop
 2:    tof_ok ← ReadToF(tof_data)                          ▷ Non-blocking, ~60 Hz
 3:    imu_ok ← ReadIMU(imu_data)                          ▷ Non-blocking, ~100 Hz
 4:
 5:    if tof_ok AND imu_ok then
 6:      synced ← Synchronize(tof_data, imu_data, Δt_max = 10 ms)
 7:      if NOT synced then continue
 8:
 9:      ▷ ── Feature Extraction ──
10:      d_raw  ← min(tof_data.distances[0..15])
11:      d_min  ← AdaptiveFilter(d_raw)                    ▷ Algorithm 2
12:      v      ← (d_min − d_min_prev) / Δt               ▷ Approach rate (mm/s)
13:      J      ← ‖a[n] − a[n−1]‖ / Δt                   ▷ Jerk magnitude (m/s³)
14:      ω      ← ‖gyro‖                                  ▷ Angular velocity (deg/s)
15:
16:      ▷ ── Rule-Based Detection ──
17:      A ← FALSE
18:      if d_min < 20 then                                ▷ Sensor blocked
19:        A ← FALSE
20:      else if (20 ≤ d_min < 100) AND (v < −500) then   ▷ Tier 1: Critical
21:        A ← TRUE
22:      else if (d_min < 150) AND (v < −1000) then        ▷ Tier 2: Proximity
23:        A ← TRUE
24:      end if
25:
26:      ▷ ── ML-Based Detection (if model available) ──
27:      M ← FALSE
28:      if ml_ready then
29:        x_norm ← ZScoreNormalize(raw_25_features, μ, σ)
30:        p ← TFLitePredict(x_norm)                       ▷ P(near-miss)
31:        if p > 0.70 then M ← TRUE
32:      end if
33:
34:      ▷ ── Mode Combination ──
35:      switch detection_mode:
36:        case HYBRID:     result ← A ∨ M
37:        case ALGORITHM:  result ← A
38:        case NEURAL_NET: result ← M
39:
40:      ▷ ── Event Handling ──
41:      if result then
42:        PlayBuzzer(NEAR_MISS_PATTERN)
43:        SetLED(ORANGE)
44:        flag_auto ← 1
45:        FlushSDCard()                                    ▷ Emergency save
46:      end if
47:
48:      ▷ ── Data Logging ──
49:      entry ← {timestamp, tof[16], imu[9], GPS(), flags}
50:      WriteCSV(entry)                                    ▷ Raw values only
51:
52:      ▷ ── Display Update ──
53:      UpdateOLED(tof_data, imu_data, d_min)              ▷ At configured Hz
54:
55:    end if
56:
57:    ▷ ── Periodic Tasks ──
58:    if TimeSince(last_volume_check) > 100 ms then
59:      UpdateVolumeFromPotentiometer()                    ▷ ADC read at 10 Hz
60:    end if
61:  end loop
```

---

### Algorithm 2: Adaptive Distance Filter

```
Algorithm 2: Adaptive Distance Filter
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

Input:  d_raw (raw minimum distance in mm)
Output: d_out (filtered distance in mm)
State:  buffer[N], index, count    where N = 5

 1:  buffer[index] ← d_raw
 2:  index ← (index + 1) mod N
 3:  count ← min(count + 1, N)
 4:
 5:  d_avg ← (1/count) × Σ buffer[0..count−1]
 6:  Δd ← |d_raw − d_avg|
 7:
 8:  if Δd > 100 then                          ▷ Fast object — bypass filter
 9:    for i ← 0 to N−1 do buffer[i] ← d_raw   ▷ Reset buffer
10:    return d_raw
11:  else if Δd > 20 then                       ▷ Medium change — blend
12:    return 0.7 × d_raw + 0.3 × d_avg
13:  else                                        ▷ Small noise — filter
14:    return d_avg
15:  end if
```

---

### Algorithm 3: Sensor Synchronization

```
Algorithm 3: Sensor Synchronization
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

Input:  tof_data (with timestamp t_tof), imu_data (with timestamp t_imu)
Output: synced_data (paired sensor frame) or FAIL
Param:  W = 10 ms (synchronization window)

 1:  if NOT tof_data.valid OR NOT imu_data.valid then
 2:    return FAIL
 3:  end if
 4:
 5:  Δt ← |t_tof − t_imu|
 6:
 7:  if Δt ≤ W then                             ▷ Within window
 8:    synced ← (tof_data, imu_data, t = t_tof)
 9:  else if previous_imu exists then            ▷ Use last IMU
10:    synced ← (tof_data, previous_imu, t = t_tof)
11:  else
12:    return FAIL
13:  end if
14:
15:  if Δt > W then                              ▷ Drift detection
16:    log_warning("Timestamp drift: Δt ms")
17:  end if
18:
19:  previous_imu ← imu_data
20:  return synced
```

---

### Algorithm 4: OLED Near-Miss State Machine

```
Algorithm 4: OLED Near-Miss State Machine
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

Input:  d_min, v, J̄ (filtered jerk), ω, timestamp t
Output: near_miss_display (boolean), reasons[] (string array)
State:  active, t_start, d_track, jerk_flag, t_jerk, J_max

 1:  ▷ ── State 1: Detect approach start ──
 2:  if v < −800 AND NOT active then
 3:    active ← TRUE
 4:    t_start ← t
 5:    d_track ← d_min
 6:    jerk_flag ← FALSE
 7:    J_max ← 0
 8:  end if
 9:
10:  ▷ ── State 2: Track during approach ──
11:  if active then
12:    d_track ← min(d_track, d_min)
13:    if J̄ > 50 then
14:      jerk_flag ← TRUE;  t_jerk ← t;  J_max ← max(J_max, J̄)
15:    end if
16:
17:    duration ← t − t_start
18:    result ← FALSE
19:
20:    ▷ ── State 3: Evaluate conditions ──
21:    C1 ← (d_track < 200)                                ▷ Critical proximity
22:    C2 ← (duration > 500) AND (duration < 5000) AND (v < −800)  ▷ Sustained
23:    C3 ← (jerk_flag) AND (t − t_jerk < 2000)           ▷ Jerk during approach
24:    C4 ← (ω > 300)                                      ▷ Evasive rotation
25:
26:    near_miss_display ← C1 ∨ C2 ∨ C3 ∨ C4
27:
28:    ▷ ── State 4: Reset ──
29:    if v > −100 OR duration > 5000 then
30:      active ← FALSE
31:    end if
32:  end if
33:
34:  ▷ ── Standalone proximity check ──
35:  if NOT active AND d_min < 150 then
36:    near_miss_display ← TRUE
37:  end if
```

---

## CSV Structure Clarification

The CSV file on the SD card stores **raw sensor values only** — derived features are not written to disk.

**What is in the CSV:**
```
timestamp_ms                          (1 column)
tof_00, tof_01, ..., tof_33          (16 columns, raw mm)
roll, pitch, yaw                      (3 columns, degrees)
accel_x, accel_y, accel_z            (3 columns, m/s²)
gyro_x, gyro_y, gyro_z              (3 columns, deg/s)
latitude, longitude, speed_ms,        (5 columns, conditional on GPS config)
  heading_deg, gps_satellites
flag_manual, flag_auto                (2 columns, 0 or 1)
```

Total: 28 columns (without GPS) or 33 columns (with GPS enabled).

**What is NOT in the CSV:**
- min_distance_mm (derived — recomputable from 16 ToF values)
- approach_rate_mm_s (derived — recomputable from consecutive min distances)
- jerk_magnitude_m_s3 (derived — recomputable from consecutive accelerations)
- angular_velocity_deg_s (derived — recomputable from gyro values)
- ML confidence score (computed in memory, not persisted)

These derived features exist only in the `feature_vector_t` struct in memory during runtime. They can be recomputed from the raw CSV data during offline analysis or ML training, which is the intended workflow.

---

## Related Documentation

- [sensor-integration-guide.md](sensor-integration-guide.md) — Sensor hardware, wiring, data flow
- [near-miss-detection-algorithm.md](near-miss-detection-algorithm.md) — Detection algorithm narrative
- [ml-complete-guide.md](ml-complete-guide.md) — ML training and deployment
- [note-4-data-processor-implementation.md](note-4-data-processor-implementation.md) — Thesis note on data processor
- [note-5-near-miss-algorithm-integration.md](note-5-near-miss-algorithm-integration.md) — Thesis note on algorithm
