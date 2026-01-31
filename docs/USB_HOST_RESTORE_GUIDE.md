# USB Host Mode - Restore Guide - January 29, 2026

## 📋 Overview

This guide explains how to **restore USB Host mode** if you ever need it in the future.

**Current configuration**: USB Device mode (for USB Transfer mode - Mode 7)  
**Backup configuration**: USB Host mode (stored in `sdkconfig.usb_host_backup`)

---

## ⚠️ Important: What You'll Lose

### If You Restore USB Host Mode

**You will LOSE**:
- ❌ USB Transfer mode (Mode 7) will stop working
- ❌ PC won't detect ESP32 as flash drive
- ❌ Cannot access SD card files via USB

**You will GAIN**:
- ✅ Can connect USB devices TO ESP32 (keyboard, mouse, flash drive)
- ✅ USB Host functionality

**Everything else stays the same**:
- ✅ All sensors work
- ✅ All processing works
- ✅ All logging works
- ✅ Console/monitoring works

---

## 🔄 When to Restore USB Host Mode

### Use Cases for USB Host Mode

1. **Connect USB keyboard to ESP32** - Read keypresses
2. **Connect USB mouse to ESP32** - Read mouse movements
3. **Connect USB flash drive to ESP32** - Read/write files on the drive
4. **Connect USB camera to ESP32** - Capture images
5. **Connect other USB devices to ESP32**

### When NOT to Restore

If you need:
- USB Transfer mode (Mode 7) - Access SD card from PC
- ESP32 as flash drive
- PC to detect ESP32

**Keep current configuration (USB Device mode)!**

---

## 📂 Backup Files

### 1. Original USB Host Configuration
**File**: `sdkconfig.usb_host_backup`  
**Contains**: Original USB Host settings that were disabled  
**Purpose**: Restore USB Host mode if needed

### 2. Current USB Device Configuration
**File**: `sdkconfig` (current)  
**Contains**: USB Device mode settings (TinyUSB)  
**Purpose**: USB Transfer mode (Mode 7)

---

## 🔧 How to Restore USB Host Mode

### Method 1: Manual Edit (Recommended)

#### Step 1: Backup Current Configuration

```bash
# Create backup of current USB Device configuration
copy sdkconfig sdkconfig.usb_device_backup
```

#### Step 2: Open sdkconfig

Open `sdkconfig` in a text editor

#### Step 3: Find USB-OTG Section

Search for:
```
#
# USB-OTG
#
```

#### Step 4: Replace with Original Settings

**Current (USB Device mode)**:
```
#
# USB-OTG
#
# CONFIG_USB_HOST_CONTROL_TRANSFER_MAX_SIZE is not set
# CONFIG_USB_HOST_HW_BUFFER_BIAS_BALANCED is not set
# CONFIG_USB_HOST_HW_BUFFER_BIAS_IN is not set
# CONFIG_USB_HOST_HW_BUFFER_BIAS_PERIODIC_OUT is not set

#
# Hub Driver Configuration
#

#
# Root Port configuration
#
# CONFIG_USB_HOST_DEBOUNCE_DELAY_MS is not set
# CONFIG_USB_HOST_RESET_HOLD_MS is not set
# CONFIG_USB_HOST_RESET_RECOVERY_MS is not set
# CONFIG_USB_HOST_SET_ADDR_RECOVERY_MS is not set
# end of Root Port configuration

# CONFIG_USB_HOST_HUBS_SUPPORTED is not set
# end of Hub Driver Configuration

# CONFIG_USB_HOST_ENABLE_ENUM_FILTER_CALLBACK is not set
# CONFIG_USB_OTG_SUPPORTED is not set
# end of USB-OTG
```

**Replace with (USB Host mode)**:
```
#
# USB-OTG
#
CONFIG_USB_HOST_CONTROL_TRANSFER_MAX_SIZE=256
CONFIG_USB_HOST_HW_BUFFER_BIAS_BALANCED=y
# CONFIG_USB_HOST_HW_BUFFER_BIAS_IN is not set
# CONFIG_USB_HOST_HW_BUFFER_BIAS_PERIODIC_OUT is not set

#
# Hub Driver Configuration
#

#
# Root Port configuration
#
CONFIG_USB_HOST_DEBOUNCE_DELAY_MS=250
CONFIG_USB_HOST_RESET_HOLD_MS=30
CONFIG_USB_HOST_RESET_RECOVERY_MS=30
CONFIG_USB_HOST_SET_ADDR_RECOVERY_MS=10
# end of Root Port configuration

# CONFIG_USB_HOST_HUBS_SUPPORTED is not set
# end of Hub Driver Configuration

# CONFIG_USB_HOST_ENABLE_ENUM_FILTER_CALLBACK is not set
CONFIG_USB_OTG_SUPPORTED=y
# end of USB-OTG
```

#### Step 5: Save and Rebuild

```bash
idf.py fullclean
idf.py build flash monitor
```

### Method 2: Copy from Backup

```bash
# Backup current configuration
copy sdkconfig sdkconfig.usb_device_backup

# Extract USB-OTG section from backup and replace in sdkconfig
# (Manual edit required - see Method 1)
```

---

## ✅ Verification

### After Restoring USB Host Mode

#### 1. Check Configuration

```bash
Select-String -Pattern "CONFIG_USB_OTG_SUPPORTED" -Path sdkconfig
```

**Expected output**:
```
CONFIG_USB_OTG_SUPPORTED=y
```

#### 2. Check USB Host Settings

```bash
Select-String -Pattern "CONFIG_USB_HOST" -Path sdkconfig | Where-Object { $_.Line -notmatch "^#" }
```

**Expected output** (should show USB Host settings):
```
CONFIG_USB_HOST_CONTROL_TRANSFER_MAX_SIZE=256
CONFIG_USB_HOST_HW_BUFFER_BIAS_BALANCED=y
CONFIG_USB_HOST_DEBOUNCE_DELAY_MS=250
CONFIG_USB_HOST_RESET_HOLD_MS=30
CONFIG_USB_HOST_RESET_RECOVERY_MS=30
CONFIG_USB_HOST_SET_ADDR_RECOVERY_MS=10
```

#### 3. Test USB Host Functionality

Connect a USB device (keyboard, mouse, flash drive) to the **LEFT port** and verify it's detected.

---

## 🔄 How to Switch Back to USB Device Mode

If you restored USB Host mode but want to go back to USB Device mode:

### Option 1: Restore from Backup

```bash
# Restore USB Device configuration
copy sdkconfig.usb_device_backup sdkconfig

# Rebuild
idf.py fullclean
idf.py build flash monitor
```

### Option 2: Re-apply USB Device Settings

Follow the same steps as "How to Restore USB Host Mode", but use the USB Device configuration instead.

---

## 📊 Configuration Comparison

| Feature | USB Host Mode | USB Device Mode |
|---------|---------------|-----------------|
| **Connect USB devices TO ESP32** | ✅ Yes | ❌ No |
| **Connect ESP32 TO PC** | ❌ No | ✅ Yes |
| **USB Transfer mode (Mode 7)** | ❌ Doesn't work | ✅ Works |
| **PC detects ESP32 as flash drive** | ❌ No | ✅ Yes |
| **Access SD card from PC** | ❌ No | ✅ Yes |
| **All sensors/processing/logging** | ✅ Yes | ✅ Yes |
| **Console/monitoring** | ✅ Yes | ✅ Yes |

---

## 🎯 Recommended Configuration

### For Your Thesis (Near-Miss Detection)

**Use**: USB Device mode (current configuration)

**Reasons**:
- ✅ USB Transfer mode works (easy file access)
- ✅ Can access SD card files from PC
- ✅ No need to remove SD card
- ✅ Convenient for data collection

**You don't need USB Host mode** because:
- ❌ Not connecting USB devices to ESP32
- ❌ Not using keyboard/mouse with ESP32
- ❌ Not reading USB flash drives with ESP32

### For Future Projects (USB Host)

**Use**: USB Host mode (restore from backup)

**Reasons**:
- ✅ Need to connect USB devices to ESP32
- ✅ Need to read USB keyboard/mouse
- ✅ Need to access USB flash drives from ESP32

---

## 📝 Change Log

### January 29, 2026 - USB Host Disabled

**What changed**:
- Disabled USB Host mode
- Enabled USB Device mode (TinyUSB)
- USB Transfer mode (Mode 7) now works

**Backup created**:
- `sdkconfig.usb_host_backup` - Original USB Host settings

**Reason**:
- PC wasn't detecting ESP32 as flash drive
- USB Host mode was conflicting with TinyUSB
- Needed USB Device mode for USB Transfer mode

---

## 🆘 Troubleshooting

### Problem: Can't Find USB-OTG Section in sdkconfig

**Solution**: Search for "USB-OTG" or "CONFIG_USB_OTG_SUPPORTED"

### Problem: Build Fails After Restoring

**Solution**:
```bash
idf.py fullclean
idf.py build
```

### Problem: USB Host Mode Doesn't Work After Restoring

**Verify**:
1. Check `CONFIG_USB_OTG_SUPPORTED=y` in sdkconfig
2. Check USB Host settings are present
3. Run `idf.py fullclean` and rebuild
4. Connect USB device to **LEFT port** (USB-OTG)

### Problem: Want Both USB Host and USB Device

**Not possible**: ESP32-S3 can only be in one mode at a time:
- USB Host mode: Connect devices TO ESP32
- USB Device mode: Connect ESP32 TO PC

**Workaround**: Switch configurations as needed and rebuild

---

## 📞 Summary

### Current Configuration (USB Device Mode)
- ✅ USB Transfer mode works
- ✅ PC detects ESP32 as flash drive
- ✅ Can access SD card files from PC
- ❌ Cannot connect USB devices to ESP32

### Backup Configuration (USB Host Mode)
- ✅ Can connect USB devices to ESP32
- ❌ USB Transfer mode doesn't work
- ❌ PC doesn't detect ESP32 as flash drive

### Backup Files
- `sdkconfig.usb_host_backup` - Original USB Host settings
- `sdkconfig.usb_device_backup` - Current USB Device settings (create when restoring)

### To Restore USB Host Mode
1. Backup current sdkconfig
2. Replace USB-OTG section with settings from `sdkconfig.usb_host_backup`
3. Run `idf.py fullclean`
4. Run `idf.py build flash monitor`
5. USB Host mode restored (USB Transfer mode will stop working)

---

**Keep this guide for future reference!**

