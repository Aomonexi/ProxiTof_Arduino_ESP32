# USB Transfer Mode - Complete Guide

**Consolidated Documentation**  
**Original Dates**: January 29-30, 2026  
**Last Updated**: January 31, 2026

---

## Overview

USB Transfer Mode (Mode 7) provides safe, direct access to SD card files from a PC without removing the SD card. The system automatically pauses data logging, enables USB Mass Storage Class (MSC), and provides clear visual feedback.

### Key Features
- One-button activation/deactivation (ACTION button)
- Automatic logging pause (prevents file corruption)
- Clear visual feedback (inverted headers, USB icon, BLUE LED)
- Safe switching (proper unmount/mount sequence)
- Direct PC access (SD card appears as removable drive)

---

## Quick Start

### Activate USB Transfer Mode
1. Press **MODE button** (GPIO 20) → Navigate to Mode 7 "USB Transfer"
2. Press **ACTION button** (GPIO 3) → Activate USB mode
3. Connect **LEFT USB port** to PC
4. Access files on PC

### Deactivate USB Transfer Mode
1. Safely eject drive from PC
2. Press **ACTION button** (GPIO 3) → Deactivate USB mode
3. Logging resumes automatically

---

## USB Ports (ESP32-S3 DevKitC)

```
┌─────────────────────────┐
│  [LEFT]      [RIGHT]    │
│   USB-OTG    USB-UART   │
│                         │
│   Use LEFT! ────────────┘
└─────────────────────────┘
```

- **LEFT port (USB-OTG)**: USB Mass Storage (for file access)
- **RIGHT port (USB-UART)**: Programming and serial monitoring

**CRITICAL**: Use LEFT port for USB Transfer mode!

---

## Display Screens

### Before Activation (USB MSC Off)
```
┌─────────────────────────┐
│ USB Transfer Mode       │ ← Inverted (white on black)
│ Press ACTION            │ ← Inverted
├─────────────────────────┤
│ This will:              │
│ 1. Pause logging        │
│ 2. Enable USB MSC       │
│ 3. Show USB icon        │
│                         │
│ Press ACTION start      │
└─────────────────────────┘
```

### After Activation (USB MSC On)
```
┌─────────────────────────┐
│ !USB MODE ACTIVE!       │ ← Inverted (white on black)
│ Logging PAUSED          │ ← Inverted
├─────────────────────────┤
│                         │
│   [USB]                 │ ← USB icon (centered)
│                         │
│ Connect LEFT port       │
│ to PC for files         │
│ ACTION to exit          │
└─────────────────────────┘
```

---

## LED Indicators

| State | LED Color | Meaning |
|-------|-----------|---------|
| Normal | Distance-based | Normal operation |
| USB Mode Active | BLUE (solid) | USB MSC active, logging paused |
| Transitioning | BLUE (solid) | Switching modes |

---

## Button Behavior in USB Transfer Mode

| Button | Action |
|--------|--------|
| MODE (GPIO 20) | Cycle to next mode (Shutdown) |
| ACTION (GPIO 3) | Toggle USB MSC on/off |

---

## Operation Sequence

### Activation
1. Navigate to USB Transfer mode (Mode 7)
2. Press ACTION button
3. System displays "Pausing logging..."
4. Logging stops, SD card unmounts from app
5. SD card mounts to USB MSC
6. Display shows "USB MODE ACTIVE"
7. LED turns BLUE
8. Connect LEFT USB port to PC
9. PC detects removable drive

### Deactivation
1. Safely eject drive from PC
2. Press ACTION button
3. System displays "Resuming logging..."
4. SD card unmounts from USB MSC
5. SD card mounts back to app
6. Logging resumes
7. Display shows "USB MODE OFF"
8. LED returns to normal color

---

## Troubleshooting

### PC Doesn't Detect USB Device

**Most Common Cause**: USB cable is power-only (no data lines)

**Solutions** (try in order):
1. ⚠️ **Try 3 different USB cables** (most common fix - 90%)
2. ⚠️ **Verify using LEFT port** (not RIGHT port)
3. ⚠️ **Test cable with Android phone** (verify data transfer works)
4. ⚠️ **Check Device Manager** (Windows) or `lsusb` (Linux)

### Button Doesn't Toggle USB Mode

**Solution**: Build and flash latest firmware
```bash
idf.py build flash monitor
```

### USB MSC Activation Fails

**Possible Causes**:
- USB MSC not initialized at startup
- SD card not available
- TinyUSB driver issue

**Solutions**:
1. Check serial console for error messages
2. Verify SD card is inserted and working
3. Restart device

### Alternative: Use SD Card Reader

If USB Transfer mode doesn't work:
1. Power off device
2. Remove SD card
3. Insert into PC card reader
4. Access files
5. Insert back into device
6. Power on

---

## Technical Details

### Console Configuration
- **Primary console**: UART0 (GPIO 43/44)
- **Secondary console**: None (disabled)
- **USB Serial/JTAG**: Disabled (frees USB-OTG hardware)
- **Monitoring**: Use RIGHT port (USB-UART)

### File System
- **Format**: FAT32 (compatible with all PCs)
- **Access**: Direct sector access via USB MSC
- **Safety**: No simultaneous access (app OR USB, not both)

### Implementation Files
- `components/usb_flash_partition/usb_msc_sdcard.c` - USB MSC implementation
- `components/usb_flash_partition/usb_flash_partition.h` - API definitions

### Key Functions
- `usb_msc_sdcard_init()` - Initialize USB MSC
- `usb_msc_sdcard_pause_logging()` - Activate USB mode
- `usb_msc_sdcard_resume_logging()` - Deactivate USB mode
- `usb_msc_sdcard_is_active()` - Check current state

---

## Mode Navigation

```
Dashboard (0) → Status (1) → ToF Focus (2) → IMU Focus (3) → GPS (4) → 
Near Miss (5) → WiFi Settings (6) → USB Transfer (7) → Shutdown (8) → (back to Dashboard)
```

---

## Safety Features

- **Automatic logging pause**: Prevents file corruption from simultaneous access
- **Proper unmount/mount sequence**: No data loss during transitions
- **User control**: Explicit activation/deactivation required
- **Clear feedback**: Visual and LED indicators

---

## Console Output Examples

### Activation
```
I (12345) USB_MSC_SD: 📤 Switching to USB mode...
I (12345) USB_MSC_SD: 💾 SD card now accessible via USB
I (12345) USB_MSC_SD: 🔌 Connect LEFT port to computer to access files
```

### Deactivation
```
I (23456) USB_MSC_SD: 📥 Switching to APP mode...
I (23456) USB_MSC_SD: 📊 Data logging resumed
```

---

## Related Documentation

- `docs/STORAGE_AND_USB_MSC_EXPLAINED.md` - Storage system overview
- `docs/note-10-usb-host-device-mode-fix.md` - USB mode technical details
- `docs/usb-interface-guide.md` - USB interface commands

---

**Tip**: If USB doesn't work, use SD card reader and focus on your thesis! USB Transfer mode is a convenience feature, not essential for near-miss detection.
