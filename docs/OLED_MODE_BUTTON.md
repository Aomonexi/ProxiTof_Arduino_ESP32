# OLED Display Mode Switch Button

**Date**: January 25, 2026  
**Status**: ✅ Implemented and ready to test

---

## Overview

Added a dedicated button to cycle through OLED display modes without needing to edit config files or reflash firmware.

---

## Hardware Setup

### Wiring

```
Push Button         →    ESP32-S3
───────────              ─────────
Terminal 1          →    GPIO 20
Terminal 2          →    GND
```

**Button Specifications:**
- Type: Normally Open (NO) momentary push button
- Pull-up: Internal (no external resistor needed)
- Active: LOW (pressed = 0V, released = 3.3V)
- Debounce: 200ms in software

**Why GPIO 20?**
- Available pin (not used by any other component)
- Supports GPIO input with internal pull-up
- No conflicts with existing sensors or peripherals

---

## Display Modes

Press the button to cycle through 4 display modes:

### 1. Dashboard Mode (Default)
**Yellow Zone (Top 16px):**
- System status: "LOGGING 123mm 60Hz"
- GPS status indicator

**Blue Zone (Bottom 48px):**
- ToF 4x4 grid with bar graphs (closer object = wider bar)
- IMU summary (Roll, Pitch, Yaw)
- Acceleration magnitude

**Best for:** Real-time monitoring during data collection

---

### 2. ToF Focus Mode
**Yellow Zone:**
- Minimum distance
- Approach rate (mm/s)

**Blue Zone:**
- Large 4x4 ToF grid with numeric values
- All 16 zones clearly visible

**Best for:** Debugging ToF sensor, analyzing distance patterns

---

### 3. IMU Focus Mode
**Yellow Zone:**
- IMU status
- Jerk magnitude (m/s³)

**Blue Zone:**
- Detailed orientation (Roll, Pitch, Yaw)
- 3-axis acceleration (X, Y, Z)
- 3-axis gyroscope (optional)

**Best for:** Debugging IMU sensor, analyzing motion patterns

---

### 4. Status Mode
**Yellow Zone:**
- System name: "ProxiTof v1.0"
- Overall status: OK/ERROR

**Blue Zone:**
- Component status:
  - ToF: OK 60Hz
  - IMU: OK 100Hz
  - SD: OK (free space)
  - GPS: FIX (satellite count)
  - RTC: Current time
- Buffer usage percentage
- Event count

**Best for:** System health check, troubleshooting

---

## How It Works

### Button Press Detection
```c
// Active LOW with internal pull-up
bool button_pressed = (gpio_get_level(GPIO_NUM_20) == 0);

// Debounce: 200ms
// Prevents false triggers from mechanical bounce
```

### Mode Cycling
```c
// Current mode → Next mode
Dashboard  → ToF Focus
ToF Focus  → IMU Focus
IMU Focus  → Status
Status     → Dashboard (loops back)
```

### Visual Feedback
When you press the button:
1. Display briefly shows: "Mode: [New Mode Name]"
2. After 500ms, switches to the new mode
3. Serial log shows: "🔄 Display mode changed: Dashboard → ToF Focus"

---

## Usage

### During Data Collection
1. **Start with Dashboard mode** - See everything at once
2. **Press button** to cycle to ToF Focus if you need detailed distance data
3. **Press again** for IMU Focus to check orientation
4. **Press again** for Status to verify all systems are healthy
5. **Press again** to return to Dashboard

### During Debugging
- **ToF not working?** → Switch to ToF Focus mode to see raw values
- **IMU acting weird?** → Switch to IMU Focus mode to see orientation
- **System issues?** → Switch to Status mode to check component health

---

## Configuration

### Enable/Disable OLED Display
Edit `config.txt` on SD card:
```
OLED_ENABLED=true          # Enable OLED display
OLED_REFRESH_HZ=60         # 60 FPS refresh rate
OLED_BRIGHTNESS=128        # 0-255 (128 = medium)
OLED_MODE=0                # Default mode (0=Dashboard)
```

**Note:** Mode button works regardless of `OLED_MODE` setting. The button always starts from the configured default mode.

### Disable Mode Button
If you don't want the mode button:
1. Don't connect the button to GPIO 20
2. The system will work fine without it
3. Display will stay in the mode set in `config.txt`

---

## Serial Output

When you press the mode button, you'll see:
```
I (12345) MAIN: 🔄 Display mode changed: Dashboard → ToF Focus
```

When OLED initializes:
```
I (1234) MAIN: ✓ Mode switch button initialized (GPIO 20)
I (1235) MAIN: 💡 Press button to cycle through display modes
```

---

## Troubleshooting

### Button Not Working

**1. Check Wiring**
```
✓ One terminal → GPIO 20
✓ Other terminal → GND
✓ No resistor needed (internal pull-up)
```

**2. Check Button Type**
```
✓ Normally Open (NO) momentary
✗ NOT Normally Closed (NC)
✗ NOT latching/toggle switch
```

**3. Test Button Manually**
```c
// Add this to main.c temporarily:
ESP_LOGI(TAG, "GPIO 20 level: %d", gpio_get_level(GPIO_NUM_20));
// Should be 1 when released, 0 when pressed
```

**4. Check OLED is Enabled**
```
// Mode button only works if OLED is enabled
// Check config.txt: OLED_ENABLED=true
```

### Display Not Changing

**1. Check Serial Output**
- Look for "Display mode changed" message
- If you see the message but display doesn't change, OLED might not be initialized

**2. Check OLED Initialization**
```
// Look for this in serial output:
✓ OLED display initialized successfully
```

**3. Try Different Modes**
- Press button 4 times to cycle through all modes
- One of them should look different!

---

## Code Implementation

### Files Modified

1. **`main/main.c`**
   - Added `MODE_BUTTON_PIN` definition (GPIO 20)
   - Added `init_mode_button()` function
   - Added `check_mode_button()` function
   - Integrated button check in main loop

2. **`docs/complete-wiring-diagram.md`**
   - Added mode button to pin map
   - Updated GPIO usage table
   - Added wiring instructions

### Key Functions

```c
// Initialize button GPIO with pull-up
static esp_err_t init_mode_button(void);

// Check button and cycle mode if pressed
// Returns true if mode was changed
static bool check_mode_button(void);
```

---

## Benefits

✅ **No reflashing needed** - Change modes on the fly  
✅ **Quick debugging** - Switch to detailed view instantly  
✅ **User-friendly** - One button, simple operation  
✅ **Visual feedback** - See mode name when switching  
✅ **Debounced** - No false triggers from button bounce  
✅ **Non-blocking** - Doesn't slow down data collection  

---

## Future Enhancements

Possible additions (not implemented yet):

1. **Long press** - Hold button for 2 seconds to adjust brightness
2. **Double press** - Quick double-tap to toggle between two favorite modes
3. **Mode memory** - Remember last used mode across reboots
4. **LED feedback** - Flash LED when mode changes
5. **Buzzer feedback** - Short beep when mode changes

---

## Summary

**Hardware:**
- Button on GPIO 20 → GND
- Normally Open momentary push button
- Internal pull-up (no external resistor)

**Software:**
- 200ms debounce
- Cycles through 4 modes
- Visual feedback on display
- Serial log messages

**Usage:**
- Press button to cycle modes
- Works during data collection
- Perfect for debugging and monitoring

**Status:** ✅ Ready to test!

---

**Connect the button and press it to cycle through display modes!** 🎯
