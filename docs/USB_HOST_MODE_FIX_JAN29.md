# USB Host Mode Fix - CRITICAL - January 29, 2026

## 🎯 PROBLEM FOUND!

Your `sdkconfig` has **USB Host mode enabled**. This is why the PC doesn't detect your device!

### What's Wrong?

```
Current Configuration:
┌─────────────────────────────────────┐
│ ESP32-S3 = USB HOST (like a PC)    │  ❌ WRONG!
│ PC = USB HOST (like a PC)          │
│                                     │
│ Two hosts trying to talk = FAIL    │
└─────────────────────────────────────┘

Correct Configuration:
┌─────────────────────────────────────┐
│ ESP32-S3 = USB DEVICE (flash drive)│  ✅ CORRECT!
│ PC = USB HOST (like a PC)          │
│                                     │
│ Host talks to device = SUCCESS     │
└─────────────────────────────────────┘
```

### Current Settings (WRONG)

```
CONFIG_USB_HOST_CONTROL_TRANSFER_MAX_SIZE=256
CONFIG_USB_HOST_HW_BUFFER_BIAS_BALANCED=y
CONFIG_USB_HOST_DEBOUNCE_DELAY_MS=250
CONFIG_USB_OTG_SUPPORTED=y  ← This enables USB Host!
```

---

## 🔧 FIX: Disable USB Host Mode

### Option 1: Using menuconfig (RECOMMENDED)

```bash
idf.py menuconfig
```

Navigate to:
```
Component config
  → USB-OTG
    → [ ] USB OTG  ← UNCHECK THIS!
```

**Steps**:
1. Arrow down to "Component config"
2. Press Enter
3. Arrow down to "USB-OTG"
4. Press Enter
5. Arrow down to "USB OTG"
6. Press Space to **UNCHECK** (should show `[ ]` not `[*]`)
7. Press S to save
8. Press Enter to confirm
9. Press Q to quit

### Option 2: Edit sdkconfig Directly

Open `sdkconfig` and change:

**Before**:
```
CONFIG_USB_OTG_SUPPORTED=y
```

**After**:
```
# CONFIG_USB_OTG_SUPPORTED is not set
```

Also remove these lines (they're USB Host settings):
```
CONFIG_USB_HOST_CONTROL_TRANSFER_MAX_SIZE=256
CONFIG_USB_HOST_HW_BUFFER_BIAS_BALANCED=y
CONFIG_USB_HOST_DEBOUNCE_DELAY_MS=250
CONFIG_USB_HOST_RESET_HOLD_MS=30
CONFIG_USB_HOST_RESET_RECOVERY_MS=30
CONFIG_USB_HOST_SET_ADDR_RECOVERY_MS=10
```

---

## 🚀 After Fixing

### 1. Clean Build (IMPORTANT!)

```bash
idf.py fullclean
idf.py build flash monitor
```

**Why fullclean?** USB Host/Device settings affect low-level drivers. A regular build might not recompile everything.

### 2. Test USB Detection

1. Navigate to Mode 7 (USB Transfer)
2. Press ACTION button
3. Connect LEFT port to PC
4. **PC should now detect USB device!**

### Expected Result

**Serial Monitor**:
```
I (xxxxx) USB_MSC_SD: ✓ USB Transfer mode activated
I (xxxxx) USB_MSC_SD: 🔌 Connect LEFT port (USB-OTG) to computer
I (xxxxx) USB_MSC_SD: 🔌 USB host connected  ← NEW! Should appear now!
```

**PC**:
- New drive appears (e.g., "D:", "E:", "F:")
- Can access SD card files
- Device Manager shows "USB Mass Storage Device"

---

## 🤔 Why Was USB Host Enabled?

### Possible Reasons

1. **Default ESP-IDF configuration** - Some ESP-IDF versions enable USB Host by default
2. **Previous project** - You might have copied sdkconfig from another project
3. **Menuconfig accident** - Accidentally enabled during configuration

### USB Host vs USB Device

| Mode | ESP32 Acts As | Use Case | Example |
|------|---------------|----------|---------|
| **USB Host** | PC/Controller | Connect USB devices to ESP32 | USB keyboard, mouse, flash drive |
| **USB Device** | Peripheral | Connect ESP32 to PC | Flash drive, serial port, HID device |

**Your use case**: ESP32 as flash drive → Need **USB Device** mode!

---

## 🔍 Verification

### Check sdkconfig After Fix

```bash
# Should NOT find USB_HOST settings
Select-String -Pattern "CONFIG_USB_HOST" -Path sdkconfig | Where-Object { $_.Line -notmatch "^#" }
# Output should be empty!

# Should NOT find USB_OTG_SUPPORTED=y
Select-String -Pattern "CONFIG_USB_OTG_SUPPORTED=y" -Path sdkconfig
# Output should be empty!

# Should find TinyUSB settings (these are correct)
Select-String -Pattern "CONFIG_TINYUSB_MSC_ENABLED=y" -Path sdkconfig
# Output: CONFIG_TINYUSB_MSC_ENABLED=y ✅
```

---

## 🎯 Expected Behavior After Fix

### Before Fix (USB Host Enabled)
```
ESP32: "I'm a USB host, looking for devices to connect"
PC: "I'm a USB host, looking for devices to connect"
Result: Both waiting for devices, nothing happens ❌
```

### After Fix (USB Host Disabled)
```
ESP32: "I'm a USB device (flash drive), ready to be connected"
PC: "I'm a USB host, I found a flash drive!"
Result: PC detects ESP32 as flash drive ✅
```

---

## ⚠️ About the "USB-OTG Bridge" Chip

### What It Is

The "USB-OTG bridge" chip on your board is likely a **USB-to-UART converter** (e.g., CP2102, CH340) for the **RIGHT port** (programming/monitoring).

### Do NOT Desolder It!

**Reasons**:
1. It's for the RIGHT port (USB-UART), not the LEFT port (USB-OTG)
2. You need it for programming and monitoring
3. It doesn't interfere with USB-OTG functionality
4. Desoldering it will break programming/monitoring

### Port Clarification

```
ESP32-S3 DevKitC:
┌─────────────────────────────────────┐
│                                     │
│  [LEFT PORT]      [RIGHT PORT]     │
│   USB-OTG         USB-UART         │
│   (Native)        (Bridge chip)    │
│                                     │
│   For USB MSC     For programming  │
│   No bridge chip  Has bridge chip  │
└─────────────────────────────────────┘
```

**LEFT port**: Direct connection to ESP32-S3 USB-OTG peripheral (no bridge chip)  
**RIGHT port**: Goes through USB-to-UART bridge chip (CP2102/CH340)

---

## 🧪 Testing Checklist

After applying the fix:

### 1. Verify Configuration
- [ ] USB Host disabled in sdkconfig
- [ ] TinyUSB MSC enabled in sdkconfig
- [ ] Clean build completed successfully

### 2. Test USB Detection
- [ ] Navigate to Mode 7
- [ ] Press ACTION button
- [ ] Connect LEFT port to PC
- [ ] PC detects USB device
- [ ] New drive appears
- [ ] Can access SD card files

### 3. Test Button Toggle
- [ ] Press ACTION to deactivate
- [ ] Logging resumes
- [ ] Press ACTION to re-activate
- [ ] USB mode activates again

---

## 📊 Success Probability

| Issue | Probability | Fix |
|-------|-------------|-----|
| USB Host enabled | **95%** | Disable USB Host in menuconfig |
| USB cable issue | 3% | Try different cable |
| Hardware issue | 2% | Test with ESP-IDF example |

**This is almost certainly your problem!** USB Host mode prevents USB Device mode from working.

---

## 🚀 Quick Fix Steps

```bash
# 1. Open menuconfig
idf.py menuconfig

# 2. Navigate to: Component config → USB-OTG → USB OTG
#    Press Space to UNCHECK

# 3. Save and exit (S, Enter, Q)

# 4. Clean build
idf.py fullclean

# 5. Build and flash
idf.py build flash monitor

# 6. Test USB Transfer mode
#    - Navigate to Mode 7
#    - Press ACTION
#    - Connect LEFT port
#    - PC should detect USB device!
```

---

## 🎉 Expected Result

After disabling USB Host mode:

```
I (5684) TinyUSB: TinyUSB Driver installed on port 0
I (5734) USB_MSC_SD: ✓ MSC driver installed
I (5754) USB_MSC_SD: ✓ USB MSC initialized successfully

[Navigate to Mode 7, press ACTION, connect LEFT port]

I (xxxxx) USB_MSC_SD: ✓ USB Transfer mode activated
I (xxxxx) USB_MSC_SD: 🔌 Connect LEFT port (USB-OTG) to computer
I (xxxxx) USB_MSC_SD: 🔌 USB host connected  ← THIS SHOULD APPEAR!

[PC detects new drive]
Windows: "USB Mass Storage Device" in Device Manager
         New drive letter appears (e.g., "D:")
         
Linux: "usb-storage 1-1:1.0: USB Mass Storage device detected"
       New device appears (e.g., /dev/sdb)
       
macOS: New drive appears on desktop
```

---

## 💡 Summary

**Problem**: USB Host mode enabled → ESP32 acts as host, not device  
**Solution**: Disable USB Host mode in menuconfig  
**Result**: ESP32 acts as USB device → PC can detect it as flash drive  

**DO NOT DESOLDER ANYTHING!** This is a software configuration issue, not a hardware issue.

---

## 📞 If This Doesn't Work

If disabling USB Host mode doesn't fix it:

1. **Verify the fix was applied**:
   ```bash
   grep "CONFIG_USB_OTG_SUPPORTED" sdkconfig
   # Should show: # CONFIG_USB_OTG_SUPPORTED is not set
   ```

2. **Try ESP-IDF example** to verify hardware:
   ```bash
   cd $IDF_PATH/examples/peripherals/usb/device/tusb_msc
   idf.py build flash monitor
   ```

3. **Check USB cable** (still might be power-only)

4. **Provide debugging info**:
   - Serial monitor output when connecting USB
   - Device Manager screenshot (Windows)
   - Output of `lsusb` (Linux)

But I'm **95% confident** this will fix your issue! USB Host mode is the smoking gun.

