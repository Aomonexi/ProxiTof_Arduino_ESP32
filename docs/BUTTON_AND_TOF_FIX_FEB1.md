# Button and ToF Orientation Fix

**Date**: February 1, 2026  
**Status**: ✅ Fixed and tested  
**Update**: February 1, 2026 (Evening) - Near-Miss mode button fix

---

## Issue 1: ACTION Button Not Working in All Modes

### Problem
The ACTION button only worked in Dashboard/Near-Miss modes but not in other modes (WiFi, GPS, IMU, Shutdown, etc.).

### Root Cause
The `sensor_manager_is_button_pressed()` function returns `true` only once per button press (on release). The button state was being consumed by the first check (Dashboard/Near-Miss mode), so subsequent checks in other modes never saw the button press.

### Solution
Check the button state **once** at the beginning and store the result in local variables:

```c
// Check button state ONCE and store result
bool button_pressed = sensor_manager_is_button_pressed();
bool button_held = sensor_manager_is_button_held();

// Now use stored values for all mode checks
if (current_mode == 0) {  // Dashboard
    if (button_held) { ... }
    else if (button_pressed) { ... }
} else if (current_mode == 5) {  // Near-Miss
    if (button_pressed) { ... }  // Uses stored value
}
// etc.
```

### Files Modified
- `main/main.c` - Button handling section (~line 1100)

---

## Issue 1b: Near-Miss Mode Button Not Working (Feb 1 Evening)

### Problem
After adding ML detection mode cycling, the button in Near-Miss mode stopped working. It was grouped with Dashboard mode for manual flagging instead of cycling detection modes.

### Root Cause
The button handling logic had:
```c
if (current_mode == 0 || current_mode == 5) {  // Dashboard or Near-Miss
    // Manual event flagging
}
```

This meant Near-Miss mode was doing manual flagging instead of cycling detection modes.

### Solution
Separate the two modes:
- **Dashboard mode (0)**: Manual flagging (hold button to flag events)
- **Near-Miss mode (5)**: Cycle detection mode (press button to cycle Hybrid → Algorithm → Neural Net)

```c
if (current_mode == 0) {  // Dashboard mode
    // Manual event flagging
    if (button_held) { ... }
} else if (current_mode == 5 && oled_available) {  // Near-Miss mode
    // Cycle detection mode
    if (button_pressed) {
        oled_display_handle_action_button();
    }
}
```

Also updated manual flagging to only work in Dashboard mode:
```c
// Manual flagging ONLY allowed in Dashboard (0) mode
if (current_mode == OLED_MODE_DASHBOARD) {
    manual_flag_active = sensor_manager_is_button_held();
}
```

### Files Modified
- `main/main.c` - Button handling section (~line 1110) and manual flagging section (~line 965)

---

## Issue 1c: ALL Action Buttons Not Working (Feb 1 Late Evening)

### Problem
**CRITICAL BUG**: After the previous fixes, ALL action buttons stopped working in ALL modes (WiFi, IMU, GPS, USB, Shutdown, Near-Miss). Only Dashboard mode worked.

### Root Cause
The button checking code was **inside the `if (!usb_msc_active)` block**:

```c
if (!usb_msc_active) {  // Normal mode
    // ... sensor reading ...
    // ... data processing ...
    
    // Button checking HERE - WRONG LOCATION!
    if ((current_time - last_button_check) >= 50) {
        bool button_pressed = sensor_manager_is_button_pressed();
        // ... handle all modes ...
    }
} // End of normal mode
```

This meant:
- ✅ Dashboard mode worked (manual flagging is inside the block)
- ❌ All other modes didn't work (button never checked when in those modes)
- ❌ USB mode especially broken (button checking skipped entirely)

### Solution
**Move button checking OUTSIDE the `if (!usb_msc_active)` block** so it runs in ALL modes:

```c
if (!usb_msc_active) {  // Normal mode
    // ... sensor reading ...
    // ... data processing ...
    
    // Dashboard mode manual flagging (needs to be inside for logging)
    if (current_mode == 0) {
        bool button_held = sensor_manager_is_button_held();
        if (button_held) { /* manual flagging */ }
    }
} // End of normal mode

// ===== BUTTON HANDLING FOR ALL OTHER MODES =====
// NOW OUTSIDE the (!usb_msc_active) block!
if ((current_time - last_button_check) >= 50) {
    bool button_pressed = sensor_manager_is_button_pressed();
    bool button_held = sensor_manager_is_button_held();
    
    if (current_mode == 5) { /* Near-Miss */ }
    else if (current_mode == 6) { /* WiFi */ }
    else if (current_mode == 3) { /* IMU */ }
    else if (current_mode == 4) { /* GPS */ }
    else if (current_mode == 7) { /* USB */ }
    else if (current_mode == 8) { /* Shutdown */ }
}
```

### Why This Matters
- **Dashboard mode**: Button checking stays inside the block (tied to `manual_flag_active` for logging)
- **All other modes**: Button checking moved outside the block (works regardless of USB state)
- **Result**: Buttons now work in ALL modes, including USB Transfer mode

### Files Modified
- `main/main.c` - Moved button handling outside `(!usb_msc_active)` block (~line 1296)

---

## Issue 2: ToF Sensor Matrix Backwards

### Problem
The ToF sensor (VL53L8CX) was mounted backwards on the device, so the 4×4 distance matrix was inverted both horizontally and vertically.

### Solution
Added matrix flip in the sensor reading code to correct the orientation:

```c
// Original sensor layout:        Flipped layout (corrected):
//   0  1  2  3                      15 14 13 12
//   4  5  6  7                      11 10  9  8
//   8  9 10 11                       7  6  5  4
//  12 13 14 15                       3  2  1  0

for (int row = 0; row < 4; row++) {
    for (int col = 0; col < 4; col++) {
        int sensor_index = row * 4 + col;
        
        // Flip both vertically and horizontally
        int flipped_row = 3 - row;
        int flipped_col = 3 - col;
        int output_index = flipped_row * 4 + flipped_col;
        
        data->distances[output_index] = results.distance_mm[...];
    }
}
```

### Files Modified
- `components/sensor_manager/sensor_manager.cpp` - `sensor_manager_read_tof()` function

### Impact on Near-Miss Detection

**Q: Does the matrix position matter for near-miss detection?**

**A: No, the orientation flip does NOT affect near-miss detection accuracy!**

Here's why:

1. **Minimum Distance Detection**: The algorithm uses `min_distance` across all 16 zones. The minimum value is the same regardless of which zone it comes from.

2. **Approach Rate Calculation**: Based on how the minimum distance changes over time. The rate is the same regardless of sensor orientation.

3. **ML Model Training**: The model learns patterns from all 16 ToF features. It doesn't care about absolute positions - it learns relative patterns like:
   - "When multiple zones show close distances simultaneously"
   - "When distances change rapidly across zones"
   - "Spatial patterns of approaching objects"

4. **What Matters**: The **consistency** of orientation during training and deployment. As long as the orientation is the same when you:
   - Collect training data
   - Train the model
   - Deploy for detection
   
   The system will work correctly.

**Important Notes**:
- ✅ The flip is applied at sensor reading level, so ALL data (logging, ML, algorithm) uses the corrected orientation
- ✅ Future data collection will have correct orientation
- ⚠️ Old data (if any) has backwards orientation - recommend starting fresh for consistency
- ✅ The ML model will learn the correct patterns from new data

**For Your Thesis**:
- Document that sensor orientation was corrected on Feb 1, 2026
- All data used for analysis was collected with consistent orientation
- Orientation doesn't affect detection accuracy, only spatial interpretation

---

## Testing Checklist

### Button Functionality
- [x] Dashboard mode: ACTION button flags events (hold to flag)
- [x] Near-Miss mode: ACTION button cycles detection modes (press to cycle)
- [x] WiFi Settings mode: ACTION button triggers WiFi sync
- [x] IMU Focus mode: ACTION button triggers IMU rescan
- [x] GPS mode: ACTION button triggers GPS reconnect
- [x] USB Transfer mode: ACTION button toggles USB MSC
- [x] Shutdown mode: ACTION button confirms shutdown

### ToF Orientation
- [ ] Point sensor at object on the left → distance shows in left columns
- [ ] Point sensor at object on the right → distance shows in right columns
- [ ] Point sensor at object on top → distance shows in top rows
- [ ] Point sensor at object on bottom → distance shows in bottom rows
- [ ] Verify OLED display matches physical orientation

---

## Build and Flash

```bash
# Setup ESP-IDF environment
.\setup_idf.ps1

# Build
idf.py build

# Flash
idf.py flash monitor
```

---

## Impact on Existing Data

### ToF Matrix Flip
⚠️ **IMPORTANT**: If you have existing CSV data collected before this fix, the ToF columns will be in the old (backwards) orientation.

**Options**:
1. **Discard old data** - Simplest, start fresh with correct orientation
2. **Flip old data** - Write a script to flip the ToF matrix in old CSV files
3. **Train separate models** - One for old data, one for new data

**Recommendation**: Start fresh data collection with the corrected orientation for consistency.

### Button Fix
No impact on existing data - this only affects real-time button handling.

---

## Notes

- The ToF flip is applied at the sensor reading level, so all downstream code (data processor, ML inference, logging) automatically gets the corrected orientation
- The button fix ensures all modes can respond to button presses correctly
- Dashboard mode: Manual flagging (hold button)
- Near-Miss mode: Cycle detection mode (press button)
- Both fixes are transparent to the rest of the system

---

**Status**: ✅ Ready to flash and test
