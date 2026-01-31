# USB Transfer Mode Button Toggle Fix - January 29, 2026

## Problem Summary

**Issue**: ACTION button doesn't exit USB Transfer mode (Mode 7). Second press does nothing.

**Root Cause**: Button handler uses `sensor_manager_is_button_pressed()` which only returns `true` for ONE frame after button release. If the OLED handler takes time to process, the button state is already cleared by the next check.

---

## Current Button Flow (BROKEN)

```
User presses ACTION button in Mode 7
  ↓
main.c detects button press (line 913)
  ↓
Calls oled_display_handle_action_button() (line 922)
  ↓
OLED handler activates USB mode (takes 500-800ms with delays)
  ↓
Returns to main.c
  ↓
Next loop iteration: Button state already cleared
  ↓
Second press: Same flow, but USB is already active
  ↓
OLED handler should deactivate, but button state might be missed
```

---

## Why It Fails

### Problem 1: Button State Timing

`sensor_manager_is_button_pressed()` returns `true` only for **one frame** after button release. The OLED handler has delays:

```c
// In oled_display.c, line ~1200
vTaskDelay(pdMS_TO_TICKS(500));  // 500ms delay
vTaskDelay(pdMS_TO_TICKS(300));  // 300ms delay
vTaskDelay(pdMS_TO_TICKS(2000)); // 2000ms delay
```

By the time the handler returns, the button state is cleared.

### Problem 2: USB State Check

The handler checks `usb_msc_sdcard_is_active()` to decide whether to activate or deactivate. But the state might not update immediately after the first call.

---

## Solution: Debounced Button Handling

### Option 1: Use Button Hold Detection (RECOMMENDED)

Instead of checking `is_button_pressed()` (one-frame), use `is_button_held()` (persistent):

```c
// In main.c, line ~912
} else if (current_mode == 7 && oled_available) {  // USB Transfer mode
    // Button 2 (GPIO 3): Toggle USB MSC mode
    static bool last_usb_button_state = false;
    bool current_usb_button_state = sensor_manager_is_button_held();
    
    // Detect button release (falling edge)
    if (!current_usb_button_state && last_usb_button_state) {
        ESP_LOGI(TAG, "💾 USB Transfer mode toggle triggered from button");
        
        // Blue LED during USB mode
        if (led_strip != NULL) {
            led_strip_set_pixel(led_strip, 0, 0, 0, 255);  // Blue
            led_strip_refresh(led_strip);
        }
        
        oled_display_handle_action_button();
    }
    
    last_usb_button_state = current_usb_button_state;
}
```

**Why this works**: Button state persists across frames, so we can detect the release edge reliably.

### Option 2: Remove Delays from OLED Handler

Remove the delays in `oled_display.c` to make the handler return quickly:

```c
// In oled_display.c, line ~1200
// REMOVE these delays:
// vTaskDelay(pdMS_TO_TICKS(500));
// vTaskDelay(pdMS_TO_TICKS(300));
// vTaskDelay(pdMS_TO_TICKS(2000));

// Replace with immediate return
```

**Why this works**: Handler returns before button state is cleared.

**Downside**: User doesn't see progress messages on OLED.

### Option 3: Add Button State Latch

Add a flag to remember button press across frames:

```c
// In main.c, add static variable
static bool usb_button_pressed = false;

// In button check loop
if (sensor_manager_is_button_pressed()) {
    usb_button_pressed = true;
}

// In USB mode handler
if (usb_button_pressed) {
    usb_button_pressed = false;  // Clear flag
    oled_display_handle_action_button();
}
```

**Why this works**: Button press is latched and persists until handled.

---

## Recommended Fix: Option 1 (Button Hold Detection)

This is the cleanest solution that works with the existing code structure.

### Changes Required

**File**: `main/main.c`  
**Location**: Line ~912 (USB Transfer mode button handler)

**Before**:
```c
} else if (current_mode == 7 && oled_available) {  // USB Transfer mode
    // Button 2 (GPIO 3): Toggle USB MSC mode
    if (sensor_manager_is_button_pressed()) {
        ESP_LOGI(TAG, "💾 USB Transfer mode toggle triggered from button");
        
        // Blue LED during USB mode
        if (led_strip != NULL) {
            led_strip_set_pixel(led_strip, 0, 0, 0, 255);  // Blue
            led_strip_refresh(led_strip);
        }
        
        oled_display_handle_action_button();
    }
}
```

**After**:
```c
} else if (current_mode == 7 && oled_available) {  // USB Transfer mode
    // Button 2 (GPIO 3): Toggle USB MSC mode
    // Use button hold detection for reliable edge detection
    static bool last_usb_button_state = false;
    bool current_usb_button_state = sensor_manager_is_button_held();
    
    // Detect button release (falling edge) - more reliable than is_button_pressed()
    if (!current_usb_button_state && last_usb_button_state) {
        ESP_LOGI(TAG, "💾 USB Transfer mode toggle triggered from button");
        
        // Blue LED during USB mode
        if (led_strip != NULL) {
            led_strip_set_pixel(led_strip, 0, 0, 0, 255);  // Blue
            led_strip_refresh(led_strip);
        }
        
        oled_display_handle_action_button();
    }
    
    last_usb_button_state = current_usb_button_state;
}
```

---

## Testing the Fix

### Test Sequence

1. **Build and flash**:
   ```bash
   idf.py build flash monitor
   ```

2. **Navigate to Mode 7**:
   - Press Mode Change button until OLED shows "USB Transfer Mode"

3. **First press (Activate)**:
   - Press ACTION button (GPIO 3)
   - OLED should show: "Activating..."
   - Then: "[USB] Connect LEFT port" + "ACTION: Cancel"
   - Serial monitor should show: "✓ USB Transfer mode activated"

4. **Second press (Deactivate)**:
   - Press ACTION button again
   - OLED should show: "Deactivating..."
   - Then: "Logging resumed" + "[OK]"
   - Serial monitor should show: "✓ Logging mode activated"

5. **Repeat**:
   - Press ACTION again to re-activate
   - Should work reliably every time

### Expected Serial Output

```
I (xxxxx) MAIN: 💾 USB Transfer mode toggle triggered from button
I (xxxxx) OLED_SPI: USB Transfer mode toggle requested from display
I (xxxxx) USB_MSC_SD: 📤 Activating USB Transfer mode...
I (xxxxx) USB_MSC_SD: ⏸️  Pausing data logging...
I (xxxxx) STORAGE_MGR: Logging stopped. Total entries: XXX, Dropped: 0
I (xxxxx) USB_MSC_SD: ✓ USB Transfer mode activated
I (xxxxx) USB_MSC_SD: 🔌 Connect LEFT port (USB-OTG) to computer

[User presses ACTION again]

I (xxxxx) MAIN: 💾 USB Transfer mode toggle triggered from button
I (xxxxx) OLED_SPI: USB Transfer mode toggle requested from display
I (xxxxx) USB_MSC_SD: 📥 Deactivating USB Transfer mode...
I (xxxxx) USB_MSC_SD: ▶️  Resuming data logging...
I (xxxxx) STORAGE_MGR: Logging started
I (xxxxx) USB_MSC_SD: ✓ Logging mode activated
```

---

## Additional Issue: PC Not Detecting USB Device

Even with the button fix, the PC still doesn't detect the USB device (`USB_connected=NO`).

### Possible Causes

1. **USB Cable**: Power-only cable (no data lines)
2. **Wrong Port**: Using RIGHT port instead of LEFT port
3. **Hardware Issue**: LEFT port not functioning as USB-OTG
4. **Driver Issue**: PC needs USB driver

### Hardware Verification Steps

#### Step 1: Verify USB Cable

Try 3 different USB cables. Many cables are power-only.

**Test**: Connect an Android phone with the same cable. If PC detects the phone, cable is good.

#### Step 2: Verify LEFT Port

The ESP32-S3 DevKitC has TWO USB ports:
- **LEFT port**: USB-OTG (for custom USB devices) ← Use this one!
- **RIGHT port**: USB-to-UART (for programming/monitoring)

Make sure you're connecting to the **LEFT port**.

#### Step 3: Check Device Manager (Windows)

When you connect the LEFT port:
1. Open Device Manager
2. Look under "Universal Serial Bus devices"
3. Look for "Unknown Device" or "Espressif" device

If nothing appears, PC isn't detecting anything at all.

#### Step 4: Test with ESP-IDF Example

Test if hardware works with a simple example:

```bash
cd $IDF_PATH/examples/peripherals/usb/device/tusb_msc
idf.py build flash monitor
```

If this works, hardware is OK and it's a software issue.  
If this doesn't work, hardware problem or cable issue.

---

## Conclusion

### Button Toggle Fix

The button toggle issue is a **software timing problem** that can be fixed by using button hold detection instead of button press detection.

**Estimated fix time**: 5 minutes (change 10 lines of code)

### USB Detection Issue

The PC not detecting the USB device is likely a **hardware/cable issue** that needs physical verification.

**Recommended approach**:
1. Fix the button toggle first (easy software fix)
2. Test with 3 different USB cables
3. Verify using LEFT port
4. Test with ESP-IDF example to verify hardware

### Alternative: Skip USB Transfer Mode

If USB Transfer mode is too complex and blocking your thesis progress:

1. **Remove USB Transfer mode** (Mode 7)
2. **Use SD card reader** to access files
3. **Focus on near-miss detection** (your main thesis goal)
4. **Add USB Transfer later** if time permits

Your thesis is about near-miss detection, not USB file transfer. Don't let this block your progress!

---

## Files to Modify

1. `main/main.c` - Line ~912 (USB Transfer mode button handler)

That's it! One file, one function, 10 lines of code.

