# Note 10: USB Host/Device Mode Fix - USB Transfer Mode Working

**Date**: January 29, 2026  
**Status**: ✅ RESOLVED  
**Impact**: USB Transfer mode (Mode 7) now works - PC detects ESP32 as flash drive

---

## 🎯 Problem Summary

**Issue**: PC didn't detect ESP32 as USB flash drive when USB Transfer mode was activated.

**Symptoms**:
- Software correctly activated USB mode (`MSC_active=YES`)
- But PC didn't detect device (`USB_connected=NO`)
- No new drive appeared in File Explorer
- Device Manager showed nothing

**Root Cause**: USB Host mode was enabled in `sdkconfig`, which conflicts with USB Device mode (TinyUSB MSC).

---

## 🔍 Technical Analysis

### USB Host vs USB Device

The ESP32-S3 USB-OTG peripheral can operate in two modes:

#### USB Host Mode
- ESP32 acts as **host** (like a PC)
- Can connect USB devices **TO** ESP32 (keyboard, mouse, flash drive)
- Uses USB Host drivers
- **Conflicts with TinyUSB (USB Device mode)**

#### USB Device Mode
- ESP32 acts as **device** (like a flash drive)
- Can connect ESP32 **TO** PC
- Uses TinyUSB drivers
- **Required for USB Transfer mode**

**Cannot use both modes simultaneously!**

### Configuration Issue

The `sdkconfig` had USB Host mode enabled:
```
CONFIG_USB_OTG_SUPPORTED=y
CONFIG_USB_HOST_CONTROL_TRANSFER_MAX_SIZE=256
CONFIG_USB_HOST_HW_BUFFER_BIAS_BALANCED=y
CONFIG_USB_HOST_DEBOUNCE_DELAY_MS=250
```

This prevented TinyUSB (USB Device mode) from working properly, even though TinyUSB was enabled:
```
CONFIG_TINYUSB_MSC_ENABLED=y
```

---

## ✅ Solution

### 1. Disabled USB Host Mode

Changed `sdkconfig` to disable USB Host:

**Before**:
```
CONFIG_USB_OTG_SUPPORTED=y
CONFIG_USB_HOST_CONTROL_TRANSFER_MAX_SIZE=256
CONFIG_USB_HOST_HW_BUFFER_BIAS_BALANCED=y
CONFIG_USB_HOST_DEBOUNCE_DELAY_MS=250
CONFIG_USB_HOST_RESET_HOLD_MS=30
CONFIG_USB_HOST_RESET_RECOVERY_MS=30
CONFIG_USB_HOST_SET_ADDR_RECOVERY_MS=10
```

**After**:
```
# CONFIG_USB_OTG_SUPPORTED is not set
# CONFIG_USB_HOST_CONTROL_TRANSFER_MAX_SIZE is not set
# CONFIG_USB_HOST_HW_BUFFER_BIAS_BALANCED is not set
# CONFIG_USB_HOST_DEBOUNCE_DELAY_MS is not set
# CONFIG_USB_HOST_RESET_HOLD_MS is not set
# CONFIG_USB_HOST_RESET_RECOVERY_MS is not set
# CONFIG_USB_HOST_SET_ADDR_RECOVERY_MS is not set
```

### 2. Kept TinyUSB Enabled

TinyUSB (USB Device mode) remained enabled:
```
CONFIG_TINYUSB_MSC_ENABLED=y
CONFIG_TINYUSB_MSC_BUFSIZE=4096
CONFIG_TINYUSB_MSC_MOUNT_PATH="/sdcard"
```

### 3. Created Backup

Original USB Host settings backed up to `sdkconfig.usb_host_backup` for future restoration if needed.

---

## 🚀 Results

### What Works Now ✅

1. ✅ **USB Transfer mode (Mode 7) works!**
   - Press ACTION button → USB mode activates
   - Connect LEFT port to PC → PC detects USB flash drive
   - New drive appears (e.g., "D:", "E:", "F:")
   - Can access SD card files from PC
   - Press ACTION button → USB mode deactivates (needs fix - see below)
   - Logging resumes

2. ✅ **PC Detection**
   - Windows: "USB Mass Storage Device" in Device Manager
   - New drive letter appears
   - Can read/write files

3. ✅ **All Other Functionality Preserved**
   - All sensors work (ToF, IMU, GPS, Buzzer, OLED, LED)
   - All processing works (near-miss detection, algorithms)
   - All data logging works (SD card, CSV files, flags)
   - Console/monitoring works (RIGHT port, `idf.py monitor`)
   - Programming works (RIGHT port, `idf.py flash`)

### What Changed 🔄

1. 🔄 **USB-OTG behavior**
   - Before: USB Host mode (can connect devices TO ESP32)
   - After: USB Device mode (can connect ESP32 TO PC)

2. 🔄 **LEFT port function**
   - Before: Could connect USB devices to ESP32 (not used)
   - After: Can connect ESP32 to PC as flash drive (needed!)

### What Was Lost ❌

1. ❌ **USB Host functionality**
   - Cannot connect USB devices TO ESP32 anymore
   - Cannot read USB keyboards, mice, flash drives with ESP32
   - **Not needed for thesis work**

---

## 🔧 Implementation Details

### Files Modified

1. **`sdkconfig`** - Disabled USB Host mode
   - Lines 2260-2287 (USB-OTG section)
   - Changed USB Host settings to disabled

### Files Created

1. **`sdkconfig.usb_host_backup`** - Backup of original USB Host settings
2. **`USB_HOST_RESTORE_GUIDE.md`** - How to restore USB Host mode if needed
3. **`USB_CONFIGURATION_REFERENCE.md`** - Quick reference for USB configurations
4. **`USB_HOST_DISABLED_SUCCESS.md`** - What was changed and why
5. **`USB_DEVICE_MODE_CLARIFICATION.md`** - USB Device vs USB Host explained

### Build Process

```bash
idf.py fullclean
idf.py build flash monitor
```

**Why fullclean?** USB Host/Device settings affect low-level USB drivers. Must recompile everything.

---

## 📊 Technical Details

### USB-OTG Hardware

The ESP32-S3 has native USB-OTG hardware that supports both Host and Device modes:
- **Hardware**: USB-OTG peripheral (LEFT port)
- **Software**: Can be configured as Host OR Device (not both)

### Two USB Ports

```
ESP32-S3 DevKitC:
┌─────────────────────────────────────┐
│  [LEFT PORT]      [RIGHT PORT]     │
│   USB-OTG         USB-UART         │
│   (Native)        (Bridge chip)    │
│                                     │
│   For USB MSC     For programming  │
│   Device mode     Always works     │
└─────────────────────────────────────┘
```

- **LEFT port**: USB-OTG (configurable as Host or Device)
- **RIGHT port**: USB-to-UART bridge (always works for programming/monitoring)

### Console Configuration

Console is on **UART0** via **RIGHT port** (USB-UART bridge):
- Primary console: UART0 (GPIO 43 TX, GPIO 44 RX)
- Secondary console: None (disabled)
- USB Serial/JTAG: Disabled (frees USB-OTG hardware)

This configuration keeps console/monitoring separate from USB-OTG functionality.

---

## 🎯 Use Cases

### Current Configuration (USB Device Mode)

**Best for**:
- ✅ USB Transfer mode (Mode 7)
- ✅ Accessing SD card files from PC
- ✅ Data collection and analysis
- ✅ Thesis work (near-miss detection)

**Not suitable for**:
- ❌ Connecting USB devices to ESP32
- ❌ Reading USB keyboards/mice with ESP32

### Alternative Configuration (USB Host Mode)

**Best for**:
- ✅ Connecting USB devices to ESP32
- ✅ Reading USB keyboards/mice
- ✅ Accessing USB flash drives from ESP32

**Not suitable for**:
- ❌ USB Transfer mode (Mode 7)
- ❌ PC detecting ESP32 as flash drive

**To restore**: See `USB_HOST_RESTORE_GUIDE.md`

---

## 🐛 Known Issues

### 1. ACTION Button Doesn't Exit USB Transfer Mode

**Status**: ⚠️ IN PROGRESS  
**Issue**: Second press of ACTION button doesn't deactivate USB mode  
**Workaround**: Restart device or change mode  
**Fix**: Being implemented (button toggle logic)

---

## 📚 Related Documentation

- `USB_HOST_RESTORE_GUIDE.md` - How to restore USB Host mode
- `USB_CONFIGURATION_REFERENCE.md` - Quick reference
- `USB_DEVICE_MODE_CLARIFICATION.md` - USB modes explained
- `USB_BUTTON_TOGGLE_FIX_JAN29.md` - Button toggle fix (partial)
- `USB_TRANSFER_COMPLETE_STATUS_JAN29.md` - Complete status

---

## 🎓 Lessons Learned

### 1. USB Host vs Device Conflict

USB Host and USB Device modes cannot coexist. Must choose one:
- **Host**: Connect devices TO ESP32
- **Device**: Connect ESP32 TO PC

### 2. Menuconfig Limitations

Some settings in menuconfig cannot be toggled if they're locked by dependencies. Direct `sdkconfig` editing is sometimes necessary.

### 3. Clean Build Required

USB configuration changes require `idf.py fullclean` to properly recompile low-level drivers.

### 4. Hardware vs Software

- **Hardware capability**: ESP32-S3 HAS USB-OTG hardware (can do both Host and Device)
- **Software configuration**: Must CHOOSE Host OR Device mode in software

### 5. Backup Important

Always backup original configuration before making changes. Created `sdkconfig.usb_host_backup` for future restoration.

---

## ✅ Success Criteria Met

- [x] PC detects ESP32 as USB flash drive
- [x] New drive appears in File Explorer
- [x] Can access SD card files from PC
- [x] Can read files (CSV logs)
- [x] Can write files (config.txt)
- [x] All other functionality preserved
- [x] Console/monitoring still works
- [ ] ACTION button exits USB mode (in progress)

---

## 🚀 Next Steps

1. ✅ **USB Transfer mode working** - DONE!
2. ⚠️ **Fix ACTION button toggle** - IN PROGRESS
3. 📝 **Document complete workflow** - IN PROGRESS
4. 🧪 **Test data collection workflow** - TODO

---

## 📝 Conclusion

Disabling USB Host mode and using USB Device mode (TinyUSB) successfully enabled USB Transfer mode. The ESP32 now acts as a USB flash drive, allowing convenient access to SD card files from a PC without removing the SD card.

This configuration is optimal for the thesis work (near-miss detection) as it provides easy data access while preserving all sensor, processing, and logging functionality.

**Status**: ✅ USB Transfer mode working! PC detects ESP32 as flash drive.

---

**Author**: Kiro AI Assistant  
**Date**: January 29, 2026  
**Version**: 1.0

