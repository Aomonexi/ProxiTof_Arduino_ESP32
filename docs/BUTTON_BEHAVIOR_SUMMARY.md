# Button Behavior Summary

**Date:** January 26, 2026  
**Feature:** Context-aware button behavior  
**Status:** ✅ Implemented

---

## Button Layout

```
┌─────────────────────────────────────┐
│         ProxiTof Device             │
│                                     │
│  [Button 1]         [Button 2]     │
│   GPIO 20            GPIO 3        │
│   MODE/NAV           ACTION        │
└─────────────────────────────────────┘
```

---

## Button Functions

### Button 1 (GPIO 20) - Mode/Navigation
**Always:** Cycle through display modes

**Modes:**
1. Dashboard → 2. ToF Focus → 3. IMU Focus → 4. Status → 5. Near Miss → 6. WiFi Settings → (back to 1)

---

### Button 2 (GPIO 3) - Action (Context-Aware)

**Function changes based on current display mode:**

```
┌──────────────────┬─────────────────────────────────┐
│ Display Mode     │ Button 2 Action                 │
├──────────────────┼─────────────────────────────────┤
│ Dashboard        │ Flag event (manual near-miss)   │
│ ToF Focus        │ No action                       │
│ IMU Focus        │ No action                       │
│ Status           │ No action                       │
│ Near Miss        │ Flag event (manual near-miss)   │
│ WiFi Settings    │ Trigger WiFi sync               │
└──────────────────┴─────────────────────────────────┘
```

---

## Usage Examples

### Example 1: Flag a Near-Miss Event

```
1. Device is in Dashboard mode (default)
2. Near-miss event occurs
3. Press Button 2 (GPIO 3)
4. Event is flagged in log file
5. Console shows: "✓ EVENT FLAGGED"
```

**Or from Near-Miss mode:**
```
1. Press Button 1 until "Near Miss" mode
2. Near-miss event occurs
3. Press Button 2 (GPIO 3)
4. Event is flagged
```

---

### Example 2: Sync Time via WiFi

```
1. Press Button 1 repeatedly until "WiFi Settings" mode
2. Display shows sync status
3. Press Button 2 (GPIO 3)
4. Display shows "Connecting..."
5. Wait 5-10 seconds
6. Display shows "Sync SUCCESS!" or "Sync FAILED"
7. Returns to WiFi Settings after 2 seconds
```

---

### Example 3: Check System Status

```
1. Press Button 1 repeatedly until "Status" mode
2. View system information:
   - ToF sensor status
   - IMU sensor status
   - SD card status
   - GPS status
   - RTC status
3. Button 2 does nothing in this mode
```

---

## Mode Descriptions

### 1. Dashboard Mode
**Display:** ToF grid (bars) + IMU summary + Status bar  
**Button 2:** Flag event  
**Use case:** Normal operation, real-time monitoring

### 2. ToF Focus Mode
**Display:** Large 4×4 ToF grid with numbers (0-4000mm)  
**Button 2:** No action  
**Use case:** Detailed distance monitoring

### 3. IMU Focus Mode
**Display:** Full IMU data (orientation, acceleration, gyro)  
**Button 2:** No action  
**Use case:** Motion analysis, calibration

### 4. Status Mode
**Display:** System health (sensors, SD, GPS, RTC, uptime)  
**Button 2:** No action  
**Use case:** Troubleshooting, system check

### 5. Near Miss Mode
**Display:** Near-miss alert details with reasons  
**Button 2:** Flag event  
**Use case:** Review near-miss events, manual flagging

### 6. WiFi Settings Mode (NEW!)
**Display:** WiFi sync status, last sync time, sync count  
**Button 2:** Trigger WiFi sync  
**Use case:** Time synchronization, WiFi control

---

## Implementation Details

### Code Changes

**Files Modified:**
1. `components/oled_display/include/oled_display.h`
   - Added `OLED_MODE_WIFI_SETTINGS` enum
   - Added `oled_display_handle_action_button()` function

2. `components/oled_display/oled_display.c`
   - Implemented WiFi Settings mode display
   - Implemented action button handler
   - Updated mode names array

3. `main/main.c`
   - Added context-aware button handling
   - Updated mode names array

### Button Debouncing

**Mode Button (Button 1):**
- Debounce time: 200ms
- Prevents accidental double-presses

**Action Button (Button 2):**
- Handled by sensor_manager
- Debounce built-in

---

## Power Consumption

### Button Press Impact

**Mode button (Button 1):**
- No power impact (just changes display)

**Action button (Button 2):**
- Dashboard/Near-Miss: No power impact (just sets flag)
- WiFi Settings: ~160mA for 5-10 seconds (WiFi sync)
- Other modes: No action, no power impact

---

## Future Enhancements

### Planned Features:

1. **Long Press Actions**
   - Button 1 long press: Jump to specific mode
   - Button 2 long press: Different action

2. **Button Combinations**
   - Both buttons: Screenshot/freeze display
   - Both buttons long: Reset/reboot

3. **Haptic Feedback**
   - Vibration motor on button press
   - Different patterns for different actions

4. **LED Feedback**
   - RGB LED blinks on button press
   - Different colors for different actions

---

## Troubleshooting

### Problem: Button doesn't respond

**Solutions:**
1. Check debounce delay (wait 200ms between presses)
2. Check button is properly connected
3. Check GPIO pins (20 and 19)

### Problem: Wrong action happens

**Solutions:**
1. Check current display mode (Button 2 is context-aware)
2. Press Button 1 to cycle to correct mode
3. Check console log for mode changes

### Problem: WiFi sync doesn't work

**Solutions:**
1. Make sure you're in WiFi Settings mode
2. Check WiFi is enabled in config.txt
3. Check WiFi credentials are correct
4. Check you're in WiFi range

---

## Quick Reference Card

```
╔═══════════════════════════════════════════════════╗
║           ProxiTof Button Reference               ║
╠═══════════════════════════════════════════════════╣
║                                                   ║
║  Button 1 (GPIO 20) - MODE                       ║
║  ├─ Press: Cycle display modes                   ║
║  └─ Modes: Dashboard → ToF → IMU → Status →      ║
║            Near Miss → WiFi Settings              ║
║                                                   ║
║  Button 2 (GPIO 19) - ACTION (context-aware)     ║
║  ├─ Dashboard: Flag event                        ║
║  ├─ Near Miss: Flag event                        ║
║  ├─ WiFi Settings: Trigger WiFi sync             ║
║  └─ Other modes: No action                       ║
║                                                   ║
╠═══════════════════════════════════════════════════╣
║  Common Tasks:                                    ║
║  • Flag event: Dashboard → Button 2              ║
║  • Sync time: Button 1 → WiFi Settings → Btn 2   ║
║  • Check status: Button 1 → Status               ║
║  • View ToF: Button 1 → ToF Focus                ║
╚═══════════════════════════════════════════════════╝
```

---

**Status:** ✅ Implemented  
**Documentation:** ✅ Complete  
**User-friendly:** ✅ Yes!

