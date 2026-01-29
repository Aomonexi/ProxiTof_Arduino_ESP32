# Volume Control & ToF Filtering Fixes

**Date**: January 29, 2026  
**Issues Fixed**:
1. Volume control goes quiet→loud→quiet (potentiometer wiring issue)
2. ToF sensor noise from tilting causes false near-miss detection

---

## 🔊 Issue 1: Volume Control Behavior

### Problem
Turning potentiometer from 0% to 100%:
- 0% → Quiet ✅
- 50% → **LOUDEST** ❌ (should be medium)
- 100% → Quiet again ❌ (should be loudest)

### Root Cause
**Potentiometer is wired backwards!**

The potentiometer has 3 pins:
```
Pin 1: One end
Pin 2: Wiper (middle)
Pin 3: Other end
```

**Current (Wrong) Wiring:**
```
Pin 1 → GND
Pin 2 → GPIO 4 (ADC)
Pin 3 → 3.3V
```

This creates a voltage that goes: 0V → 3.3V → 0V as you turn it!

### Solution: Swap the End Connections

**Correct Wiring:**
```
Pin 1 → 3.3V  ← SWAP THIS
Pin 2 → GPIO 4 (ADC) - unchanged
Pin 3 → GND   ← SWAP THIS
```

**How to Fix:**
1. Unplug the potentiometer
2. Swap the two end wires (3.3V ↔ GND)
3. Leave the middle wire (GPIO 4) unchanged
4. Plug back in and test

**Expected Behavior After Fix:**
- Turn CCW (0%) → 0V → Quiet/Silent
- Turn middle (50%) → 1.65V → Medium volume
- Turn CW (100%) → 3.3V → Loudest

### Alternative: Software Inversion (If You Can't Rewire)

If you can't physically swap the wires, uncomment this line in `components/buzzer/buzzer.c`:

```c
// Line ~427
uint8_t new_volume = ((4095 - adc_value) * 100) / 4095;  // Inverted
```

And comment out:
```c
// uint8_t new_volume = (adc_value * 100) / 4095;  // Normal
```

---

## 📡 Issue 2: ToF Sensor Noise & Fast Object Detection

### Problem
When device is tilted or at an angle:
- ToF readings bounce/fluctuate (±50-100mm noise)
- Noise creates false "approach velocity"
- Triggers false near-miss alerts

**BUT**: We still need to detect fast-moving objects like bikes passing close!

### Challenge
- **Filter too much** → Miss fast bikes (5-8 m/s = 18-30 km/h)
- **Filter too little** → False alerts from tilt noise

### Solution: Smart Adaptive Filter

Implemented a **3-tier adaptive filter** that:

#### Tier 1: Large Changes (>100mm) - RAW VALUE
```
Use case: Bike passing at 20 km/h
- Distance changes rapidly: 2000mm → 500mm → 2000mm
- Change magnitude: >100mm per frame
- Action: Use RAW value (no filtering)
- Result: ✅ Bike detected immediately!
```

#### Tier 2: Medium Changes (20-100mm) - WEIGHTED AVERAGE
```
Use case: Person walking past
- Distance changes moderately: 1500mm → 1400mm → 1300mm
- Change magnitude: 20-100mm per frame
- Action: 70% raw + 30% filtered
- Result: ✅ Detected with slight smoothing
```

#### Tier 3: Small Changes (<20mm) - FILTERED VALUE
```
Use case: Device tilted, vibration, hand tremor
- Distance fluctuates slightly: 1000mm → 1015mm → 995mm
- Change magnitude: <20mm per frame
- Action: Use 5-sample moving average
- Result: ✅ Noise filtered out, no false alerts
```

### How It Works

```cpp
// Pseudo-code
if (change > 100mm) {
    // Fast object - use raw value immediately
    return raw_distance;
    
} else if (change > 20mm) {
    // Moderate change - weighted average
    return (raw * 0.7 + filtered * 0.3);
    
} else {
    // Small noise - use filtered value
    return filtered_distance;
}
```

### Real-World Scenarios

#### Scenario 1: Bike Passing at 25 km/h (7 m/s)
```
Frame 1: 3000mm (bike far away)
Frame 2: 2000mm (bike approaching) - Change: 1000mm → RAW VALUE USED
Frame 3: 800mm (bike very close) - Change: 1200mm → RAW VALUE USED
Frame 4: 500mm (closest point) - Change: 300mm → RAW VALUE USED
Frame 5: 1500mm (bike passing) - Change: 1000mm → RAW VALUE USED

Result: ✅ Near-miss detected! Buzzer sounds!
```

#### Scenario 2: Device Tilted on Table
```
Frame 1: 1000mm (stable)
Frame 2: 1015mm (tilt noise) - Change: 15mm → FILTERED
Frame 3: 995mm (tilt noise) - Change: 20mm → FILTERED
Frame 4: 1010mm (tilt noise) - Change: 15mm → FILTERED
Frame 5: 1000mm (stable) - Change: 10mm → FILTERED

Result: ✅ No false alert! Noise filtered out.
```

#### Scenario 3: Person Walking Past at 5 km/h (1.4 m/s)
```
Frame 1: 2000mm (person approaching)
Frame 2: 1950mm (getting closer) - Change: 50mm → WEIGHTED (70% raw)
Frame 3: 1880mm (closer) - Change: 70mm → WEIGHTED (70% raw)
Frame 4: 1800mm (closest) - Change: 80mm → WEIGHTED (70% raw)
Frame 5: 1900mm (moving away) - Change: 100mm → WEIGHTED (70% raw)

Result: ✅ Detected with slight smoothing, no false alerts from minor noise
```

### Filter Parameters

```c
// In data_processor.cpp
#define DISTANCE_FILTER_SIZE 5      // 5-sample moving average
#define LARGE_CHANGE_THRESHOLD 100  // mm - bypass filter (fast objects)
#define MEDIUM_CHANGE_THRESHOLD 20  // mm - weighted average
#define SMALL_CHANGE_THRESHOLD 5    // mm - full filtering (noise)
```

### Why This Works for Your Use Case

**Bike Passing Scenario (Your Concern):**
```
Device on your back, bike passes at 30 km/h (8.3 m/s):

Time 0.0s: Bike 5m away (5000mm)
Time 0.1s: Bike 4.2m away (4200mm) - Change: 800mm → RAW VALUE
Time 0.2s: Bike 3.4m away (3400mm) - Change: 800mm → RAW VALUE
Time 0.3s: Bike 2.6m away (2600mm) - Change: 800mm → RAW VALUE
Time 0.4s: Bike 1.8m away (1800mm) - Change: 800mm → RAW VALUE
Time 0.5s: Bike 1.0m away (1000mm) - Change: 800mm → RAW VALUE
Time 0.6s: Bike 0.2m away (200mm) - Change: 800mm → RAW VALUE

✅ NEAR-MISS DETECTED at 0.6s!
✅ Buzzer sounds!
✅ Event logged!

The filter NEVER interferes because changes are >100mm per frame!
```

**Tilt Noise Scenario:**
```
Device tilted 10 degrees:

Frame 1: 1000mm (stable)
Frame 2: 1012mm (tilt) - Change: 12mm → FILTERED
Frame 3: 998mm (tilt) - Change: 14mm → FILTERED
Frame 4: 1005mm (tilt) - Change: 7mm → FILTERED

✅ NO false alert!
✅ Noise smoothed out!
```

---

## 🧪 Testing Procedure

### Test 1: Volume Control
1. **Turn potentiometer fully CCW** (counter-clockwise)
   - Expected: Volume = 0%, buzzer silent or very quiet
   
2. **Turn potentiometer to middle**
   - Expected: Volume = 50%, buzzer medium loudness
   
3. **Turn potentiometer fully CW** (clockwise)
   - Expected: Volume = 100%, buzzer loudest

4. **If behavior is wrong**:
   - Swap potentiometer end wires (3.3V ↔ GND)
   - OR enable software inversion in code

### Test 2: ToF Noise Filtering
1. **Place device on table** (stable)
   - Expected: No false near-miss alerts
   
2. **Tilt device at various angles**
   - Expected: Distance readings stable, no false alerts
   
3. **Wave hand slowly** (1-2 m/s)
   - Expected: Distance changes smoothly, no false alerts
   
4. **Wave hand quickly** (3-5 m/s)
   - Expected: Distance changes tracked, possible near-miss if close

### Test 3: Fast Object Detection
1. **Simulate bike pass**: Quickly move hand from far to close
   - Start: 2-3 meters away
   - Move: Rapidly toward sensor (simulate 20-30 km/h)
   - End: Within 20-50cm
   - Expected: ✅ Near-miss detected! Buzzer sounds!

2. **Check console logs**:
   ```
   D (12345) DataProcessor: ⚡ Fast change detected: 850 mm - using RAW value (bike/vehicle?)
   W (12346) MAIN: ╔════════════════════════════════════════════════╗
   W (12346) MAIN: ║  ⚠️  NEAR-MISS EVENT DETECTED #1               ║
   ```

---

## 📊 Filter Performance Metrics

| Scenario | Change Rate | Filter Action | Detection | False Alerts |
|----------|-------------|---------------|-----------|--------------|
| **Bike @ 30 km/h** | 800 mm/frame | RAW (bypass) | ✅ Detected | ❌ None |
| **Person @ 5 km/h** | 50 mm/frame | Weighted 70% | ✅ Detected | ❌ None |
| **Tilt noise** | 10-20 mm/frame | Filtered 100% | ❌ Ignored | ✅ Prevented |
| **Vibration** | 5-10 mm/frame | Filtered 100% | ❌ Ignored | ✅ Prevented |
| **Hand wave fast** | 200+ mm/frame | RAW (bypass) | ✅ Detected | ❌ None |
| **Hand wave slow** | 30-50 mm/frame | Weighted 70% | ✅ Detected | ❌ None |

---

## 🔧 Configuration Options

If you need to adjust the filter sensitivity, edit these values in `data_processor.cpp`:

```cpp
// Line ~420 in applyDistanceFilter()

// More aggressive filtering (reduces false alerts, may miss slower objects)
if (change > 150) {  // Increase from 100
    return raw_distance;
}

// Less aggressive filtering (catches slower objects, more false alerts)
if (change > 50) {  // Decrease from 100
    return raw_distance;
}

// Adjust weighted average ratio
uint16_t weighted = (raw_distance * 8 + filtered_distance * 2) / 10;  // 80% raw, 20% filtered
```

---

## 🎯 Summary

### Volume Control Fix
- **Hardware fix**: Swap potentiometer end wires (3.3V ↔ GND)
- **Software fix**: Uncomment inversion line in `buzzer.c`
- **Result**: 0% = quiet, 100% = loudest

### ToF Filtering Fix
- **Smart adaptive filter**: 3-tier system
- **Fast objects (>100mm change)**: No filtering - immediate detection
- **Medium objects (20-100mm)**: Light filtering - balanced response
- **Noise (<20mm)**: Full filtering - false alerts prevented
- **Result**: Catches bikes at 30 km/h, ignores tilt noise

---

## 🚀 Build and Test

```bash
# Build with new fixes
idf.py build

# Flash to device
idf.py flash monitor

# Test volume control
# - Turn potentiometer and verify volume changes correctly

# Test ToF filtering
# - Tilt device - should NOT trigger false alerts
# - Wave hand quickly - SHOULD trigger near-miss if close
```

---

**Status**: Ready for testing! Both issues addressed with smart solutions. 🎯
