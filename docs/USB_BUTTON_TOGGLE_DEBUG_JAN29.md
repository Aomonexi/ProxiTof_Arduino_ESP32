# USB Button Toggle - Debug Fix - January 29, 2026

## 🐛 Problem

ACTION button doesn't exit USB Transfer mode. Second press does nothing - device stays in USB mode.

**Symptoms**:
- First press: USB mode activates ✅
- Second press: Nothing happens ❌
- Device stuck in USB Transfer mode
- Must restart or change mode to exit

---

## 🔍 Debug Changes Applied

### Added Debug Logging

Added detailed logging to track button toggle behavior:

**File**: `main/main.c` (line ~912)

**Changes**:
1. ✅ Log USB state BEFORE toggle
2. ✅ Log USB state AFTER toggle
3. ✅ Verify state actually changed
4. ✅ Warn if state didn't change

**New logging output**:
```c
ESP_LOGI(TAG, "💾 USB Transfer mode toggle triggered from button");
ESP_LOGI(TAG, "Current USB state: %s", usb_active_before ? "ACTIVE" : "INACTIVE");
// ... toggle happens ...
ESP_LOGI(TAG, "After toggle: USB state: %s (ret=%s)", 
         usb_active_after ? "ACTIVE" : "INACTIVE",
         esp_err_to_name(toggle_ret));
```

---

## 🧪 Testing Instructions

### Step 1: Build and Flash

```bash
idf.py build flash monitor
```

### Step 2: Navigate to Mode 7

Press Mode Change button until OLED shows "USB Transfer Mode"

### Step 3: First Press (Activate)

Press ACTION button

**Expected serial output**:
```
I (xxxxx) MAIN: 💾 USB Transfer mode toggle triggered from button
I (xxxxx) MAIN: Current USB state: INACTIVE
I (xxxxx) OLED_SPI: USB Transfer mode toggle requested from display
I (xxxxx) USB_MSC_SD: 📤 Activating USB Transfer mode...
I (xxxxx) USB_MSC_SD: ✓ USB Transfer mode activated
I (xxxxx) MAIN: After toggle: USB state: ACTIVE (ret=ESP_OK)
```

**Expected OLED**:
```
USB Transfer
  [USB]
Connect LEFT port
ACTION: Cancel
```

### Step 4: Second Press (Deactivate)

Press ACTION button again

**Expected serial output**:
```
I (xxxxx) MAIN: 💾 USB Transfer mode toggle triggered from button
I (xxxxx) MAIN: Current USB state: ACTIVE
I (xxxxx) OLED_SPI: USB Transfer mode toggle requested from display
I (xxxxx) USB_MSC_SD: 📥 Deactivating USB Transfer mode...
I (xxxxx) USB_MSC_SD: ✓ Logging mode activated
I (xxxxx) MAIN: After toggle: USB state: INACTIVE (ret=ESP_OK)
```

**Expected OLED**:
```
USB Transfer
Logging resumed
  [OK]
```

### Step 5: Check for Warnings

If button doesn't work, look for:
```
W (xxxxx) MAIN: ⚠️  USB state didn't change! Check OLED handler.
```

This indicates the OLED handler isn't toggling the USB state.

---

## 🔍 Possible Issues

### Issue 1: Button State Not Detected

**Symptom**: No "toggle triggered" message in serial output

**Cause**: Button press not detected by `sensor_manager_is_button_held()`

**Debug**:
```c
// Add this temporarily to see button state
ESP_LOGI(TAG, "Button state: held=%d, pressed=%d", 
         sensor_manager_is_button_held(),
         sensor_manager_is_button_pressed());
```

### Issue 2: OLED Handler Not Toggling

**Symptom**: "USB state didn't change" warning appears

**Cause**: `oled_display_handle_action_button()` not calling USB toggle functions

**Check**: Look at `components/oled_display/oled_display.c` line ~1240

### Issue 3: USB State Check Wrong

**Symptom**: State shows ACTIVE but USB isn't actually active

**Cause**: `usb_msc_sdcard_is_active()` returning wrong value

**Check**: Look at `components/usb_flash_partition/usb_msc_sdcard.c`

### Issue 4: Delays Blocking Button

**Symptom**: Button works but takes long time

**Cause**: `vTaskDelay()` in OLED handler blocking button detection

**Solution**: Reduce delays or make them non-blocking

---

## 🎯 Expected Behavior

### Correct Toggle Sequence

```
State: INACTIVE (logging active)
  ↓
User presses ACTION
  ↓
Button detected: "toggle triggered"
  ↓
OLED handler called
  ↓
USB mode activated: "Activating..."
  ↓
State: ACTIVE (USB active, logging paused)
  ↓
User presses ACTION again
  ↓
Button detected: "toggle triggered"
  ↓
OLED handler called
  ↓
USB mode deactivated: "Deactivating..."
  ↓
State: INACTIVE (logging resumed)
```

---

## 📊 Debug Checklist

After testing, check:

### Button Detection
- [  ] First press detected (see "toggle triggered" message)
- [  ] Second press detected (see "toggle triggered" message)
- [  ] Button state changes (held → not held)

### USB State Changes
- [  ] First press: INACTIVE → ACTIVE
- [  ] Second press: ACTIVE → INACTIVE
- [  ] No "state didn't change" warnings

### OLED Display
- [  ] First press: Shows "Activating..." then "[USB]"
- [  ] Second press: Shows "Deactivating..." then "[OK]"
- [  ] Display updates correctly

### Logging
- [  ] Logging pauses when USB active
- [  ] Logging resumes when USB inactive
- [  ] No crashes or errors

---

## 🔧 If It Still Doesn't Work

### Collect Debug Info

1. **Serial output**: Copy entire output from first press to second press
2. **Button state**: Does button detection work?
3. **USB state**: Does state change after toggle?
4. **OLED display**: What does OLED show?

### Possible Fixes

#### Fix 1: Increase Button Check Frequency

If button presses are missed:

```c
// In main.c, change button check interval
if ((current_time - last_button_check) >= 20) { // Was 50ms, now 20ms
```

#### Fix 2: Remove Delays from OLED Handler

If delays are blocking:

```c
// In oled_display.c, reduce delays
vTaskDelay(pdMS_TO_TICKS(100));  // Was 500ms, now 100ms
```

#### Fix 3: Add Button Debounce

If button bounces:

```c
// Add debounce delay
static uint32_t last_toggle_time = 0;
uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
if ((now - last_toggle_time) < 500) {  // 500ms debounce
    return;  // Ignore rapid presses
}
last_toggle_time = now;
```

---

## 📝 Summary

**Changes made**:
- ✅ Added debug logging to track USB state changes
- ✅ Added verification that state actually changed
- ✅ Added warning if toggle doesn't work

**Next steps**:
1. Build and flash with debug logging
2. Test button toggle (first and second press)
3. Check serial output for state changes
4. Report findings for further debugging

**Expected result**:
- Button should toggle USB mode on/off reliably
- Debug logs will show exactly what's happening
- Can identify where the problem is

---

**Status**: ⚠️ DEBUG VERSION - Test and report findings

