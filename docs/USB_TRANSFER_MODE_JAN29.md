# USB Transfer Mode - Complete Documentation

**Date**: January 29, 2026  
**Mode Number**: 7 (between WiFi Settings and Shutdown)  
**Purpose**: Enable USB Mass Storage mode for direct SD card file access  
**Status**: ✅ Implemented and ready for testing

---

## Overview

USB Transfer Mode provides a safe, user-friendly way to access SD card files directly from a PC without removing the SD card. The system automatically pauses data logging, enables USB Mass Storage Class (MSC), and provides clear visual feedback throughout the process.

### Key Features

- **One-button activation/deactivation** - Simple ACTION button toggle
- **Automatic logging pause** - No file corruption from simultaneous access
- **Clear visual feedback** - Inverted headers, USB icon, LED indicators
- **Safe switching** - Proper unmount/mount sequence
- **Direct PC access** - SD card appears as removable drive
- **No SD card removal** - Access files via USB cable

---

## Display Layouts

### Activation Screen (USB MSC Off)

```
┌────────────────────────────────────────────────────────┐
│ YELLOW ZONE (Pages 0-1, 16 pixels tall) - INVERTED    │
├────────────────────────────────────────────────────────┤
│ Page 0: USB Transfer Mode                              │ ← Header (inverted)
│ Page 1: Press ACTION                                   │ ← Instruction (inverted)
├────────────────────────────────────────────────────────┤
│ BLUE ZONE (Pages 2-7, 48 pixels tall)                 │
├────────────────────────────────────────────────────────┤
│ Page 2: This will:                                     │ ← Header
│ Page 3: 1. Pause logging                               │ ← Action 1
│ Page 4: 2. Enable USB MSC                              │ ← Action 2
│ Page 5: 3. Show USB icon                               │ ← Action 3
│ Page 6:                                                │ ← Empty
│ Page 7: Press ACTION start                             │ ← Instruction
└────────────────────────────────────────────────────────┘
```

### Active Screen (USB MSC On)

```
┌────────────────────────────────────────────────────────┐
│ YELLOW ZONE (Pages 0-1, 16 pixels tall) - INVERTED    │
├────────────────────────────────────────────────────────┤
│ Page 0: !USB MODE ACTIVE!                              │ ← Status (inverted)
│ Page 1: Logging PAUSED                                 │ ← Warning (inverted)
├────────────────────────────────────────────────────────┤
│ BLUE ZONE (Pages 2-7, 48 pixels tall)                 │
├────────────────────────────────────────────────────────┤
│ Page 2:                                                │ ← Empty
│ Page 3:   [USB]                                        │ ← USB icon (centered)
│ Page 4:                                                │ ← Empty
│ Page 5: Connect LEFT port                              │ ← Instruction
│ Page 6: to PC for files                                │ ← Instruction
│ Page 7: ACTION to exit                                 │ ← Exit instruction
└────────────────────────────────────────────────────────┘
```

---

## Button Behavior

### Button 1 (GPIO 20) - Mode Switch
- **Action:** Cycle to next mode
- **Next mode:** Shutdown (Mode 8)
- **Effect:** Exits USB Transfer mode (does NOT deactivate USB MSC if active)

### Button 2 (GPIO 3) - Action
- **When USB MSC Off:** Activate USB MSC mode
  - LED: Blue during activation
  - Display: Shows "Pausing logging..." → "USB MODE ACTIVE"
  - Result: Logging paused, SD card accessible via USB
  
- **When USB MSC On:** Deactivate USB MSC mode
  - LED: Blue during deactivation
  - Display: Shows "Resuming logging..." → "USB MODE OFF"
  - Result: Logging resumed, SD card back to normal

---

## Operation Sequence

### Activation (Normal → USB Mode)

1. **User Action:** Navigate to USB Transfer mode (Mode 7)
2. **Display:** Shows activation screen with instructions
3. **User Action:** Press ACTION button (GPIO 3)
4. **System:** Displays "Pausing logging..."
5. **System:** Calls `usb_msc_sdcard_pause_logging()`
6. **System:** Stops data logging
7. **System:** Unmounts SD card from app (`/sdcard`)
8. **System:** Mounts SD card to USB MSC
9. **Display:** Shows "USB MODE ACTIVE" with USB icon
10. **LED:** Turns BLUE (solid)
11. **User Action:** Connect LEFT USB port to PC
12. **PC:** Detects removable drive
13. **User:** Can read/write files directly

### Deactivation (USB Mode → Normal)

1. **User Action:** Safely eject drive from PC
2. **User Action:** Press ACTION button (GPIO 3)
3. **System:** Displays "Resuming logging..."
4. **System:** Calls `usb_msc_sdcard_resume_logging()`
5. **System:** Unmounts SD card from USB MSC
6. **System:** Mounts SD card back to app (`/sdcard`)
7. **System:** Resumes data logging
8. **Display:** Shows "USB MODE OFF"
9. **LED:** Returns to distance-based color
10. **System:** Continues normal operation

---

## LED Indicators

| State | LED Color | Meaning |
|-------|-----------|---------|
| Activation | Blue (solid) | Transitioning to USB mode |
| Active | Blue (solid) | USB MSC active, logging paused |
| Deactivation | Blue (solid) | Transitioning to normal mode |
| Normal | Distance-based | Normal operation resumed |

---

## Safety Features

### Automatic Logging Pause
- Logging automatically stops when USB MSC activates
- Prevents file corruption from simultaneous access
- No manual intervention needed
- Automatic resume when USB MSC deactivates

### Clear Visual Feedback
- **Inverted yellow zone** - High visibility status
- **USB icon** - Clear indication of active state
- **LED indicator** - Blue = USB mode
- **Status messages** - Step-by-step feedback

### Safe Switching
- **Proper unmount/mount sequence** - No data loss
- **Clean state management** - No corruption
- **Error handling** - Graceful failure recovery

### User Control
- **Explicit activation** - User must press button
- **Explicit deactivation** - User must press button
- **No automatic switching** - Predictable behavior

---

## Technical Details

### USB Port Assignment
- **LEFT port (USB-OTG):** USB Mass Storage Class (MSC)
- **RIGHT port (USB Serial JTAG):** Programming, monitoring, commands

### SD Card Access
- **Normal mode:** Mounted at `/sdcard` for app
- **USB mode:** Unmounted from app, mounted to USB MSC
- **Transition:** Safe unmount/mount sequence via TinyUSB

### Logging Behavior
- **Normal mode:** Active logging to SD card
- **USB mode:** Logging paused (no writes)
- **Transition:** Automatic pause/resume

### File System
- **Format:** FAT32 (compatible with all PCs)
- **Access:** Direct sector access via USB MSC
- **Safety:** No simultaneous access (app OR USB, not both)

### Implementation
- **Component:** `components/usb_flash_partition/usb_msc_sdcard.c`
- **Functions:**
  - `usb_msc_sdcard_init()` - Initialize USB MSC
  - `usb_msc_sdcard_pause_logging()` - Activate USB mode
  - `usb_msc_sdcard_resume_logging()` - Deactivate USB mode
  - `usb_msc_sdcard_is_active()` - Check current state
  - `usb_msc_sdcard_get_status()` - Get detailed status

---

## Use Cases

### Use Case 1: Access Files on PC

**Scenario:** User wants to view/copy log files on PC

**Steps:**
1. Navigate to USB Transfer mode (Mode 7)
2. Read what will happen on screen
3. Press ACTION button (GPIO 3)
4. Wait for "USB MODE ACTIVE" message
5. Connect LEFT USB port to PC
6. PC shows removable drive
7. Browse/copy files as needed
8. Safely eject drive from PC
9. Press ACTION button to exit
10. Wait for "USB MODE OFF" message
11. System resumes logging

**Result:** Files accessed without removing SD card

### Use Case 2: Quick File Check

**Scenario:** User wants to verify latest log file

**Steps:**
1. Navigate to USB Transfer mode
2. Press ACTION to activate
3. Connect to PC
4. Check latest log file timestamp
5. Disconnect from PC
6. Press ACTION to deactivate
7. Continue normal operation

**Result:** Quick verification without interrupting workflow

### Use Case 3: Emergency File Backup

**Scenario:** User needs to backup important data immediately

**Steps:**
1. Navigate to USB Transfer mode
2. Activate USB MSC
3. Connect to PC
4. Copy all important files to PC
5. Safely eject
6. Deactivate USB MSC
7. System ready for use

**Result:** Data backed up safely

---

## Error Handling

### USB MSC Initialization Failed

**Symptom:** "FAILED - Check USB init" message

**Possible Causes:**
- USB MSC not initialized at startup
- SD card not available
- TinyUSB driver issue

**Solution:**
1. Check serial console for error messages
2. Verify SD card is inserted and working
3. Restart device
4. Check USB cable connection

### Activation Failed

**Symptom:** "FAILED" message when activating

**Possible Causes:**
- USB MSC already active
- SD card unmount failed
- TinyUSB error

**Solution:**
1. Try deactivating first
2. Check serial console for details
3. Restart device if needed

### Deactivation Failed

**Symptom:** "FAILED" message when deactivating

**Possible Causes:**
- USB still connected to PC
- SD card mount failed
- File system error

**Solution:**
1. Safely eject from PC first
2. Disconnect USB cable
3. Try deactivating again
4. Check serial console for details

---

## Console Output

### Activation Sequence

```
I (12345) USB_MSC_SD: 📤 Switching to USB mode...
I (12345) USB_MSC_SD: 🔄 MSC mount operation starting...
I (12345) USB_MSC_SD: 📤 Switching to USB mode - pausing data logging
I (12345) USB_MSC_SD: ✓ MSC mount operation completed
I (12345) USB_MSC_SD: 💾 SD card now accessible via USB
I (12345) USB_MSC_SD: 🔌 Connect LEFT port to computer to access files
I (12345) MAIN: 💾 USB Transfer mode toggle triggered from button
```

### Deactivation Sequence

```
I (23456) USB_MSC_SD: 📥 Switching to APP mode...
I (23456) USB_MSC_SD: 🔄 MSC mount operation starting...
I (23456) USB_MSC_SD: 📥 Switching to APP mode - resuming data logging
I (23456) USB_MSC_SD: ✓ MSC mount operation completed
I (23456) USB_MSC_SD: 📊 Data logging resumed
I (23456) MAIN: 💾 USB Transfer mode toggle triggered from button
```

### USB Host Connection

```
I (34567) USB_MSC_SD: 🔌 USB host connected
I (45678) USB_MSC_SD: 🔌 USB host disconnected
```

---

## Testing Checklist

### Display Tests
- [ ] USB Transfer mode appears in mode cycle (after WiFi Settings)
- [ ] Activation screen shows correct instructions
- [ ] Active screen shows USB icon
- [ ] Status messages are clear and accurate
- [ ] Inverted headers are visible

### Functionality Tests
- [ ] ACTION button activates USB MSC
- [ ] Display shows "USB MODE ACTIVE"
- [ ] LED turns BLUE
- [ ] Logging pauses (verified in serial console)
- [ ] LEFT USB port works as flash drive
- [ ] PC can read SD card files
- [ ] PC can write to SD card
- [ ] PC can create new files
- [ ] PC can delete files
- [ ] ACTION button deactivates USB MSC
- [ ] Display shows "USB MODE OFF"
- [ ] Logging resumes (verified in serial console)
- [ ] LED returns to normal color

### Safety Tests
- [ ] No file corruption after transition
- [ ] No data loss during USB access
- [ ] Proper unmount/mount sequence
- [ ] Clean state transitions
- [ ] Error handling works correctly

### Integration Tests
- [ ] Mode switching works correctly
- [ ] Button behavior is consistent
- [ ] LED indicators are correct
- [ ] Serial console output is clear
- [ ] System resumes normal operation after USB mode

---

## Benefits

### Data Safety
- **No file corruption** - Automatic logging pause prevents simultaneous access
- **Proper unmount/mount** - Clean state transitions
- **Error handling** - Graceful failure recovery

### User Convenience
- **No SD card removal** - Access files via USB cable
- **Direct PC access** - SD card appears as removable drive
- **Simple activation** - One-button toggle

### System Integrity
- **Automatic pause/resume** - Logging handled automatically
- **No data loss** - Safe switching mechanism
- **Clean transitions** - Proper state management

### Operational Safety
- **Clear feedback** - Visual and LED indicators
- **User control** - Explicit activation/deactivation
- **Predictable behavior** - No automatic switching

---

## Mode Navigation

**Current Order:**
```
Dashboard → Status → ToF Focus → IMU Focus → GPS → Near Miss → WiFi Settings → USB Transfer → Shutdown → (back to Dashboard)
```

**Mode Numbers:**
- 0: Dashboard
- 1: Status
- 2: ToF Focus
- 3: IMU Focus
- 4: GPS
- 5: Near Miss
- 6: WiFi Settings
- 7: USB Transfer (NEW!)
- 8: Shutdown

---

## Implementation Files

### Modified Files
1. `components/oled_display/include/oled_display.h`
   - Added `OLED_MODE_USB_TRANSFER` enum

2. `components/oled_display/oled_display.c`
   - Added USB Transfer mode display logic
   - Added USB Transfer mode button handling
   - Updated mode names array

3. `main/main.c`
   - Added USB Transfer mode button handling
   - Updated mode names array

### Existing Files (Used)
1. `components/usb_flash_partition/usb_msc_sdcard.c`
   - USB MSC implementation
   - Pause/resume logging functions

2. `components/usb_flash_partition/usb_flash_partition.h`
   - USB MSC API definitions

3. `components/storage_manager/storage_manager.c`
   - SD card access and logging

---

## Next Steps

1. **Build and flash** - Compile and upload to device
2. **Test activation** - Verify USB MSC activates correctly
3. **Test PC access** - Verify files are accessible from PC
4. **Test deactivation** - Verify logging resumes correctly
5. **Test error cases** - Verify error handling works
6. **Update user manual** - Document USB Transfer mode for users

---

**Last Updated**: January 29, 2026  
**Status**: ✅ Implementation complete, ready for testing  
**Author**: Kiro AI Assistant
