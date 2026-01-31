# USB Live Status Display - January 30, 2026

## Overview
Enhanced Mode 7 (USB Transfer) to show **LIVE USB connection status** on the OLED display. Now you can see in real-time whether the PC is connected, ready for transfer, or idle - making it safe to know when to exit USB mode.

## Problem Solved
Previously, Mode 7 only showed static text "Connect LEFT port to PC". You couldn't tell:
- Is PC connected?
- Is PC ready to transfer files?
- Is PC idle/suspended?
- Is it safe to press ACTION to exit?

## Solution: Live Status Display

### Three Status States

#### 1. **WAITING** (No PC Connected) ✅ SAFE TO EXIT
```
!USB MODE ACTIVE!
Waiting for PC
-----------------
Status: WAITING

Connect LEFT port
to PC

ACTION: Exit (safe)
```
- USB cable not connected OR PC hasn't recognized device
- **SAFE** to press ACTION button to exit
- No risk of data corruption

#### 2. **READY** (PC Connected & Active) ⚠️ EJECT FIRST
```
!USB CONNECTED!
PC accessing SD
-----------------
Status: READY

PC can read/write

Eject first, then
ACTION to exit
```
- PC has recognized device and mounted drive
- PC can actively read/write files
- **UNSAFE** to exit without ejecting first
- Must eject on PC before pressing ACTION

#### 3. **SUSPENDED** (PC Connected but Idle) ⚠️ PROBABLY SAFE
```
!USB SUSPENDED!
PC idle/sleep
-----------------
Status: SUSPENDED

PC idle or sleep
Wake PC to access

ACTION: Exit
```
- PC is connected but in sleep/idle mode
- No active file transfers
- **Probably safe** to exit, but eject first if possible
- PC might have pending operations

## Technical Implementation

### New Functions Added

#### 1. `usb_msc_sdcard_is_ready()` 
**File**: `components/usb_flash_partition/usb_msc_sdcard.c`

```c
bool usb_msc_sdcard_is_ready(void)
{
    // Device is ready when mounted and not suspended
    return tud_mounted() && !tud_suspended();
}
```

Uses TinyUSB functions:
- `tud_mounted()` - Returns true when PC has enumerated device
- `tud_suspended()` - Returns true when USB bus is suspended (PC idle/sleep)

#### 2. Updated OLED Display Logic
**File**: `components/oled_display/oled_display.c` line ~950

Checks three conditions:
1. `usb_msc_active` - Is USB Transfer mode active?
2. `usb_connected` - Is PC connected? (`tud_mounted()`)
3. `usb_ready` - Is PC ready? (`tud_mounted() && !tud_suspended()`)

Based on these, shows appropriate status screen.

### Status Detection Logic

```
usb_msc_active = false
├─> Show activation screen ("Press ACTION")

usb_msc_active = true
├─> usb_connected = true, usb_ready = true
│   └─> Status: READY (PC can transfer files)
│
├─> usb_connected = true, usb_ready = false
│   └─> Status: SUSPENDED (PC idle/sleep)
│
└─> usb_connected = false
    └─> Status: WAITING (No PC connected)
```

## User Experience

### Scenario 1: Normal File Transfer
1. Navigate to Mode 7 (USB Transfer)
2. OLED shows: "Press ACTION"
3. Press ACTION button
4. OLED shows: "Status: WAITING"
5. Connect USB cable to LEFT port
6. **OLED updates to: "Status: READY"** ← Live update!
7. PC shows drive, user copies files
8. User ejects drive on PC
9. **OLED updates to: "Status: WAITING"** ← Live update!
10. User disconnects USB cable
11. User presses ACTION to exit
12. **No warning** - safe disconnect ✅

### Scenario 2: PC Goes to Sleep
1. USB Transfer mode active
2. OLED shows: "Status: READY"
3. PC goes to sleep (no user action)
4. **OLED updates to: "Status: SUSPENDED"** ← Live update!
5. User can wake PC or exit mode

### Scenario 3: Unsafe Exit Attempt
1. USB Transfer mode active
2. OLED shows: "Status: READY" (PC connected)
3. User presses ACTION without ejecting
4. **Warning screen appears** (from previous implementation)
5. OLED shows: "!!! WARNING !!! USB still connected!"
6. User sees warning and knows to eject first

## Display States Reference

### Before Activation
```
USB Transfer Mode
Press ACTION
-----------------
This will:
1. Pause logging
2. Enable USB MSC
3. Expose SD card

ACTION: Activate
```

### After Activation - Waiting for PC
```
!USB MODE ACTIVE!
Waiting for PC
-----------------
Status: WAITING

Connect LEFT port
to PC

ACTION: Exit (safe)
```
**Meaning**: USB mode active but no PC connected yet. Safe to exit anytime.

### PC Connected and Ready
```
!USB CONNECTED!
PC accessing SD
-----------------
Status: READY

PC can read/write

Eject first, then
ACTION to exit
```
**Meaning**: PC is actively accessing SD card. Must eject on PC before exiting.

### PC Connected but Suspended
```
!USB SUSPENDED!
PC idle/sleep
-----------------
Status: SUSPENDED

PC idle or sleep
Wake PC to access

ACTION: Exit
```
**Meaning**: PC is connected but in low-power mode. Probably safe to exit, but eject first if possible.

## How to Use

### Safe Exit Procedure (Recommended)
1. Check OLED display in Mode 7
2. If shows "Status: READY":
   - Eject drive on PC (Windows: "Safely Remove Hardware")
   - Wait for OLED to change to "Status: WAITING"
   - Disconnect USB cable
   - Press ACTION button
3. If shows "Status: WAITING":
   - Safe to press ACTION immediately
4. If shows "Status: SUSPENDED":
   - Probably safe, but eject on PC if possible
   - Press ACTION button

### Quick Exit (If Urgent)
1. Press ACTION button anytime
2. If warning appears, you know PC was connected
3. System exits anyway after warning
4. Check files on SD card later for corruption

## Technical Details

### TinyUSB Status Functions

#### `tud_mounted()`
- Returns `true` when USB host (PC) has enumerated the device
- Returns `false` when:
  - USB cable not connected
  - PC hasn't recognized device yet
  - PC has ejected the drive
  - USB cable disconnected

#### `tud_suspended()`
- Returns `true` when USB bus is in suspended state
- Happens when:
  - PC goes to sleep
  - PC enters power-saving mode
  - No USB activity for extended period
- Returns `false` when PC is actively using USB

#### `tud_ready()`
- Convenience function: `tud_mounted() && !tud_suspended()`
- Returns `true` only when PC is connected AND active
- Best indicator for "safe to transfer files"

### Display Update Rate
- OLED updates every ~100ms in USB mode (main.c line ~1070)
- Status changes appear within 100-200ms
- Near real-time feedback for user

### Limitations
1. **Cannot detect active file transfer**: TinyUSB doesn't provide API to check if read/write is in progress
2. **Cannot detect pending writes**: PC might have cached writes not yet flushed
3. **Suspended state ambiguous**: Could mean PC idle OR PC preparing to transfer

## Testing Instructions

### Test 1: Live Status Updates
1. Flash firmware: `idf.py flash monitor`
2. Navigate to Mode 7
3. Press ACTION to activate
4. **Expected**: OLED shows "Status: WAITING"
5. Connect USB cable to LEFT port
6. **Expected**: OLED updates to "Status: READY" within 1 second
7. On PC, eject the drive
8. **Expected**: OLED updates to "Status: WAITING" within 1 second
9. Disconnect USB cable
10. Press ACTION to exit
11. **Expected**: No warning, smooth exit

### Test 2: PC Sleep Detection
1. Activate USB Transfer mode
2. Connect to PC
3. **Expected**: OLED shows "Status: READY"
4. Put PC to sleep (close laptop lid or sleep mode)
5. **Expected**: OLED updates to "Status: SUSPENDED"
6. Wake PC
7. **Expected**: OLED updates back to "Status: READY"

### Test 3: Safe Exit Indicator
1. Activate USB Transfer mode
2. Don't connect USB cable
3. **Expected**: OLED shows "Status: WAITING" and "ACTION: Exit (safe)"
4. Press ACTION
5. **Expected**: No warning, immediate exit

### Test 4: Unsafe Exit Warning
1. Activate USB Transfer mode
2. Connect to PC
3. **Expected**: OLED shows "Status: READY" and "Eject first, then ACTION to exit"
4. Press ACTION without ejecting
5. **Expected**: Warning screen appears
6. **Expected**: Mode exits after warning

## Serial Console Output

### Status Changes
```
I (12345) USB_MSC_SD: 🔌 USB host connected
I (12345) MAIN: 🔌 USB Status: MSC_active=YES, USB_connected=YES
```

```
I (23456) USB_MSC_SD: 🔌 USB host disconnected
I (23456) MAIN: 🔌 USB Status: MSC_active=YES, USB_connected=NO
```

### Safe Exit (No Warning)
```
I (34567) MAIN: 💾 USB Transfer mode EXIT triggered from button
I (34567) USB_MSC_SD: 📥 Deactivating USB Transfer mode...
I (34567) USB_MSC_SD: ✓ USB host not connected - safe to deactivate
```

### Unsafe Exit (Warning)
```
I (45678) MAIN: 💾 USB Transfer mode EXIT triggered from button
I (45678) USB_MSC_SD: 📥 Deactivating USB Transfer mode...
W (45678) USB_MSC_SD: ⚠️  USB host still connected!
W (45678) USB_MSC_SD: ⚠️  Please disconnect USB cable from LEFT port first
```

## Benefits

### For Users
1. **Visual feedback**: Know exactly what's happening
2. **Safe exit guidance**: Clear instructions when to exit
3. **Real-time updates**: See status changes immediately
4. **Confidence**: No guessing if PC is connected

### For Data Safety
1. **Informed decisions**: User knows when it's safe to exit
2. **Clear warnings**: "Eject first" message when PC connected
3. **Status awareness**: See if PC is idle vs active
4. **Reduced corruption risk**: Users more likely to eject first

## Files Modified

1. **components/usb_flash_partition/usb_msc_sdcard.c**
   - Added `usb_msc_sdcard_is_ready()` function

2. **components/usb_flash_partition/usb_flash_partition.h**
   - Added function declaration

3. **components/oled_display/oled_display.c**
   - Updated Mode 7 display logic (line ~950)
   - Added three status screens (WAITING, READY, SUSPENDED)
   - Added live status checking

## Build Status
✅ **BUILD SUCCESSFUL**
- Binary size: 0x129580 bytes (1,218 KB)
- Free space: 0x56a80 bytes (354 KB, 23% free)
- No compilation errors
- No warnings

## Related Documentation
- `USB_SAFE_DISCONNECT_IMPLEMENTATION.md` - Safe disconnect warnings
- `USB_BUTTON_AND_SAFETY_COMPLETE.md` - Button exit implementation
- `docs/note-10-usb-host-device-mode-fix.md` - USB Host/Device mode fix

## Status Summary
✅ **Live status display implemented**
✅ **Three status states (WAITING, READY, SUSPENDED)**
✅ **Real-time updates on OLED**
✅ **Safe exit guidance**
✅ **Build successful**
⏳ **Testing needed**

## Next Steps
1. Flash firmware: `idf.py flash monitor`
2. Test live status updates (connect/disconnect USB)
3. Test PC sleep detection
4. Verify safe exit guidance
5. Confirm status changes appear in real-time

---

**Now you can see exactly what's happening with USB connection in real-time!**
