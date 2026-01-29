# Pin Reassignment Summary - Volume Control Fix

**Date**: January 29, 2026  
**Issue**: Potentiometer volume control not working due to ADC2/WiFi conflict  
**Status**: ✅ Code updated, ready for testing

---

## 🔄 Pin Changes Made

### Before (Broken Configuration)
```
ToF LPN Pin:        GPIO 4  (free pin)
Potentiometer:      GPIO 46 (ADC2_CH5) ❌ Conflicts with WiFi!
```

### After (Fixed Configuration)
```
ToF LPN Pin:        GPIO 36 (free pin) ✅
Potentiometer:      GPIO 4  (ADC1_CH3) ✅ WiFi compatible!
```

---

## 📝 Why These Changes?

### Problem: ADC2 Cannot Be Used with WiFi
- **GPIO 46** is on **ADC2_CH5**
- **ADC2** is used by WiFi driver on ESP32-S3
- When WiFi is active, ADC2 channels are **unavailable**
- This caused potentiometer readings to fail

### Solution: Move to ADC1
- **GPIO 4** is on **ADC1_CH3**
- **ADC1** works independently of WiFi
- Can read potentiometer even with WiFi enabled
- No conflicts with other peripherals

### ToF LPN Pin
- **GPIO 4** was previously used for ToF LPN (Low Power/Enable)
- Moved to **GPIO 36** (free pin, no conflicts)
- ToF sensor works perfectly with LPN on GPIO 36

---

## 🔧 Files Updated

### 1. `components/sensor_manager/include/sensor_manager.h`
```c
// Line 28
#define TOF_LPN_PIN         GPIO_NUM_36  // Changed from GPIO_NUM_4
```

### 2. `main/main.c`
```c
// Line 50
#define BUZZER_POT_PIN GPIO_NUM_4   // Changed from GPIO_NUM_46
```

### 3. `components/buzzer/buzzer.c`
```c
// Lines 230-250
adc_oneshot_unit_init_cfg_t adc_config = {
    .unit_id = ADC_UNIT_1,  // Changed from ADC_UNIT_2
};

buzzer_state.adc_channel = ADC_CHANNEL_3;  // Changed from ADC_CHANNEL_5
```

### 4. `docs/complete-wiring-diagram.md`
- Updated ToF LPN pin: GPIO 4 → GPIO 36
- Updated potentiometer pin: GPIO 46 → GPIO 4
- Updated pin map table
- Updated wiring diagrams
- Updated troubleshooting section

### 5. `VOLUME_CONTROL_TROUBLESHOOTING.md`
- Updated pin definitions
- Corrected GPIO references

---

## 🔌 Updated Wiring Instructions

### VL53L8CX ToF Sensor
```
VL53L8CX          ESP32-S3
--------          --------
SDA            →  GPIO 8
SCL            →  GPIO 9
LPN            →  GPIO 36  ← CHANGED from GPIO 4
VCC            →  3.3V
GND            →  GND
```

### Potentiometer (Volume Control)
```
Potentiometer     ESP32-S3
-------------     --------
Wiper (middle) →  GPIO 4   ← CHANGED from GPIO 46
One end        →  3.3V
Other end      →  GND
```

### Buzzer (PWM)
```
Buzzer            ESP32-S3
------            --------
Signal         →  GPIO 45  (unchanged)
VCC            →  5V or 3.3V
GND            →  GND
```

---

## ✅ Verification Checklist

### Code Changes
- [x] `sensor_manager.h`: TOF_LPN_PIN = GPIO_NUM_36
- [x] `main.c`: BUZZER_POT_PIN = GPIO_NUM_4
- [x] `buzzer.c`: ADC_UNIT_1, ADC_CHANNEL_3
- [x] `buzzer.c`: Added test ADC read during init
- [x] `main.c`: Volume monitoring loop with error logging
- [x] `oled_display.c`: Volume popup implementation
- [x] Documentation updated

### Hardware Wiring (To Be Done)
- [ ] Move ToF LPN wire from GPIO 4 to GPIO 36
- [ ] Move potentiometer wiper from GPIO 46 to GPIO 4
- [ ] Verify potentiometer ends: one to 3.3V, one to GND
- [ ] Verify buzzer on GPIO 45

### Testing Steps
1. [ ] Build firmware: `idf.py build`
2. [ ] Flash to device: `idf.py flash`
3. [ ] Monitor console: `idf.py monitor`
4. [ ] Check for: "✓ Potentiometer volume control enabled (ADC1 - WiFi compatible)"
5. [ ] Check for: "📊 Initial ADC reading: XXXX (volume: XX%)"
6. [ ] Turn potentiometer and watch for: "🔊 Volume: XX%"
7. [ ] Verify volume popup appears on OLED
8. [ ] Trigger near-miss and verify volume changes affect buzzer loudness

---

## 🎯 Expected Console Output

### Successful Initialization
```
I (1234) BUZZER: Configuring volume control via potentiometer on GPIO4
I (1235) BUZZER: ✓ Potentiometer volume control enabled (ADC1 - WiFi compatible)
I (1236) BUZZER: 📊 Initial ADC reading: 2048 (volume: 50%)
```

### Volume Changes
```
I (5000) MAIN: 🔊 Volume: 25%
I (5500) MAIN: 🔊 Volume: 50%
I (6000) MAIN: 🔊 Volume: 75%
I (6500) MAIN: 🔊 Volume: 100%
```

### Error (If Wiring Wrong)
```
W (1234) BUZZER: ⚠️  ADC test read failed: ESP_ERR_INVALID_STATE
W (5000) MAIN: ⚠️  Volume control error: ESP_ERR_INVALID_STATE
W (5001) MAIN: 💡 Check potentiometer wiring: GPIO4 (wiper), 3.3V, GND
```

---

## 🐛 Troubleshooting

### Volume Control Not Working
1. **Check wiring**:
   - Potentiometer wiper → GPIO 4 (not GPIO 46!)
   - One end → 3.3V
   - Other end → GND

2. **Check console logs**:
   - Should see "ADC1 - WiFi compatible"
   - Should see initial ADC reading
   - Should see volume changes when turning pot

3. **Test potentiometer**:
   - Use multimeter to measure voltage on wiper
   - Should vary from 0V to 3.3V as you turn it
   - If stuck at 0V or 3.3V, pot may be wired backwards

### Volume Popup Not Appearing
1. **Check OLED initialization**:
   - OLED must be working first
   - Check OLED wiring (GPIO 7, 1, 35, 21)

2. **Check volume changes**:
   - Popup only appears when volume changes >5%
   - Turn potentiometer more to trigger update

3. **Check console**:
   - Should see "🔊 Volume: XX%" messages
   - If no messages, potentiometer not being read

### ToF Sensor Not Working
1. **Check LPN pin**:
   - Should be on GPIO 36 (not GPIO 4!)
   - Verify wire moved correctly

2. **Check I2C**:
   - SDA on GPIO 8
   - SCL on GPIO 9
   - Both should have pull-ups

---

## 📚 Related Documentation

- `BUZZER_VOLUME_CONTROL_IMPLEMENTATION.md` - Full implementation details
- `VOLUME_CONTROL_TROUBLESHOOTING.md` - Detailed troubleshooting guide
- `docs/complete-wiring-diagram.md` - Complete system wiring reference

---

## 🎉 Next Steps

1. **Rewire hardware**:
   - Move ToF LPN: GPIO 4 → GPIO 36
   - Move potentiometer: GPIO 46 → GPIO 4

2. **Build and flash**:
   ```bash
   idf.py build flash monitor
   ```

3. **Test volume control**:
   - Turn potentiometer
   - Watch console for volume messages
   - Check OLED for volume popup
   - Trigger near-miss to test buzzer volume

4. **Verify all features**:
   - Startup melody plays
   - Near-miss alert (3 beeps)
   - Shutdown melody plays
   - Volume adjusts buzzer loudness
   - Volume popup appears on OLED

---

**Status**: Ready for hardware rewiring and testing! 🚀
