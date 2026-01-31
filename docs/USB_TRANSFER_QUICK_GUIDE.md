# USB Transfer Mode - Quick Reference Guide

**Date**: January 29, 2026  
**Status**: ✅ Ready to build and test

---

## What Was Done

Added a dedicated **USB Transfer Mode** (Mode 7) that allows you to access SD card files directly from your PC without removing the SD card.

### Key Features

✅ **One-button activation** - Press ACTION button to toggle USB MSC on/off  
✅ **Automatic logging pause** - System automatically stops logging when USB mode is active  
✅ **Clear visual feedback** - Inverted headers, USB icon, and BLUE LED  
✅ **Safe switching** - Proper unmount/mount sequence prevents data corruption  
✅ **No SD card removal** - Access files via USB cable  

---

## How to Use

### Step 1: Navigate to USB Transfer Mode
- Press **MODE button** (GPIO 20) repeatedly
- Cycle through modes until you see "USB Transfer Mode"

### Step 2: Activate USB MSC
- Press **ACTION button** (GPIO 3)
- Display shows "Pausing logging..."
- Display changes to "!USB MODE ACTIVE!"
- LED turns **BLUE**
- Logging is now **PAUSED**

### Step 3: Connect to PC
- Connect **LEFT USB port** to your PC
- PC will detect a removable drive
- You can now read/write files directly

### Step 4: Deactivate USB MSC
- Safely eject the drive from your PC
- Press **ACTION button** (GPIO 3) again
- Display shows "Resuming logging..."
- Display changes to "USB MODE OFF"
- LED returns to normal color
- Logging **RESUMES**

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

## Mode Order (Updated)

```
Dashboard (0)
  ↓
Status (1)
  ↓
ToF Focus (2)
  ↓
IMU Focus (3)
  ↓
GPS (4)
  ↓
Near Miss (5)
  ↓
WiFi Settings (6)
  ↓
USB Transfer (7) ← NEW!
  ↓
Shutdown (8)
  ↓
(back to Dashboard)
```

---

## Button Behavior

### Button 1 (GPIO 20) - Mode Switch
- **Action:** Cycle to next mode
- **From USB Transfer:** Goes to Shutdown mode
- **Note:** Does NOT deactivate USB MSC if active

### Button 2 (GPIO 3) - Action
- **When USB MSC Off:** Activates USB MSC mode
- **When USB MSC On:** Deactivates USB MSC mode
- **LED:** Turns BLUE during USB mode

---

## LED Indicators

| State | LED Color | Meaning |
|-------|-----------|---------|
| Normal | Distance-based | Normal operation |
| USB Mode | BLUE (solid) | USB MSC active, logging paused |

---

## Safety Features

✅ **Automatic logging pause** - Prevents file corruption  
✅ **Clear visual feedback** - Inverted headers, USB icon, LED  
✅ **Safe switching** - Proper unmount/mount sequence  
✅ **User control** - Explicit activation/deactivation required  

---

## Files Modified

1. `components/oled_display/include/oled_display.h`
   - Added `OLED_MODE_USB_TRANSFER` enum

2. `components/oled_display/oled_display.c`
   - Added USB Transfer mode display logic
   - Added USB Transfer mode button handling

3. `main/main.c`
   - Added USB Transfer mode button handling
   - Updated mode names array

---

## Build Instructions

```bash
# Setup ESP-IDF environment
.\setup_idf.ps1

# Build project
idf.py build

# Flash to device
idf.py flash monitor
```

---

## Testing Checklist

### Basic Functionality
- [ ] USB Transfer mode appears in mode cycle (after WiFi Settings)
- [ ] Activation screen shows correct instructions
- [ ] ACTION button activates USB MSC
- [ ] Display shows "USB MODE ACTIVE"
- [ ] LED turns BLUE
- [ ] Logging pauses (check serial console)

### PC Access
- [ ] LEFT USB port works as flash drive
- [ ] PC can read existing files
- [ ] PC can write new files
- [ ] PC can delete files

### Deactivation
- [ ] ACTION button deactivates USB MSC
- [ ] Display shows "USB MODE OFF"
- [ ] Logging resumes (check serial console)
- [ ] LED returns to normal color

### Safety
- [ ] No file corruption after transition
- [ ] No data loss during USB access

---

## Troubleshooting

### Problem: USB MSC activation fails

**Solution:**
1. Check if USB MSC was initialized at startup
2. Verify SD card is inserted and working
3. Check serial console for error messages
4. Restart device

### Problem: PC doesn't detect drive

**Solution:**
1. Verify LEFT USB port is connected (not RIGHT port)
2. Check USB cable is data-capable (not charge-only)
3. Try different USB port on PC
4. Check serial console for USB host connection message

### Problem: Logging doesn't resume after deactivation

**Solution:**
1. Check serial console for error messages
2. Try activating and deactivating again
3. Restart device if needed

---

## Documentation

**Detailed Documentation:**
- `docs/USB_TRANSFER_MODE_JAN29.md` - Complete technical documentation
- `docs/USB_TRANSFER_IMPLEMENTATION_SUMMARY_JAN29.md` - Implementation details

**Quick Reference:**
- `USB_TRANSFER_QUICK_GUIDE.md` - This file

---

## Summary

You now have a dedicated USB Transfer Mode that:
- Allows direct SD card access via USB
- Automatically pauses/resumes logging
- Provides clear visual feedback
- Prevents file corruption
- Requires no SD card removal

**Ready to build and test!** 🚀

---

**Last Updated**: January 29, 2026  
**Author**: Kiro AI Assistant
