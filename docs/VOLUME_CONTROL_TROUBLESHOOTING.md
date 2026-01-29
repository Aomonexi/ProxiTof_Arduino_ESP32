# Volume Control Troubleshooting Guide

**Date**: January 28, 2026  
**Issue**: Potentiometer not adjusting volume or showing popup  
**Status**: Debugging in progress

---

## What's Working ✅
- ✅ Buzzer sounds (startup, near-miss, button beeps)
- ✅ Near-miss 3 beeps working
- ✅ Startup melody working
- ✅ Shutdown melody added (descending tones)

## What's Not Working ❌
- ❌ Potentiometer volume control
- ❌ Volume popup not appearing on OLED

---

## Possible Causes

### 1. ADC2 Conflict with WiFi (Most Likely)
**Problem**: ESP32-S3 ADC2 cannot be used when WiFi is active

**Check**:
```
Look for this in console logs:
I (5000) BUZZER: ⚠️  ADC test read failed: ESP_ERR_TIMEOUT
I (5000) BUZZER: 💡 ADC2 may be in use by WiFi - volume control disabled
```

**Solution**: Use ADC1 instead of ADC2

**GPIO Options for ADC1**:
- GPIO 1-10 are ADC1 channels
- **Recommended**: GPIO 4 (ADC1_CH3) - Currently used for ToF LPN
- **Alternative**: GPIO 5 (ADC1_CH4) - Currently used for IMU INT

**Problem**: All good ADC1 pins are already in use!

**Best Solution**: Disable WiFi during normal operation, only enable for sync

### 2. Potentiometer Not Connected
**Check**:
- Wiper (middle pin) → GPIO 46
- One end → 3.3V
- Other end → GND

**Test**: Measure voltage on wiper with multimeter (should vary 0-3.3V)

### 3. Wrong ADC Channel
**Check**: GPIO 46 = ADC2_CH5 (correct)

### 4. Volume Monitoring Not Running
**Check**: Look for volume logs in console:
```
I (10000) MAIN: 🔊 Volume: 50%
```

If not appearing, volume monitoring code isn't running.

---

## Debug Steps

### Step 1: Check Console Logs
Flash the new firmware and look for these messages:

**Buzzer Initialization**:
```
I (5000) MAIN: Initializing buzzer (PWM audio alert)...
I (5000) BUZZER: Initializing buzzer on GPIO45
I (5000) BUZZER: Configuring volume control via potentiometer on GPIO46
I (5000) BUZZER: ✓ Potentiometer volume control enabled
I (5000) BUZZER: 📊 Initial ADC reading: 2048 (volume: 50%)
I (5000) MAIN: ✓ Buzzer initialized (freq=2000 Hz, volume=50%)
```

**If ADC Fails**:
```
I (5000) BUZZER: ⚠️  ADC test read failed: ESP_ERR_TIMEOUT
I (5000) BUZZER: 💡 ADC2 may be in use by WiFi - volume control disabled
```

**Volume Monitoring** (should appear every time you turn pot):
```
I (10000) MAIN: 🔊 Volume: 75%
I (10500) MAIN: 🔊 Volume: 80%
```

**If Volume Monitoring Fails**:
```
W (10000) MAIN: ⚠️  Volume control error: ESP_ERR_INVALID_STATE
W (10000) MAIN: 💡 Check potentiometer wiring: GPIO46 (wiper), 3.3V, GND
```

### Step 2: Test Potentiometer Hardware
```
1. Power off device
2. Set multimeter to DC voltage mode
3. Connect black probe to GND
4. Connect red probe to GPIO 46 (wiper)
5. Power on device
6. Turn potentiometer slowly
7. Voltage should vary smoothly from 0V to 3.3V
```

If voltage doesn't change → Potentiometer wiring issue

### Step 3: Check WiFi Status
```
Look for WiFi initialization in logs:
I (2000) WIFI: WiFi initialized
I (2000) WIFI: Connecting to SSID...
```

If WiFi is active → ADC2 won't work

### Step 4: Manual Volume Test
Add this to main.c after buzzer init:
```c
// Test volume control manually
ESP_LOGI(TAG, "Testing volume control...");
for (int vol = 0; vol <= 100; vol += 25) {
    buzzer_set_volume(vol);
    ESP_LOGI(TAG, "Volume set to: %d%%", vol);
    buzzer_play_tone(2000, 200);  // Play test tone
    vTaskDelay(pdMS_TO_TICKS(500));
}
```

This will play tones at different volumes to verify volume control works.

---

## Solutions

### Solution 1: Disable WiFi (Recommended)
**Modify**: `data/config.txt`
```
WIFI_ENABLED=false
```

This frees up ADC2 for potentiometer use.

**Trade-off**: No automatic time sync, must sync manually via USB

### Solution 2: Use ADC1 Pin (Requires Rewiring)
**Problem**: All good ADC1 pins are in use

**Option A**: Move ToF LPN from GPIO 4 to another pin
- ToF LPN is just a reset/enable pin
- Can use any free GPIO
- **Recommended**: GPIO 19 (currently free)

**Wiring Change**:
```
OLD: ToF LPN → GPIO 4
NEW: ToF LPN → GPIO 19
     Potentiometer → GPIO 4 (ADC1_CH3)
```

**Code Change**:
```c
// In main/main.c
#define TOF_LPN_PIN GPIO_NUM_36  // Changed from 4 (now used for potentiometer)
#define BUZZER_POT_PIN GPIO_NUM_4  // Changed from 46 (now ADC1 - WiFi compatible)
```

### Solution 3: Use Digital Volume Control
**Remove potentiometer**, use buttons instead:
- Button 1: Volume Up
- Button 2: Volume Down
- Store volume in NVS (non-volatile storage)

**Pros**: No ADC conflicts, works with WiFi
**Cons**: Less intuitive than analog pot

### Solution 4: WiFi On-Demand Only
**Keep WiFi disabled by default**, enable only when needed:
- Press button to enter WiFi mode
- Sync time
- Disable WiFi
- Potentiometer works again

---

## Recommended Fix (Easiest)

### Option 1: Disable WiFi
1. Edit `data/config.txt` on SD card
2. Set `WIFI_ENABLED=false`
3. Restart device
4. Potentiometer should work now

### Option 2: Move ToF LPN Pin
1. Disconnect ToF LPN from GPIO 4
2. Connect ToF LPN to GPIO 19
3. Connect potentiometer wiper to GPIO 4
4. Update code:
```c
// In sensor_manager or wherever ToF is initialized
#define TOF_LPN_PIN GPIO_NUM_36  // Changed from 4 (now used for potentiometer)

// In main.c
#define BUZZER_POT_PIN GPIO_NUM_4  // Changed from 46 (ADC1 - WiFi compatible)
```
5. Rebuild and flash

---

## Testing After Fix

### Test 1: Check Console Logs
```
Expected:
I (5000) BUZZER: ✓ Potentiometer volume control enabled
I (5000) BUZZER: 📊 Initial ADC reading: XXXX (volume: XX%)
```

### Test 2: Turn Potentiometer
```
Expected:
I (10000) MAIN: 🔊 Volume: 25%
I (10500) MAIN: 🔊 Volume: 50%
I (11000) MAIN: 🔊 Volume: 75%
```

### Test 3: Check OLED Popup
```
Expected:
- Popup appears when volume changes
- Shows "VOL:XX%" with progress bar
- Auto-hides after 2 seconds
```

### Test 4: Volume Actually Changes
```
Test:
1. Set volume to 25% (turn pot CCW)
2. Trigger near-miss
3. Beeps should be quiet
4. Set volume to 100% (turn pot CW)
5. Trigger near-miss
6. Beeps should be loud
```

---

## Current Status

**Build**: ✅ Successful  
**Shutdown Sound**: ✅ Added (descending tones)  
**Error Logging**: ✅ Enhanced  
**ADC Test**: ✅ Added to initialization  

**Next**: Flash firmware and check console logs for ADC errors

---

## Quick Reference

### Console Log Checklist
- [ ] "✓ Buzzer initialized"
- [ ] "✓ Potentiometer volume control enabled"
- [ ] "📊 Initial ADC reading: XXXX"
- [ ] "🔊 Volume: XX%" (when turning pot)
- [ ] No "ADC test read failed" errors
- [ ] No "Volume control error" warnings

### Hardware Checklist
- [ ] Potentiometer wiper → GPIO 46
- [ ] Potentiometer end 1 → 3.3V
- [ ] Potentiometer end 2 → GND
- [ ] Voltage on wiper varies 0-3.3V when turned
- [ ] WiFi disabled in config.txt

---

**Status**: Ready to flash and test  
**Expected Issue**: ADC2/WiFi conflict  
**Recommended Fix**: Disable WiFi or move to ADC1 pin
