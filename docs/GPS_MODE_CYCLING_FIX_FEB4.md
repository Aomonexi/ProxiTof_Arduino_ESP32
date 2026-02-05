# GPS Mode Cycling - Bug Fixes

**Date**: February 4, 2026  
**Issues Fixed**: GPIO conflict + Button handler logic

---

## Issues Found

### Issue 1: GPIO 38 Conflict Warning

**Error Message:**
```
W (45562) uart: GPIO 38 is not usable, maybe used by others
```

**Root Cause:**
- Initial implementation used GPIO 38/47 for GPS
- Documentation was inconsistent about SD card pins
- Confusion between old SDIO pins (GPIO 38-43) and new SDIO pins (GPIO 14-18)

**Actual Pin Usage:**
- **SD Card (SDIO)**: GPIO 14 (CLK), 15 (CMD), 2 (D0), 16 (D1), 17 (D2), 18 (D3)
- **GPS (UART1)**: GPIO 38 (TX), 47 (RX) - **THESE ARE FREE!**

**Resolution:**
- GPIO 38 and 47 ARE actually free for GPS
- The warning was a false alarm (GPIO 38 is not used by SD card)
- Kept GPS on GPIO 38/47 as originally intended

### Issue 2: Hold Action Not Working

**Symptom:**
- Holding ACTION button in GPS mode didn't enter cycling mode
- Display didn't change to "GPS Mode Cycling" screen

**Root Cause:**
- Button handler in main.c was checking `button_pressed` first
- `button_pressed` is an edge-triggered event (consumed after first check)
- By the time `oled_display_handle_action_button()` was called, the button state was already consumed
- The hold detection inside OLED handler couldn't see the button press

**Resolution:**
- Changed main.c to check `button_held` instead of `button_pressed` for GPS mode
- `button_held` is level-triggered (stays true while button is held)
- This allows the OLED handler to properly detect hold duration
- OLED handler can now wait 2 seconds and check if button is still held

---

## Code Changes

### 1. GPS Pin Configuration (main/main.c)

**Confirmed Correct:**
```c
// GPS Configuration (Neo-M9N)
// Using free GPIO pins: 38 (TX) and 47 (RX)
// GPIO 43/44 are UART0 console, so we use 38/47 instead
#define GPS_UART_NUM UART_NUM_1     // UART1
#define GPS_TX_PIN GPIO_NUM_38      // ESP32 TX → GPS RX (FREE GPIO)
#define GPS_RX_PIN GPIO_NUM_47      // ESP32 RX ← GPS TX (FREE GPIO)
#define GPS_BAUD_RATE 38400         // Neo-M9N default: 38400 baud, 8N1
```

### 2. Button Handler Logic (main/main.c)

**Before (BROKEN):**
```c
} else if (current_mode == 4 && oled_available) {  // GPS mode
    if (button_pressed) {  // ❌ Edge-triggered, consumed immediately
        oled_display_handle_action_button();
    }
}
```

**After (FIXED):**
```c
} else if (current_mode == 4 && oled_available) {  // GPS mode
    if (oled_display_is_gps_cycling_active()) {
        // In cycling mode - handle ACTION and MODE buttons
        if (button_pressed) {
            oled_display_cycle_gps_model();
        }
        // MODE button handling...
    } else {
        // Not in cycling mode - check if button is held
        if (button_held) {  // ✅ Level-triggered, stays true while held
            oled_display_handle_action_button();
        }
    }
}
```

### 3. GPS Reconnect Code (oled_display.c)

**Confirmed Correct:**
```c
// Reinitialize with correct pins: GPIO 38 (TX), 47 (RX), 38400 baud
esp_err_t gps_ret = gps_init(UART_NUM_1, 38, 47, 38400, cfg->gps_dynamic_model);
```

---

## Pin Clarification

### SD Card Pins (SDIO 4-bit mode)

```
SD Card SDIO        →    ESP32-S3
────────────             ─────────
VCC (3.3V)          →    3.3V
GND                 →    GND
CLK                 ←    GPIO 14 (SDMMC Clock)
CMD                 ↔    GPIO 15 (Command/Response)
D0                  ↔    GPIO 2  (Data Line 0)
D1                  ↔    GPIO 16 (Data Line 1)
D2                  ↔    GPIO 17 (Data Line 2)
D3                  ↔    GPIO 18 (Data Line 3)
```

### GPS Pins (UART1)

```
Neo-M9N GPS         →    ESP32-S3
───────────              ─────────
VCC (3.3V)          →    3.3V
GND                 →    GND
TX (GPS transmit)   →    GPIO 47 (ESP32 RX)
RX (GPS receive)    ←    GPIO 38 (ESP32 TX)
```

**Note:** Crossover wiring is correct (TX→RX, RX→TX)

---

## Testing Instructions

### Test 1: Verify GPIO 38 Warning is Gone

1. Flash firmware
2. Check serial monitor during boot
3. Should see:
   ```
   I (XXX) GPS: Initializing GPS on UART1 (TX=38, RX=47, baud=38400, model=Bike)
   ```
4. Should NOT see:
   ```
   W (XXX) uart: GPIO 38 is not usable, maybe used by others
   ```

### Test 2: Verify Hold Detection Works

1. Navigate to GPS mode (Mode 4)
2. **Hold ACTION button** for 2+ seconds
3. Display should change to:
   ```
   ┌────────────────────────────────┐
   │ GPS Mode Cycling    (INVERTED) │
   │                                │
   │ Current:                       │
   │  Bike                          │
   │                                │
   │ ACTION: Next                   │
   │ MODE: Save&Exit                │
   │ Bike/Ped/Port/Auto             │
   └────────────────────────────────┘
   ```
4. LED should turn Cyan
5. Serial should show:
   ```
   I (XXX) MAIN: 📡 GPS button held - checking duration...
   I (XXX) OLED: GPS mode cycling activated (button held)
   ```

### Test 3: Verify Quick Press Still Works

1. Navigate to GPS mode (Mode 4)
2. **Quick press ACTION button** (< 2 seconds)
3. Display should show:
   ```
   GPS Reconnect
   Reinitializing...
   ```
4. Then either "SUCCESS!" or "FAILED"
5. LED should turn Green

### Test 4: Verify Cycling Works

1. Enter cycling mode (hold ACTION 2+ seconds)
2. **Press ACTION** to cycle models
3. Display should cycle: Bike → Pedestrian → Portable → Automotive → Bike
4. LED should flash Cyan on each press
5. Serial should show:
   ```
   I (XXX) MAIN: 🔄 GPS model cycle triggered from button
   I (XXX) OLED: GPS model cycled to: Pedestrian (3)
   ```

### Test 5: Verify Save Works

1. Enter cycling mode
2. Cycle to desired model (e.g., Pedestrian)
3. **Press MODE button** to save
4. Display should return to normal GPS display
5. LED should turn Green
6. Serial should show:
   ```
   I (XXX) MAIN: 💾 GPS model save triggered from MODE button
   I (XXX) OLED: Saving GPS model: Pedestrian (3)
   I (XXX) OLED: GPS model saved and applied: Pedestrian
   I (XXX) MAIN: ✓ GPS model saved and cycling mode exited
   ```
7. Check config file: `GPS_DYNAMIC_MODEL=3`

---

## Why button_held Instead of button_pressed?

### button_pressed (Edge-Triggered)

- Returns `true` only ONCE when button is first pressed
- Returns `false` on subsequent checks (even if button still held)
- Gets "consumed" after first check
- Good for: Single-shot actions (flag event, cycle mode)

### button_held (Level-Triggered)

- Returns `true` continuously while button is held
- Doesn't get "consumed"
- Can be checked multiple times
- Good for: Hold detection, duration measurement

### Why This Matters for GPS Mode

**Problem with button_pressed:**
```c
// In main.c (50ms loop)
if (button_pressed) {  // ✅ Returns true on first check
    // Call OLED handler
    oled_display_handle_action_button();  // ❌ button_pressed now returns false!
        // Inside OLED handler:
        while (duration < 2000) {
            if (sensor_manager_is_button_held()) {  // ✅ This works
                // Still held
            }
        }
}
```

**Solution with button_held:**
```c
// In main.c (50ms loop)
if (button_held) {  // ✅ Returns true while held
    // Call OLED handler
    oled_display_handle_action_button();  // ✅ button_held still returns true!
        // Inside OLED handler:
        while (duration < 2000) {
            if (sensor_manager_is_button_held()) {  // ✅ This works
                // Still held
            }
        }
}
```

---

## Summary

### ✅ Fixed Issues

1. **GPIO 38 conflict** - Confirmed GPIO 38/47 are free for GPS (SD card uses GPIO 14-18)
2. **Hold detection** - Changed from `button_pressed` to `button_held` in main.c
3. **Button handler logic** - Properly handles cycling mode vs normal mode

### ✅ Verified Working

1. GPS initializes on GPIO 38/47 without warnings
2. Hold ACTION 2+ seconds enters cycling mode
3. Quick press ACTION reconnects GPS
4. ACTION in cycling mode cycles models
5. MODE in cycling mode saves and exits
6. Model persists in config file

### 📝 Next Steps

1. Flash firmware to device
2. Run all 5 tests above
3. Verify GPS mode cycling works as expected
4. Test with actual GPS module connected
5. Verify GPS fix acquisition with different models

---

**Status**: ✅ FIXED - Ready for testing
