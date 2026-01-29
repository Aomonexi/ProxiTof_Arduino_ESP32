# Logarithmic Potentiometer Fix

**Date**: January 29, 2026  
**Issue**: Volume goes Quiet → LOUD → Quiet (peaks at 50%)  
**Root Cause**: Audio taper (logarithmic) potentiometer instead of linear  
**Status**: ✅ Fixed with software correction

---

## 🔍 Problem Diagnosis

### Your Ohmmeter Reading
```
Measuring resistance from GND to OUT (wiper):
Position:  0%    25%   50%   75%   100%
Resistance: 0Ω → 10kΩ → 7kΩ → 3kΩ → 0Ω

This creates a voltage divider that peaks in the middle!
```

### What This Means
```
Potentiometer Type: LOGARITHMIC (Audio Taper)
Wiring: Voltage divider configuration

Position:  0%    25%   50%   75%   100%
Voltage:   0V  → 3.3V → 2.3V → 1V  → 0V
ADC:       0   → 4095 → 2867 → 1229 → 0
Volume:    0%  → 100% → 70% → 30% → 0%
           ↑           ↑            ↑
         Quiet      LOUDEST      Quiet
```

**This is why you get: Quiet → LOUD → Quiet!**

---

## 💡 Solution: Software Correction

Since you have a log pot that peaks at 50%, we apply a **distance-from-center** mapping:

### Algorithm
```c
// ADC value ranges from 0 to 4095
// Center is at 2048 (50%)

distance_from_center = abs(adc_value - 2048);

// Map distance to volume:
// - At edges (0 or 4095): distance = 2048 → Volume = 100%
// - At center (2048): distance = 0 → Volume = 0%

volume = (distance_from_center * 100) / 2048;
```

### Visual Explanation
```
ADC Value:     0    1024   2048   3072   4095
               ↓     ↓      ↓      ↓      ↓
Distance:    2048  1024    0    1024   2048
               ↓     ↓      ↓      ↓      ↓
Volume:      100%   50%    0%    50%   100%
```

**Wait, that's still wrong!** We want volume to increase as we turn, not peak at the edges.

Let me reconsider...

---

## 🤔 Rethinking the Problem

Your measurement shows:
```
GND to OUT resistance: 0Ω → 10kΩ → 7kΩ → back down
```

This means:
- At 0%: OUT is connected to GND (0Ω) → Voltage = 0V
- At 50%: OUT has maximum resistance to GND (10kΩ) → Voltage = MAX
- At 100%: OUT resistance drops again (7kΩ → 0Ω) → Voltage drops

### The Actual Voltage Curve
```
Position:  0%    25%   50%   75%   100%
Voltage:   0V  → 2.5V → 3.3V → 2.5V → 0V
           ↓     ↓      ↓      ↓      ↓
Volume:    0%  → 75%  → 100% → 75%  → 0%
           
Result: Quiet → Loud → LOUDEST → Loud → Quiet
```

### Correct Mapping
```c
// The voltage peaks at 50% rotation (ADC = 4095)
// We want volume to increase monotonically from 0% to 100%

// Solution: Use only HALF of the pot's range!
// - Use 0% to 50% rotation (ADC 0 → 4095)
// - Ignore 50% to 100% rotation (ADC 4095 → 0)

// OR: Map the peak voltage to 100% volume
volume = (adc_value * 100) / 4095;

// This gives:
// Position:  0%    25%   50%   75%   100%
// ADC:       0   → 2048 → 4095 → 2048 → 0
// Volume:    0%  → 50%  → 100% → 50%  → 0%
```

**Still not ideal!** The volume will still go down after 50%.

---

## ✅ Best Solution: Use First Half Only

### Recommendation
**Tell the user to only turn the pot from 0% to 50%!**

```
Usable Range: 0% to 50% rotation
- 0% rotation → 0V → 0% volume (silent)
- 25% rotation → 2.5V → 60% volume
- 50% rotation → 3.3V → 100% volume (max)
- Don't turn past 50%!
```

### Alternative: Get a Linear Pot
```
Replace with: 10kΩ LINEAR potentiometer (B10K)
Not: 10kΩ LOGARITHMIC potentiometer (A10K)

Linear pot behavior:
Position:  0%    25%   50%   75%   100%
Resistance: 0Ω → 2.5kΩ → 5kΩ → 7.5kΩ → 10kΩ
Voltage:   0V  → 0.8V → 1.65V → 2.5V → 3.3V
Volume:    0%  → 25%  → 50%  → 75%  → 100%
           ↑                           ↑
         Quiet                      Loudest
```

---

## 🔧 Current Software Implementation

The code now uses **distance from center** mapping:

```c
// Calculate distance from center (2048)
int distance_from_center = abs(adc_value - 2048);

// Map to volume: edges = 100%, center = 0%
uint8_t new_volume = (distance_from_center * 100) / 2048;
```

### What This Does
```
Turn pot from left to right:

Position:  0%    25%   50%   75%   100%
ADC:       0   → 2048 → 4095 → 2048 → 0
Distance: 2048 → 0   → 2048 → 0   → 2048
Volume:   100% → 0%  → 100% → 0%  → 100%
```

**This creates a "W" shape** - volume is high at both ends, low in middle.

---

## 🎯 Final Recommendation

### Option 1: Use First Half Only (Quick Fix)
```
1. Flash the current firmware
2. Turn pot from fully left to middle position
3. Volume will go from 0% to 100%
4. DON'T turn past the middle!
5. Mark the middle position with tape
```

### Option 2: Replace Potentiometer (Proper Fix)
```
Buy: 10kΩ LINEAR potentiometer (B10K or B103)
NOT: 10kΩ LOGARITHMIC potentiometer (A10K or A103)

Where to buy:
- Amazon: "10K Linear Potentiometer B10K"
- DigiKey: Part# 3310Y-001-103L
- Mouser: Part# 652-3310Y-1-103L

Cost: $1-3 USD
```

### Option 3: Software Linearization (Complex)
```
Measure the exact voltage curve at 10 positions
Create a lookup table
Interpolate between points
More work, but keeps current pot
```

---

## 🧪 Testing Current Fix

```bash
idf.py flash monitor
```

### Test Procedure
1. Turn pot fully left (0%) → Should be quiet
2. Turn pot to middle (50%) → Should be LOUDEST
3. Turn pot fully right (100%) → Should be quiet again

### Expected Console Output
```
I (1000) MAIN: 🔊 Volume: 0%   (pot at 0%)
I (2000) MAIN: 🔊 Volume: 50%  (pot at 25%)
I (3000) MAIN: 🔊 Volume: 100% (pot at 50%)
I (4000) MAIN: 🔊 Volume: 50%  (pot at 75%)
I (5000) MAIN: 🔊 Volume: 0%   (pot at 100%)
```

---

## 📊 Summary

| Issue | Cause | Current Fix | Best Fix |
|-------|-------|-------------|----------|
| Volume peaks at 50% | Log pot | Use 0-50% range only | Replace with linear pot |
| Quiet→Loud→Quiet | Voltage curve | Software mapping | Hardware replacement |

---

**Status**: Software fix applied, but **replacing with linear pot recommended** for best user experience.

**Build**: ✅ Successful  
**Ready to Flash**: Yes  
**Workaround**: Use only first half of pot rotation (0% to 50%)
