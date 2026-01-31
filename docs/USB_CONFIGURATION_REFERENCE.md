# USB Configuration Reference - Quick Guide

## 📋 Current Configuration

**Mode**: USB Device (TinyUSB)  
**Date**: January 29, 2026  
**Purpose**: USB Transfer mode (Mode 7) - Access SD card from PC

---

## 📂 Backup Files

| File | Purpose | When to Use |
|------|---------|-------------|
| `sdkconfig` | Current configuration (USB Device) | Normal operation |
| `sdkconfig.usb_host_backup` | Original USB Host settings | Restore USB Host mode |
| `sdkconfig.usb_device_backup` | USB Device backup (create when restoring) | Switch back to USB Device |

---

## 🔄 Quick Restore Commands

### Restore USB Host Mode

```bash
# 1. Backup current configuration
copy sdkconfig sdkconfig.usb_device_backup

# 2. Edit sdkconfig - Replace USB-OTG section with settings from sdkconfig.usb_host_backup

# 3. Rebuild
idf.py fullclean
idf.py build flash monitor
```

### Restore USB Device Mode

```bash
# 1. Restore from backup
copy sdkconfig.usb_device_backup sdkconfig

# 2. Rebuild
idf.py fullclean
idf.py build flash monitor
```

---

## 🎯 Configuration Comparison

### USB Device Mode (Current)

**What it does**:
- ESP32 acts as USB device (flash drive)
- PC can detect ESP32
- Can access SD card files from PC

**Use cases**:
- ✅ USB Transfer mode (Mode 7)
- ✅ Access data files from PC
- ✅ Convenient file transfer

**Limitations**:
- ❌ Cannot connect USB devices TO ESP32

**Key setting**:
```
# CONFIG_USB_OTG_SUPPORTED is not set
CONFIG_TINYUSB_MSC_ENABLED=y
```

### USB Host Mode (Backup)

**What it does**:
- ESP32 acts as USB host (like a PC)
- Can connect USB devices TO ESP32
- Can read USB keyboards, mice, flash drives

**Use cases**:
- ✅ Connect USB keyboard to ESP32
- ✅ Connect USB mouse to ESP32
- ✅ Connect USB flash drive to ESP32

**Limitations**:
- ❌ USB Transfer mode doesn't work
- ❌ PC doesn't detect ESP32

**Key setting**:
```
CONFIG_USB_OTG_SUPPORTED=y
CONFIG_USB_HOST_CONTROL_TRANSFER_MAX_SIZE=256
```

---

## ✅ Verification Commands

### Check Current Mode

```bash
# Check if USB Host is enabled
Select-String -Pattern "CONFIG_USB_OTG_SUPPORTED" -Path sdkconfig

# USB Device mode: # CONFIG_USB_OTG_SUPPORTED is not set
# USB Host mode:   CONFIG_USB_OTG_SUPPORTED=y
```

### Check TinyUSB (USB Device)

```bash
Select-String -Pattern "CONFIG_TINYUSB_MSC_ENABLED" -Path sdkconfig

# Should show: CONFIG_TINYUSB_MSC_ENABLED=y
```

---

## 🎯 Recommended Configuration

### For Your Thesis (Current)

**Use**: USB Device mode ✅

**Reasons**:
- USB Transfer mode works
- Easy file access from PC
- No need to remove SD card
- Convenient for data collection

### For Future Projects

**Use**: USB Host mode (if needed)

**Reasons**:
- Need to connect USB devices to ESP32
- Need to read USB keyboard/mouse
- Need to access USB flash drives from ESP32

---

## 📝 Change History

### January 29, 2026
- **Changed**: USB Host → USB Device
- **Reason**: Enable USB Transfer mode (Mode 7)
- **Backup**: `sdkconfig.usb_host_backup` created
- **Impact**: USB Transfer mode now works, PC detects ESP32

---

## 🆘 Quick Help

### Problem: Want to restore USB Host mode

**Solution**: See `USB_HOST_RESTORE_GUIDE.md`

### Problem: Want to switch back to USB Device mode

**Solution**: Restore from `sdkconfig.usb_device_backup`

### Problem: Build fails after changing configuration

**Solution**:
```bash
idf.py fullclean
idf.py build
```

---

## 📞 Files to Read

- `USB_HOST_RESTORE_GUIDE.md` - Detailed restoration guide
- `sdkconfig.usb_host_backup` - Original USB Host settings
- `USB_HOST_DISABLED_SUCCESS.md` - What was changed and why
- `USB_DEVICE_MODE_CLARIFICATION.md` - USB Device vs USB Host explained

---

**Keep this reference for future configuration changes!**

