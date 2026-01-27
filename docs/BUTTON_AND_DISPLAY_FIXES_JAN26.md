# Button and Display Fixes - January 26, 2026

## Issues Fixed

### 1. Button 2 Not Responding ✅

**Problem:** Button 2 (GPIO 3) had a 1-second cooldown between presses, making it feel unresponsive.

**Root Cause:** In `sensor_manager.cpp`, the button detection had:
```cpp
(current_time - last_button_press_time) >= 1000) {  // 1 second between presses
```

**Fix:** Reduced cooldown from 1000ms to 200ms:
```cpp
(current_time - last_button_press_time) >= 200) {  // 200ms between presses
```

**Result:** Button now responds 5x faster while still preventing accidental double-presses.

---

### 2. FLAG Indicator - Inverted Yellow Zone ✅

**Problem:** FLAG indicator was just plain text, hard to see when button pressed.

**User Request:** "Make the yellow zone inverted when button is active (dark text on yellow background)"

**Implementation:**

#### Dashboard Mode:
- **Normal:** Yellow zone shows time, status, GPS, distance (white text on black)
- **Button Pressed:** Entire yellow zone inverts (black text on yellow background)

```
Normal:
┌────────────────────────────────────┐
│ 12:34:56 LOG 3D                    │ ← White on black
│ 1234mm +50mm/s                     │
└────────────────────────────────────┘

Button Pressed:
┌────────────────────────────────────┐
│░12:34:56 LOG 3D░░░░░░░░░░░░░░░░░░░░│ ← Black on yellow (inverted)
│░1234mm +50mm/s░░░░░░░░░░░░░░░░░░░░░│
└────────────────────────────────────┘
```

#### Near Miss Mode:
- **Normal:** Header inverted (alert state)
- **Button Pressed:** Header becomes normal, timestamp inverts (creates "flashing" effect)

**Code Changes:**
- `components/oled_display/oled_display.c` - Lines 440-470 (Dashboard)
- `components/oled_display/oled_display.c` - Lines 669-685 (Near Miss)

---

### 3. Near Miss Mode Manual Flagging Indicator ⚠️

**User Request:** "Near miss mode might have to have extra indicator for manual flagging"

**Current Behavior:**
- Near Miss mode shows inverted header: "!NEAR MISS ALERT!"
- When button pressed, header becomes normal (un-inverted) and timestamp inverts
- This creates a visual "flash" effect

**Question for User:**
Do you want an additional text indicator like "MANUAL FLAG" in the blue zone when button is pressed in Near Miss mode?

**Options:**
1. **Current (flashing header)** - Header flashes when button pressed
2. **Add text line** - Show "MANUAL FLAG" on line 3 when button pressed
3. **Both** - Flashing header + text indicator

---

## About the Fluctuating Speed

**User Question:** "Not sure what is left of the min distance, the speed I guess but why is the speed fluctuating?"

**Answer:** That's the **approach rate** (how fast objects are getting closer/farther):

- **Positive value** (e.g., `+50mm/s`) = Object moving AWAY
- **Negative value** (e.g., `-800mm/s`) = Object APPROACHING (rapid approach!)
- **Fluctuating** = Normal! It changes based on:
  - How fast you're moving
  - How fast objects around you are moving
  - Sensor noise (filtered but still some variation)

**Display Format:**
```
1234mm +50mm/s
  ↑       ↑
  |       └─ Approach rate (positive = moving away)
  └───────── Minimum distance
```

**Why it fluctuates:**
- Calculated from distance change over time: `(current_distance - previous_distance) / time_delta`
- Real-world motion is never perfectly smooth
- Sensor readings have ~1-2mm noise
- This is NORMAL and expected behavior

---

## About the 2-Second Display Freeze

**User Question:** "The number is not continuous, it's like stopping for a bit after 2 seconds, is it the flushing behavior?"

**Investigation Needed:** The emergency flush happens every **10 seconds**, not 2 seconds. Possible causes:

1. **SD card write latency** - Even though flush is every 10s, individual writes might cause brief pauses
2. **OLED update blocking** - Display update might be blocking the main loop
3. **Sensor read timing** - ToF or IMU might have periodic delays

**Current Settings:**
```c
#define EMERGENCY_FLUSH_INTERVAL_MS 10000  // Every 10 seconds
#define FILE_SYNC_INTERVAL_MS 30000        // Every 30 seconds
```

**To Diagnose:**
1. Flash the new firmware
2. Watch the serial monitor for timing logs
3. Note exactly when the freeze happens
4. Check if it correlates with:
   - "🔄 Periodic flush" messages (every 10s)
   - Any error messages
   - Specific display modes

**Potential Fixes (if needed):**
- Move SD card writes to separate task (non-blocking)
- Reduce OLED update frequency in certain modes
- Optimize differential rendering (already implemented)
- Increase buffer size to reduce flush frequency

---

## Testing Checklist

### Button Responsiveness
- [ ] Press Button 2 in Dashboard mode - LED turns blue immediately
- [ ] Yellow zone inverts when button pressed
- [ ] Release button - LED turns off, yellow zone returns to normal
- [ ] Press button multiple times quickly - should respond each time (200ms cooldown)
- [ ] Hold button - LED stays blue, yellow zone stays inverted

### WiFi Settings Mode
- [ ] Switch to WiFi Settings mode (Mode 6)
- [ ] Press Button 2 - should show "Connecting..." message
- [ ] WiFi sync should trigger
- [ ] LED turns blue during sync
- [ ] Display shows "Sync SUCCESS!" or "Sync FAILED"

### Near Miss Mode
- [ ] Trigger near-miss event (get close to object quickly)
- [ ] Mode switches to Near Miss automatically
- [ ] Press Button 2 - header should "flash" (invert/uninvert)
- [ ] Timestamp line should invert when button pressed

### Display Freeze Investigation
- [ ] Watch display in Dashboard mode
- [ ] Note if numbers freeze periodically
- [ ] Check serial monitor for timing of freezes
- [ ] Correlate with flush messages

---

## Files Modified

1. **components/sensor_manager/sensor_manager.cpp**
   - Line ~730: Reduced button cooldown from 1000ms to 200ms

2. **components/oled_display/oled_display.c**
   - Lines 440-470: Dashboard mode - invert yellow zone when button pressed
   - Lines 669-685: Near Miss mode - flashing header when button pressed

---

## Next Steps

1. **Flash firmware** to device
2. **Test button responsiveness** in all modes
3. **Investigate 2-second freeze** - check serial logs
4. **Decide on Near Miss indicator** - do you want additional text?

---

## Build Info

- **Build Date:** January 26, 2026
- **Commit:** 5276e26-dirty
- **Binary Size:** 0x11c840 bytes (1,165,376 bytes)
- **Free Space:** 26% (409,536 bytes)
- **Build Status:** ✅ SUCCESS

---

## Questions for User

1. **Near Miss Mode:** Do you want an additional "MANUAL FLAG" text indicator, or is the flashing header enough?

2. **Display Freeze:** After flashing, can you confirm:
   - Does the freeze still happen?
   - How often (every 2 seconds, 10 seconds, random)?
   - Does it happen in all modes or just Dashboard?

3. **Approach Rate:** Is the fluctuating speed display okay, or would you prefer:
   - Smoothed/averaged value (less responsive but steadier)
   - Hide it when value is small (< 10mm/s)
   - Different format/units

---

## Performance Notes

**Button Check Rate:** 50ms (20 Hz) - Very responsive
**OLED Update Rate:** 100ms (10 Hz) - Smooth for human perception
**Sensor Read Rate:** 
- ToF: 60 Hz (16.7ms)
- IMU: 100 Hz (10ms)

**Memory Usage:**
- Frame buffer: 1024 bytes (differential rendering)
- Circular buffer: 1000 entries × ~100 bytes = ~100KB
- Total RAM: ~150KB used (plenty of headroom)
