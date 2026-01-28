# Hybrid Detection Strategy - Algorithms + Manual Flags + ESP-NN

**Date**: January 27, 2026  
**Purpose**: Explain how rule-based algorithms help train ESP-NN and work together

---

## Overview

The system uses a **3-tier hybrid approach** that combines:
1. **Rule-based algorithms** (immediate, deterministic)
2. **Manual flagging** (human ground truth)
3. **ESP-NN** (learned patterns)

Each tier complements the others to create a robust detection system.

---

## The Three Detection Tiers

```
┌─────────────────────────────────────────────────────────────────┐
│                    TIER 1: RULE-BASED ALGORITHMS                │
│                     (Active from Day 1)                         │
└─────────────────────────────────────────────────────────────────┘
                              ↓
        ┌─────────────────────────────────────────┐
        │  Detects obvious near-misses:           │
        │  - Critical proximity (< 300mm)         │
        │  - Rapid approach (> 500mm/s)           │
        │  - High jerk (> 20 m/s³)                │
        │  - High rotation (> 180 deg/s)          │
        │  → Sets flag_auto = 1                   │
        └─────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────────┐
│                    TIER 2: MANUAL FLAGGING                      │
│                     (Human supervision)                         │
└─────────────────────────────────────────────────────────────────┘
                              ↓
        ┌─────────────────────────────────────────┐
        │  You press ACTION button when:          │
        │  - Algorithm missed something           │
        │  - Subjective danger (felt unsafe)      │
        │  - Context-aware (know what's coming)   │
        │  → Sets flag_manual = 1                 │
        └─────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────────┐
│                    TIER 3: ESP-NN (FUTURE)                      │
│                  (Learns from Tier 1 + Tier 2)                  │
└─────────────────────────────────────────────────────────────────┘
                              ↓
        ┌─────────────────────────────────────────┐
        │  Neural network learns patterns:        │
        │  - Combines sensor patterns             │
        │  - Learns from algorithm detections     │
        │  - Learns from manual flags             │
        │  - Discovers new patterns               │
        │  → Sets flag_auto = 1 (replaces Tier 1) │
        └─────────────────────────────────────────┘
```

---

## How Algorithms Help Training

### 1. Automatic Labeling (Weak Supervision)

**Problem**: You can't press the button for EVERY near-miss (too busy riding!)

**Solution**: Algorithms automatically label obvious cases

**Example Log Entries:**

```c
// Case 1: Algorithm detected, you didn't press button
log_entry_t entry1 = {
    .tof = {150, 180, 200, ...},      // Very close!
    .accel_x = 3.5, .accel_y = 2.1,   // High acceleration
    .flag_manual = 0,                  // You didn't press button
    .flag_auto = 1                     // Algorithm detected it
};
// Label for training: NEAR-MISS ✓

// Case 2: You pressed button, algorithm also detected
log_entry_t entry2 = {
    .tof = {250, 280, 300, ...},      // Close
    .gyro_x = 45.2, .gyro_y = 30.1,   // Fast rotation
    .flag_manual = 1,                  // You pressed button
    .flag_auto = 1                     // Algorithm also detected
};
// Label for training: NEAR-MISS ✓ (high confidence!)

// Case 3: You pressed button, algorithm didn't detect
log_entry_t entry3 = {
    .tof = {800, 850, 900, ...},      // Not that close
    .accel_x = 0.5, .accel_y = 0.2,   // Low acceleration
    .flag_manual = 1,                  // You pressed button (subjective danger)
    .flag_auto = 0                     // Algorithm missed it
};
// Label for training: NEAR-MISS ✓ (algorithm should learn this!)

// Case 4: Neither detected
log_entry_t entry4 = {
    .tof = {1500, 1600, 1700, ...},   // Far away
    .accel_x = 0.1, .accel_y = 0.05,  // Normal riding
    .flag_manual = 0,                  // You didn't press button
    .flag_auto = 0                     // Algorithm didn't detect
};
// Label for training: SAFE ✓
```

**Key Insight**: Algorithms provide **thousands of labeled examples** automatically!

---

### 2. Bootstrapping the Dataset

**Without Algorithms:**
- You must manually flag EVERY near-miss
- Exhausting and error-prone
- Miss many events while focused on riding
- Takes months to collect enough data

**With Algorithms:**
- Algorithms catch obvious cases automatically
- You only flag edge cases and subjective dangers
- Collect 10x more labeled data in same time
- Dataset ready in weeks, not months

**Example Dataset Composition:**

```
Total samples: 10,000
├─ Safe (flag_manual=0, flag_auto=0): 9,000 (90%)
├─ Algorithm detected (flag_auto=1): 800 (8%)
│  ├─ Confirmed by manual (flag_manual=1): 200 (2%)
│  └─ Not manually flagged: 600 (6%)
└─ Manual only (flag_manual=1, flag_auto=0): 200 (2%)
```

**Training Labels:**
- Near-miss: 1,000 samples (800 auto + 200 manual-only)
- Safe: 9,000 samples

**Ratio**: 9:1 (good for training!)

---

### 3. Feature Engineering Guidance

**Algorithms tell us WHICH features matter:**

**Current Algorithm Thresholds:**
```c
// Critical proximity
if (min_distance < 300) {
    near_miss = true;
}

// Rapid approach
if (approach_rate < -500.0f) {  // Negative = approaching
    near_miss = true;
}

// High jerk
if (jerk_magnitude > 20.0f) {
    near_miss = true;
}

// High rotation
if (angular_velocity > 180.0f) {
    near_miss = true;
}
```

**These become derived features for ESP-NN:**
```python
# Add derived features to training data
features = [
    *tof,                                    # 16 raw ToF values
    roll, pitch, yaw,                        # 3 orientation
    accel_x, accel_y, accel_z,              # 3 acceleration
    gyro_x, gyro_y, gyro_z,                 # 3 gyroscope
    min(tof),                                # Derived: min distance
    calculate_approach_rate(tof, prev_tof), # Derived: approach rate
    calculate_jerk(accel, prev_accel),      # Derived: jerk magnitude
    calculate_angular_vel(gyro)             # Derived: angular velocity
]
# Total: 28 features (16 + 9 + 3 derived)
```

**Neural network learns:**
- When to trust these features
- How to combine them
- New patterns algorithms missed

---

## Training Strategy: Progressive Learning

### Phase 1: Algorithm-Only (Weeks 1-2)

**Goal**: Collect initial dataset with algorithm labels

**Configuration:**
```c
// In main.c
bool use_algorithm_detection = true;
bool use_neural_network = false;
```

**What happens:**
- Algorithms detect obvious near-misses
- You manually flag edge cases
- System logs both `flag_auto` and `flag_manual`
- Collect 5,000-10,000 samples

**Dataset after Phase 1:**
- 500-1,000 near-miss samples (mostly algorithm-detected)
- 4,000-9,000 safe samples
- Ready for initial training!

---

### Phase 2: Train Initial Model (Week 3)

**Goal**: Train first ESP-NN model

**Process:**
1. Extract logs from SD card
2. Create training dataset:
   ```python
   # Label = 1 if EITHER flag is set
   y = (flag_manual | flag_auto).astype(int)
   ```
3. Train neural network
4. Evaluate performance (aim for >85% accuracy)
5. Deploy to ESP32

**Model learns:**
- Algorithm patterns (from `flag_auto`)
- Human judgment (from `flag_manual`)
- Combinations of both

---

### Phase 3: Hybrid Detection (Weeks 4-6)

**Goal**: Run algorithm AND neural network together

**Configuration:**
```c
// In main.c
bool use_algorithm_detection = true;   // Keep algorithms
bool use_neural_network = true;        // Add neural network
```

**Detection Logic:**
```c
// Run both detectors
bool algo_detected = detect_near_miss_algorithm(&features);
float nn_probability = predict_near_miss(features);
bool nn_detected = (nn_probability > 0.7);

// Combine results
bool near_miss = algo_detected || nn_detected;

// Log both for analysis
log_entry.flag_auto = near_miss ? 1 : 0;
log_entry.flag_manual = button_pressed ? 1 : 0;
log_entry.nn_confidence = nn_probability;  // NEW FIELD!
```

**Benefits:**
- Algorithms catch what NN misses
- NN catches what algorithms miss
- Best of both worlds!
- Collect more training data

---

### Phase 4: Neural Network Only (Week 7+)

**Goal**: Replace algorithms with trained NN

**Configuration:**
```c
// In main.c
bool use_algorithm_detection = false;  // Disable algorithms
bool use_neural_network = true;        // NN only
```

**When to switch:**
- NN accuracy > 95%
- NN detection rate > 90%
- False alarm rate < 5%
- Tested in real-world conditions

**Benefits:**
- Simpler code
- Faster inference
- Learns new patterns
- Adapts to your riding style

---

## Comparison: Algorithms vs Neural Network

### Rule-Based Algorithms

**Strengths:**
- ✅ Works immediately (no training needed)
- ✅ Deterministic (same input = same output)
- ✅ Explainable (know why it triggered)
- ✅ Fast (simple math)
- ✅ Provides training labels

**Weaknesses:**
- ❌ Fixed thresholds (doesn't adapt)
- ❌ Misses complex patterns
- ❌ Many false positives/negatives
- ❌ Requires manual tuning
- ❌ Can't learn from experience

**Example Misses:**
- Slow approach but dangerous context
- Multiple small movements (cumulative danger)
- Subjective danger (feels unsafe but metrics OK)

---

### Neural Network (ESP-NN)

**Strengths:**
- ✅ Learns patterns from data
- ✅ Adapts to your riding style
- ✅ Discovers new patterns
- ✅ Handles complex combinations
- ✅ Improves with more data

**Weaknesses:**
- ❌ Needs training data (weeks to collect)
- ❌ Black box (hard to explain)
- ❌ Slower inference (more computation)
- ❌ Can overfit to training data
- ❌ Requires retraining for updates

**Example Catches:**
- Subtle patterns algorithms miss
- Context-aware detection
- Personalized to your riding
- Combinations of weak signals

---

## Hybrid Approach: Best of Both Worlds

### Strategy 1: Ensemble Detection

**Use both detectors, combine results:**

```c
// Run both
bool algo_detected = detect_algorithm(&features);
float nn_probability = predict_nn(features);

// Weighted combination
float combined_score = 0.3 * algo_detected + 0.7 * nn_probability;
bool near_miss = (combined_score > 0.5);
```

**Benefits:**
- Algorithms provide safety net
- NN adds intelligence
- Reduces false negatives

---

### Strategy 2: Confidence-Based Switching

**Use algorithm when NN is uncertain:**

```c
float nn_probability = predict_nn(features);

if (nn_probability > 0.8 || nn_probability < 0.2) {
    // NN is confident, trust it
    near_miss = (nn_probability > 0.5);
} else {
    // NN is uncertain, use algorithm
    near_miss = detect_algorithm(&features);
}
```

**Benefits:**
- NN handles clear cases
- Algorithm handles edge cases
- Best accuracy

---

### Strategy 3: Algorithm as Features

**Feed algorithm outputs to NN:**

```python
# Add algorithm outputs as features
features = [
    *tof,                           # 16 ToF
    *imu,                           # 9 IMU
    min_distance,                   # Derived
    approach_rate,                  # Derived
    jerk_magnitude,                 # Derived
    angular_velocity,               # Derived
    algo_critical_proximity,        # Algorithm output (0 or 1)
    algo_rapid_approach,            # Algorithm output (0 or 1)
    algo_high_jerk,                 # Algorithm output (0 or 1)
    algo_high_rotation              # Algorithm output (0 or 1)
]
# Total: 32 features
```

**Benefits:**
- NN learns when to trust algorithms
- NN can override algorithms
- Combines human + algorithm knowledge

---

## Training Data Composition

### Ideal Dataset

**Total samples: 10,000**

**Near-miss samples: 1,000 (10%)**
- Algorithm detected: 600 (60%)
- Manual flagged: 200 (20%)
- Both detected: 200 (20%)

**Safe samples: 9,000 (90%)**
- True negatives: 8,500 (85%)
- False positives (algorithm): 300 (3%)
- False positives (manual): 200 (2%)

**Label Strategy:**
```python
# Conservative: Only label as near-miss if BOTH agree
y = (flag_manual & flag_auto).astype(int)

# Moderate: Label as near-miss if EITHER detects
y = (flag_manual | flag_auto).astype(int)

# Aggressive: Weight manual flags higher
y = flag_manual | (flag_auto & (nn_confidence > 0.8))
```

---

## Summary

**How Algorithms Help Training:**

1. **Automatic Labeling**
   - Provide thousands of labeled examples
   - Catch obvious cases you might miss
   - Bootstrap the dataset quickly

2. **Feature Engineering**
   - Show which features matter
   - Provide derived features (min distance, approach rate, etc.)
   - Guide neural network architecture

3. **Safety Net**
   - Catch cases NN misses during training
   - Provide fallback during deployment
   - Reduce false negatives

4. **Validation**
   - Compare NN predictions to algorithm
   - Identify where NN needs improvement
   - Measure NN performance

**The Hybrid Approach:**
- Start with algorithms (Week 1-2)
- Add manual flags (Week 1-6)
- Train NN on combined labels (Week 3)
- Run hybrid detection (Week 4-6)
- Switch to NN-only when ready (Week 7+)

**Key Insight:**
Algorithms are not replaced by NN - they're the **teachers** that help train it! The NN learns from algorithm detections AND your manual corrections, creating a smarter system than either alone.

---

**Document Version:** 1.0  
**Last Updated:** January 27, 2026  
**Status:** ✅ Hybrid strategy documented
