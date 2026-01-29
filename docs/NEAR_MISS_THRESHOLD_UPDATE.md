# Near-Miss Detection Threshold Update

**Date**: January 28, 2026  
**Issue**: False positives - alerting on static objects without movement  
**Solution**: Stricter thresholds + distance requirement for combined detection

---

## Problem

Near-miss alerts were triggering on:
- Static objects that are just close (no movement)
- Slow approaches that aren't dangerous
- Normal cycling movements (low jerk)

**Root cause**: Thresholds were too sensitive for real-world cycling scenarios.

---

## Solution Implemented

### 1. Minimum Valid Distance Filter (NEW)

**Problem**: Device on table triggers false positives (distance = 0mm)

**Solution**: Filter out readings < 20mm (2cm)

| Parameter | Value | Reason |
|-----------|-------|--------|
| **Minimum Valid Distance** | **20 mm (2 cm)** | Filters sensor blockage, device on table, covered sensor |

**Why 20mm?**
- Distance = 0mm → Sensor blocked or facing surface directly
- Distance < 20mm → Too close to be valid near-miss (sensor covered)
- Distance ≥ 20mm → Valid near-miss zone starts

### 2. Increased Thresholds (Stricter)

| Parameter | Old Value | New Value | Change |
|-----------|-----------|-----------|--------|
| **Minimum Valid Distance** | N/A | **20 mm (2 cm)** | New - filters blockage |
| **Rapid Approach** | 500 mm/s (0.5 m/s) | **1000 mm/s (1.0 m/s)** | 2x stricter |
| **High Jerk** | 2.0 m/s³ | **7.0 m/s³** | 3.5x stricter |
| **Critical Proximity Distance** | 300 mm (30 cm) | **100 mm (10 cm)** | 3x stricter |
| **Critical Proximity Approach** | 200 mm/s (0.2 m/s) | **500 mm/s (0.5 m/s)** | 2.5x stricter |
| **Combined Detection Distance** | N/A | **150 mm (15 cm)** | New - near-miss zone |

### 2. Added Distance Requirement for Combined Detection

**New rule**: Combined near-miss only triggers if distance < 500mm (50cm)

**Before**:
```
Combined = Rapid approach + High jerk (within 500ms)
```

**After**:
```
Combined = Distance < 500mm AND Rapid approach + High jerk (within 500ms)
```

### 3. Jerk Filtering (Already Implemented)

Jerk is already filtered in the data processor to reduce IMU noise.

---

## New Detection Logic

### Critical Proximity (Immediate Danger)
```
Trigger when:
  Distance 20-100mm (2-10cm) AND
  Approaching > 500mm/s (0.5m/s)

Example: Object suddenly appears at 5cm away while moving
Note: Distance < 20mm filtered out (sensor blockage)
```

### Combined Near-Miss (Cycling Scenario - Priority-Based)
```
PRIORITY SYSTEM (Highest to Lowest):

Validity Check (FIRST):
  Distance >= 20mm (filters sensor blockage)
  → If < 20mm, NO ALERT (device on table, sensor covered)

Priority 1 - Fast Approach + Close Distance:
  Distance 20-150mm (2-15cm) AND
  Rapid approach > 1000mm/s (1.0m/s)
  → TRIGGERS NEAR-MISS (NO JERK REQUIRED)
  
Priority 2 - Body Reaction (Optional Enhancement):
  Priority 1 triggered AND
  High jerk > 7m/s³ (body reaction)
  → Confirms near-miss severity for training data

Example Scenarios:
1. Device on table (0mm) → NO ALERT ❌ (filtered)
2. Motorcycle passes at 15cm, fast approach, NO reaction → NEAR-MISS ✅
3. Motorcycle passes at 15cm, fast approach, WITH swerve → NEAR-MISS ✅ (higher severity)
4. Motorcycle passes at 15cm, slow approach, WITH swerve → NO ALERT ❌ (not fast enough)
```

**Key Insight**: Fast approach + close distance is the **highest priority**. Jerk is optional and only used to confirm severity. Readings < 20mm are filtered to prevent false positives from sensor blockage.

---

## Why These Values?

### Rapid Approach: 1000mm/s (1.0 m/s)

- **Old**: 500mm/s = slow walking speed (false positives)
- **New**: 1000mm/s = fast approach (real danger)
- **Rationale**: Cycling speeds are 15-30 km/h (4-8 m/s), so 1 m/s relative approach is significant

### High Jerk: 7.0 m/s³

- **Old**: 2.0 m/s³ = normal movement (IMU noise)
- **New**: 7.0 m/s³ = sudden reaction (real event)
- **Rationale**: Filters out daily vibrations while catching true near-miss reactions

**Daily Vibrations (Filtered Out)**:
- **Turning**: Gradual angular velocity change (jerk < 3m/s³)
- **Bumpy road**: High-frequency but filtered (jerk < 5m/s³)
- **Walking/pedaling**: Periodic pattern (jerk < 5m/s³)
- **Near-miss reaction**: Sudden swerve (jerk > 7m/s³) ✅

### Critical Proximity: 100mm + 500mm/s

- **Old**: 300mm + 200mm/s = triggers on slow approaches
- **New**: 100mm + 500mm/s = truly dangerous situations
- **Rationale**: 10cm is genuinely close, and 0.5m/s approach is fast at that distance

### Combined Detection Distance: 150mm (15cm)

- **New**: Must be within 15cm for combined detection
- **Rationale**: Real near-miss zone for cycling
- **Scenario**: Motorcycle passes at 15cm → body hasn't reacted yet → approach detected → body reacts with swerve → jerk confirms near-miss

**Two-Phase Detection**:
1. **Phase 1**: Fast approach at 15cm (early warning)
2. **Phase 2**: Body reaction creates jerk (confirmation)

---

## Expected Behavior

### Will Trigger ✅

1. **Motorcycle passes close, fast approach, NO reaction** (Late/No Reaction):
   - Distance: 140mm (14cm)
   - Approach: 1500mm/s (fast)
   - Jerk: 2m/s³ (no reaction)
   - Result: Near-miss ✅ (Priority 1: Fast + Close)

2. **Motorcycle passes close, fast approach, WITH swerve** (Confirmed):
   - Distance: 140mm (14cm)
   - Approach: 1500mm/s (fast)
   - Jerk: 8m/s³ (body reaction)
   - Result: Near-miss ✅ (Priority 1 + Priority 2: Higher severity)

3. **Sudden obstacle at close range**:
   - Distance: 80mm (8cm)
   - Approach: 600mm/s
   - Result: Critical proximity ✅ (Tier 1)

4. **Close call with pedestrian**:
   - Distance: 120mm (12cm)
   - Approach: 1200mm/s
   - Jerk: 6m/s³
   - Result: Near-miss ✅ (Priority 1: Fast + Close)

### Will NOT Trigger ❌

1. **Device on table** (FIXED):
   - Distance: 0mm (sensor blocked)
   - Approach: -1647mm/s (noise)
   - Jerk: 18m/s³ (movement)
   - Result: No alert ❌ (correct! - filtered by minimum distance)

2. **Sensor covered**:
   - Distance: 5mm (hand covering sensor)
   - Approach: -500mm/s
   - Result: No alert ❌ (correct! - below 20mm threshold)

3. **Static object nearby**:
   - Distance: 150mm
   - Approach: 0mm/s (static)
   - Result: No alert ❌ (correct! - no movement)

4. **Slow approach to wall**:
   - Distance: 120mm (12cm)
   - Approach: 300mm/s (slow)
   - Jerk: 8m/s³ (reaction)
   - Result: No alert ❌ (correct! - not fast enough, even with jerk)

5. **Far object with fast movement**:
   - Distance: 800mm
   - Approach: 1500mm/s
   - Jerk: 7m/s³
   - Result: No alert ❌ (correct! - too far, outside 15cm zone)

6. **Normal cycling vibration**:
   - Distance: 400mm
   - Approach: 500mm/s
   - Jerk: 3m/s³ (below threshold)
   - Result: No alert ❌ (correct! - filtered vibration)

7. **Close but slow approach with reaction**:
   - Distance: 140mm (14cm)
   - Approach: 800mm/s (below 1000mm/s threshold)
   - Jerk: 9m/s³ (strong reaction)
   - Result: No alert ❌ (correct! - approach not fast enough)

---

## Files Modified

1. **main/main.c**:
   - Updated threshold definitions
   - Updated logging messages

2. **components/data_processor/data_processor.cpp**:
   - Updated `isCriticalProximity()` - stricter thresholds
   - Updated `isRapidApproach()` - 1000mm/s default
   - Updated `isHighJerk()` - 5.0m/s³ default
   - Updated `detectNearMiss()` - added distance check for combined detection

---

## Testing Recommendations

### Test 1: Static Object
- Place hand 15cm from sensor
- Keep hand still
- **Expected**: No alert (distance alone doesn't trigger)

### Test 2: Slow Approach
- Move hand slowly toward sensor (< 1m/s)
- **Expected**: No alert until very close (<10cm) and fast (>0.5m/s)

### Test 3: Fast Approach
- Move hand quickly toward sensor (> 1m/s)
- With sudden movement (jerk)
- **Expected**: Alert when <50cm with movement

### Test 4: Cycling Simulation
- Mount on bike
- Ride normally
- **Expected**: No false alerts from normal vibrations

### Test 5: Near-Miss Simulation
- Have someone walk close (<50cm) while you swerve
- **Expected**: Alert triggers

---

## Tuning Guide

If you still get false positives, increase thresholds further:

```c
// Even stricter (if needed)
#define RAPID_APPROACH_THRESHOLD 1500.0f   // 1.5 m/s
#define HIGH_JERK_THRESHOLD 7.0f           // 7 m/s³
#define CRITICAL_PROXIMITY_DISTANCE 80     // 8 cm
```

If you miss real near-misses, decrease thresholds:

```c
// More sensitive (if needed)
#define RAPID_APPROACH_THRESHOLD 800.0f    // 0.8 m/s
#define HIGH_JERK_THRESHOLD 4.0f           // 4 m/s³
#define CRITICAL_PROXIMITY_DISTANCE 150    // 15 cm
```

---

## Summary

✅ **Priority-based detection** - Fast approach + close distance is highest priority  
✅ **NO jerk required** - Handles late/no reaction scenarios (real near-misses)  
✅ **Jerk is optional** - Only used to confirm severity for training data  
✅ **Increased thresholds** - 2-3x stricter to reduce false positives  
✅ **15cm near-miss zone** - Critical distance for cycling safety  
✅ **Jerk filtering** - Filters out turning, bumpy roads, and pedaling (>7m/s³)  
✅ **Better logging** - Shows new thresholds at startup  

**Result**: Near-miss detection prioritizes **fast approach + close distance** over body reaction. This correctly identifies near-misses even when the cyclist doesn't react in time.

**Priority System**:
1. **Highest**: Fast approach (<15cm, >1m/s) → TRIGGERS NEAR-MISS
2. **Optional**: Body reaction (jerk >7m/s³) → Confirms severity

---

**Status**: Ready for testing  
**Next**: Build, flash, and test with real cycling scenarios
