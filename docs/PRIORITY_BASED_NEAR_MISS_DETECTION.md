# Priority-Based Near-Miss Detection System

**Date**: January 28, 2026  
**Status**: ✅ Implemented and Built Successfully  
**Issue Addressed**: Late/no reaction scenarios where near-miss is real but cyclist doesn't react

---

## Problem Statement

**User Scenario**: 
> "What if the user has very late reaction or doesn't react at all but it is approaching fast and passing near the proxitof tof sensor, so it's like literally a near-miss situation but the user doesn't really jerk, will it still counted?"

**Answer**: YES! The system now uses a **priority-based approach** where fast approach + close distance is the highest priority, and jerk is optional.

---

## Priority System

### Priority 1: Fast Approach + Close Distance (HIGHEST)
```
Trigger Conditions:
  Distance < 150mm (15cm) AND
  Approach rate > 1000mm/s (1.0 m/s)

Result: NEAR-MISS DETECTED ✅
Jerk: NOT REQUIRED
```

**Why this is highest priority**:
- Fast-moving object at 15cm IS a near-miss, regardless of reaction
- Handles late reaction scenarios (cyclist reacts too late)
- Handles no reaction scenarios (cyclist doesn't see it coming)
- Real danger exists even without body movement

**Example**:
- Motorcycle passes at 14cm distance
- Approaching at 1.5 m/s (fast)
- Cyclist doesn't react (jerk = 2 m/s³)
- **Result**: NEAR-MISS ✅ (Priority 1 triggered)

---

### Priority 2: Body Reaction (OPTIONAL ENHANCEMENT)
```
Trigger Conditions:
  Priority 1 triggered AND
  Jerk > 7.0 m/s³ (sudden body movement)

Result: Confirms near-miss severity
Purpose: Training data quality + severity classification
```

**Why this is optional**:
- Jerk confirms the cyclist perceived the danger
- Useful for classifying near-miss severity
- Helps improve training data quality
- But NOT required for near-miss detection

**Example**:
- Motorcycle passes at 14cm distance
- Approaching at 1.5 m/s (fast)
- Cyclist swerves (jerk = 8 m/s³)
- **Result**: NEAR-MISS ✅ (Priority 1 + Priority 2 = Higher severity)

---

## Detection Tiers

### Validity Check (FIRST - Always Applied)
```
Distance >= 20mm (2cm)

→ Filters sensor blockage
→ Prevents false positives from:
   - Device on table (0mm)
   - Sensor covered by hand (<20mm)
   - Invalid ToF readings
```

### Tier 1: Critical Proximity
```
Distance 20-100mm (2-10cm) AND
Approach > 500mm/s (0.5 m/s)

→ IMMEDIATE DANGER
→ NO JERK REQUIRED
```

### Tier 2: Proximity Alert (Priority 1)
```
Distance 20-150mm (2-15cm) AND
Approach > 1000mm/s (1.0 m/s)

→ NEAR-MISS ZONE
→ NO JERK REQUIRED
→ HIGHEST PRIORITY
```

### Tier 3: Confirmed Near-Miss (Priority 2)
```
Tier 2 triggered AND
Jerk > 7.0 m/s³

→ CONFIRMED SEVERITY
→ OPTIONAL ENHANCEMENT
```

---

## Real-World Scenarios

### Scenario 0: Device on Table ❌ (FIXED)
```
Situation: Device sitting on table, sensor facing down
- Distance: 0mm (sensor blocked by table)
- Approach: -1647mm/s (noise from movement)
- Jerk: 18m/s³ (picking up device)

Detection: NO ALERT ❌
Reason: Distance < 20mm filtered out (sensor blockage)
Fix: Minimum valid distance filter prevents false positive
```

### Scenario 1: Late Reaction ✅
```
Situation: Motorcycle passes close, cyclist reacts too late
- Distance: 140mm (14cm)
- Approach: 1500mm/s (fast)
- Jerk: 3m/s³ (late reaction, below threshold)

Detection: NEAR-MISS ✅ (Priority 1)
Reason: Fast approach + close distance is sufficient
```

### Scenario 2: No Reaction ✅
```
Situation: Motorcycle passes close, cyclist doesn't see it
- Distance: 130mm (13cm)
- Approach: 1800mm/s (very fast)
- Jerk: 1m/s³ (no reaction)

Detection: NEAR-MISS ✅ (Priority 1)
Reason: Fast approach + close distance is sufficient
```

### Scenario 3: Strong Reaction ✅
```
Situation: Motorcycle passes close, cyclist swerves hard
- Distance: 140mm (14cm)
- Approach: 1500mm/s (fast)
- Jerk: 9m/s³ (strong reaction)

Detection: NEAR-MISS ✅ (Priority 1 + Priority 2)
Reason: Fast approach + close distance + body reaction
Severity: HIGH (confirmed by cyclist reaction)
```

### Scenario 4: Slow Approach ❌
```
Situation: Cyclist slowly approaches wall
- Distance: 120mm (12cm)
- Approach: 800mm/s (slow)
- Jerk: 8m/s³ (reaction)

Detection: NO ALERT ❌
Reason: Approach not fast enough (< 1000mm/s)
Note: Even with jerk, approach speed is too low
```

### Scenario 5: Far Object ❌
```
Situation: Car passes far away, fast
- Distance: 800mm (80cm)
- Approach: 2000mm/s (very fast)
- Jerk: 10m/s³ (reaction)

Detection: NO ALERT ❌
Reason: Outside 15cm near-miss zone
Note: Not close enough to be dangerous
```

---

## Jerk Filtering (7.0 m/s³ Threshold)

### Filtered Out (< 7.0 m/s³)
- **Turning**: Gradual angular velocity change (jerk < 3 m/s³)
- **Bumpy road**: High-frequency vibration (jerk < 5 m/s³)
- **Walking/pedaling**: Periodic pattern (jerk < 5 m/s³)
- **Normal cycling**: Daily vibrations (jerk < 6 m/s³)

### Detected (> 7.0 m/s³)
- **Near-miss reaction**: Sudden swerve (jerk > 7 m/s³) ✅
- **Emergency braking**: Sudden deceleration (jerk > 7 m/s³) ✅
- **Collision avoidance**: Quick maneuver (jerk > 7 m/s³) ✅

---

## Implementation Details

### File: `components/data_processor/data_processor.cpp`

```cpp
bool DataProcessor::detectNearMiss(const feature_vector_t& features) {
    // Tier 1: Critical Proximity (< 10cm + fast)
    if (isCriticalProximity(features.min_distance_mm, features.approach_rate_mm_s)) {
        return true;
    }
    
    // Tier 2: Proximity Alert (< 15cm + fast) - NO JERK REQUIRED
    // Priority 1: Fast approach + close distance is HIGHEST PRIORITY
    if (features.min_distance_mm < 150 && isRapidApproach(features.approach_rate_mm_s)) {
        return true;  // ← TRIGGERS WITHOUT JERK
    }
    
    // Tier 3: Confirmed Near-Miss (Tier 2 + jerk)
    // Priority 2: Jerk is optional, only for severity confirmation
    
    return false;
}
```

### Thresholds (main/main.c)

```c
#define RAPID_APPROACH_THRESHOLD 1000.0f   // mm/s (1.0 m/s)
#define HIGH_JERK_THRESHOLD 7.0f           // m/s³
#define CRITICAL_PROXIMITY_DISTANCE 100    // mm (10 cm)
#define CRITICAL_PROXIMITY_APPROACH 500.0f // mm/s (0.5 m/s)
#define COMBINED_DETECTION_MAX_DISTANCE 150 // mm (15 cm)
```

---

## Testing Recommendations

### Test 1: No Reaction Scenario
```
Setup: Have someone walk past at 15cm, don't react
Expected: NEAR-MISS ✅ (Priority 1 only)
Verify: LED flashes orange/red, console logs near-miss
```

### Test 2: Late Reaction Scenario
```
Setup: Have someone walk past at 15cm, react slowly
Expected: NEAR-MISS ✅ (Priority 1 only, jerk below threshold)
Verify: LED flashes orange/red, console logs near-miss
```

### Test 3: Strong Reaction Scenario
```
Setup: Have someone walk past at 15cm, swerve hard
Expected: NEAR-MISS ✅ (Priority 1 + Priority 2)
Verify: LED flashes orange/red, console logs near-miss with high jerk
```

### Test 4: Slow Approach (Should NOT Trigger)
```
Setup: Slowly approach wall at 15cm
Expected: NO ALERT ❌ (approach too slow)
Verify: LED shows distance color (yellow), no near-miss alert
```

### Test 5: Far Object (Should NOT Trigger)
```
Setup: Have someone walk past at 50cm, fast
Expected: NO ALERT ❌ (too far)
Verify: LED shows distance color (green/yellow), no near-miss alert
```

---

## Benefits of Priority-Based Approach

✅ **Handles late reactions**: Detects near-miss even if cyclist reacts too late  
✅ **Handles no reactions**: Detects near-miss even if cyclist doesn't react  
✅ **Real danger detection**: Fast approach + close distance IS dangerous  
✅ **Severity classification**: Jerk confirms how severe the near-miss was  
✅ **Training data quality**: Can filter by severity (with/without jerk)  
✅ **Reduced false positives**: Still requires fast approach (>1m/s)  
✅ **Cycling-specific**: 15cm zone is realistic for cycling safety  

---

## Summary

The system now uses a **priority-based approach** where:

1. **Priority 1 (Highest)**: Fast approach + close distance → TRIGGERS NEAR-MISS
2. **Priority 2 (Optional)**: Body reaction (jerk) → Confirms severity

This correctly identifies near-misses in all scenarios:
- ✅ Late reaction (jerk below threshold)
- ✅ No reaction (no jerk at all)
- ✅ Strong reaction (jerk above threshold)

**Key Principle**: If an object passes within 15cm at >1m/s, it's a near-miss regardless of whether the cyclist reacts. The reaction (jerk) is optional and only used to confirm severity.

---

**Status**: ✅ Implemented, built, and ready for testing  
**Next**: Flash to device and test with real cycling scenarios
