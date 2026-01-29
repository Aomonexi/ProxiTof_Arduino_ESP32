# Quick Fix Guide - Volume & ToF Issues

## 🔊 Volume Control: Quiet→Loud→Quiet Problem

**QUICK FIX**: Swap the potentiometer end wires!

```
BEFORE (Wrong):
Pot Pin 1 → GND
Pot Pin 2 → GPIO 4 (middle pin - don't touch!)
Pot Pin 3 → 3.3V

AFTER (Correct):
Pot Pin 1 → 3.3V  ← SWAP
Pot Pin 2 → GPIO 4 (middle pin - unchanged)
Pot Pin 3 → GND   ← SWAP
```

**Test**: Turn pot from left to right → volume should go from quiet to loud (not loud to quiet to loud)

---

## 📡 ToF Sensor: Smart Filtering Explained

**Problem**: Tilting device causes false near-miss alerts  
**Solution**: Smart 3-tier adaptive filter

### How It Works

```
┌─────────────────────────────────────────────────┐
│  Distance Change  │  Filter Action  │  Result   │
├─────────────────────────────────────────────────┤
│  >100mm (LARGE)   │  NO FILTER      │  Fast     │
│  Bike @ 30 km/h   │  Use RAW value  │  objects  │
│                   │                 │  detected │
├─────────────────────────────────────────────────┤
│  20-100mm (MED)   │  LIGHT FILTER   │  Walking  │
│  Person walking   │  70% raw + 30%  │  detected │
│                   │  filtered       │           │
├─────────────────────────────────────────────────┤
│  <20mm (SMALL)    │  FULL FILTER    │  Noise    │
│  Tilt/vibration   │  Moving average │  removed  │
└─────────────────────────────────────────────────┘
```

### Real Example: Bike Passing

```
Bike at 30 km/h = 8.3 m/s = 830 mm per 100ms frame

Frame 1: 5000mm (bike far)
Frame 2: 4170mm (approaching) → Change: 830mm → RAW VALUE USED ✅
Frame 3: 3340mm (closer)      → Change: 830mm → RAW VALUE USED ✅
Frame 4: 2510mm (very close)  → Change: 830mm → RAW VALUE USED ✅
Frame 5: 1680mm (closest)     → Change: 830mm → RAW VALUE USED ✅
Frame 6: 850mm (passing)      → Change: 830mm → RAW VALUE USED ✅

RESULT: ✅ NEAR-MISS DETECTED! Buzzer sounds!
```

### Real Example: Device Tilted

```
Device tilted 10 degrees on table

Frame 1: 1000mm (stable)
Frame 2: 1012mm (tilt noise) → Change: 12mm → FILTERED ✅
Frame 3: 998mm (tilt noise)  → Change: 14mm → FILTERED ✅
Frame 4: 1005mm (tilt noise) → Change: 7mm  → FILTERED ✅
Frame 5: 1000mm (stable)     → Change: 5mm  → FILTERED ✅

RESULT: ✅ NO FALSE ALERT! Noise filtered out.
```

---

## 🧪 Quick Test Procedure

### 1. Volume Test (30 seconds)
```
1. Turn pot fully left  → Should be quiet/silent
2. Turn pot to middle   → Should be medium volume
3. Turn pot fully right → Should be LOUDEST
4. If wrong: Swap pot end wires (3.3V ↔ GND)
```

### 2. Tilt Test (1 minute)
```
1. Place device on table → No alerts
2. Tilt device 10-20°    → No alerts (noise filtered)
3. Tilt device 45°       → No alerts (noise filtered)
4. Return to flat        → No alerts
✅ PASS: No false alerts from tilting
```

### 3. Fast Object Test (1 minute)
```
1. Hold hand 2m away
2. Quickly move hand toward sensor (simulate bike)
3. Stop at 20-50cm distance
4. Should hear: 3 beeps (near-miss alert)
✅ PASS: Fast object detected
```

---

## 📊 What Changed in Code

### Volume Control (`components/buzzer/buzzer.c`)
```c
// Line ~427
// Normal reading (assumes correct wiring)
uint8_t new_volume = (adc_value * 100) / 4095;

// If wiring is backwards, uncomment this instead:
// uint8_t new_volume = ((4095 - adc_value) * 100) / 4095;
```

### ToF Filtering (`components/data_processor/data_processor.cpp`)
```c
// Added smart adaptive filter
uint16_t applyDistanceFilter(uint16_t raw_distance) {
    // Calculate change from moving average
    int change = abs(raw - filtered);
    
    if (change > 100mm)  return raw;           // Fast object
    if (change > 20mm)   return weighted;      // Medium object
    else                 return filtered;      // Noise
}
```

---

## 🎯 Expected Results

### ✅ Volume Control
- 0% position: Quiet/silent
- 50% position: Medium volume
- 100% position: LOUDEST

### ✅ ToF Filtering
- Tilt device: No false alerts
- Fast hand wave: Near-miss detected
- Bike passing: Near-miss detected
- Vibration: No false alerts

---

## 🚀 Flash and Test

```bash
idf.py flash monitor
```

**Watch for:**
- Volume changes smoothly from 0% to 100%
- No false alerts when tilting device
- Fast objects still trigger near-miss

---

**Build Status**: ✅ Compiled successfully  
**Ready to Flash**: Yes  
**Estimated Test Time**: 5 minutes
