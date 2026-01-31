# USB Mode Simplification - January 30, 2026

## Summary of Changes

Based on user feedback, simplified the USB Transfer mode to remove confusing automatic disconnect logic and clarify the user workflow.

## Changes Made

### 1. Removed USB Disconnect Checks from Resume Function

**File**: `components/usb_flash_partition/usb_msc_sdcard.c`

**Before**:
```c
// SAFETY CHECK: Warn if USB host is still connected
bool usb_connected = tud_mounted();
if (usb_connected) {
    ESP_LOGW(TAG, "⚠️  USB host still connected!");
    ESP_LOGW(TAG, "⚠️  Please disconnect USB cable from LEFT port first");
    ESP_LOGW(TAG, "⚠️  Continuing anyway - data may be corrupted if transfer in progress");
    
    // Give USB host time to finish any pending operations
    vTaskDelay(pdMS_TO_TICKS(500));
} else {
    ESP_LOGI(TAG, "✓ USB host not connected - safe to deactivate");
}
```

**After**:
```c
ESP_LOGI(TAG, "📥 Deactivating USB Transfer mode...");
ESP_LOGI(TAG, "💡 User workflow: Eject on PC → Disconnect cable → Press ACTION");

// Switch to APP mode (unmount from USB, mount to app)
// No need to check USB connection - user is responsible for ejecting first
```

**Reason**: The automatic checks were confusing and didn't actually prevent problems. User is now responsible for proper workflow.

### 2. Simplified OLED Display Handler

**File**: `components/oled_display/oled_display.c`

**Before**:
```c
// SAFETY CHECK: Check if USB host is still connected
bool usb_connected = usb_msc_sdcard_is_host_connected();

if (usb_connected) {
    ESP_LOGW(TAG, "⚠️  USB host still connected - deactivating anyway");
}
```

**After**:
```c
// Deactivate USB MSC mode
ESP_LOGI(TAG, "Deactivating USB Transfer mode");
ESP_LOGI(TAG, "💡 Ensure you ejected on PC and disconnected cable first!");
```

**Reason**: Simpler message, no confusing warnings. User knows what to do.

### 3. Updated OLED Display Instructions

**File**: `components/oled_display/oled_display.c`

**Before**:
```
PC can read/write

Eject first, then
ACTION to exit
```

**After**:
```
PC can read/write

1.Eject on PC
2.ACTION to exit
```

**Reason**: Clearer step-by-step instructions. Numbered steps are easier to follow.

## New User Workflow

### Activating USB Transfer Mode:
1. Navigate to Mode 7 (USB Transfer)
2. Press ACTION button
3. Display shows "USB MODE ACTIVE"
4. Connect LEFT port (USB-OTG) to PC
5. Display shows "USB CONNECTED - PC accessing SD"
6. PC can now read/write files

### Deactivating USB Transfer Mode:
1. **Eject drive on PC** (right-click → Eject)
2. **Disconnect USB cable** from LEFT port
3. **Press ACTION button** on device
4. Display shows "Logging mode activated"
5. Device resumes normal operation

## What Was Removed

❌ Automatic USB connection detection on exit
❌ Warning messages about USB still connected
❌ Delays waiting for USB to disconnect
❌ Confusing "deactivating anyway" messages

## What Was Kept

✅ Simple ACTION button toggle
✅ Clear display status (WAITING/READY/SUSPENDED)
✅ Proper SD card mount point switching
✅ Logging pause/resume
✅ User instructions on OLED

## Benefits

### Before:
- Confusing warnings
- Automatic checks that didn't help
- User unsure if safe to disconnect
- Mixed messages about what to do

### After:
- Clear workflow: Eject → Disconnect → ACTION
- No confusing warnings
- User knows exactly what to do
- Simple and predictable behavior

## Manual Flag Status

The manual flag implementation is **already correct** as of the previous fix:

✅ **Mode-specific behavior**: Only works in Dashboard (mode 0) and Near Miss (mode 5)
✅ **Continuous flagging**: Flags all entries while button is held (as requested)
✅ **No false flags**: Doesn't flag during shutdown, USB mode, or other modes
✅ **Logging**: Shows "🚩 Manual flag ACTIVATED/DEACTIVATED" messages

**User's request (query 8)**: "hold on, but we still need the button to log the near miss as long as the button is pressed, is just make sure that it only logs only on the near miss mode and the dashboard when the action button is being pressed"

**Implementation**: Exactly as requested! See `main/main.c` lines 726-755.

## Display Glitch Status

The display glitch when pressing ACTION in USB mode was **already fixed** in the previous session:

✅ **TinyUSB task moved to Core 1**: Prevents preemption of main loop
✅ **50ms delays added**: Allows display to refresh properly
✅ **Task yields added**: Ensures smooth transitions

**Files modified**: 
- `components/usb_flash_partition/usb_msc_sdcard.c` (TinyUSB task config)
- `components/oled_display/oled_display.c` (display refresh delays)
- `main/main.c` (task yields)

## Remaining Issue: USB Mode Performance

**User feedback (query 7)**: "the USB mode seems very slow? can the USB mode have a dedicated session, so basically the whole device switch mode to it, so it has the resources to run it in full speed?"

**Current behavior**: Device continues running all tasks (sensors, GPS, display) while in USB mode, which may slow down USB transfers.

**Proposed solution** (not yet implemented):
1. When USB mode activates:
   - Stop sensor reading (ToF, IMU)
   - Stop GPS processing
   - Reduce OLED updates to 1Hz (just status)
   - Stop data processor
   - Focus CPU on USB transfer

2. When USB mode deactivates:
   - Resume all tasks
   - Return to normal operation

**Status**: Not implemented yet. Waiting for user confirmation if this is needed.

## Build Status

✅ **Changes compiled successfully**
- Modified 2 files
- No compilation errors
- No warnings
- Ready for testing

## Testing Checklist

### Test 1: USB Activation
1. Navigate to Mode 7
2. Press ACTION
3. **Expected**: Display shows "USB MODE ACTIVE - Waiting for PC"
4. Connect USB cable
5. **Expected**: Display shows "USB CONNECTED - PC accessing SD"
6. **Expected**: PC detects USB drive

### Test 2: USB Deactivation (Proper Workflow)
1. While in USB mode with PC connected
2. Eject drive on PC
3. Disconnect USB cable
4. Press ACTION
5. **Expected**: Display shows "Logging mode activated"
6. **Expected**: No error messages
7. **Expected**: Logging resumes

### Test 3: Manual Flag (Dashboard Mode)
1. Navigate to Mode 0 (Dashboard)
2. Press and hold ACTION button
3. **Expected**: Serial console shows "🚩 Manual flag ACTIVATED"
4. **Expected**: All log entries while held have flag_manual=1
5. Release button
6. **Expected**: Serial console shows "🚩 Manual flag DEACTIVATED"
7. **Expected**: Subsequent entries have flag_manual=0

### Test 4: Manual Flag (Near Miss Mode)
1. Navigate to Mode 5 (Near Miss)
2. Press and hold ACTION button
3. **Expected**: Same behavior as Dashboard mode
4. **Expected**: Continuous flagging while held

### Test 5: Manual Flag (Other Modes)
1. Navigate to Mode 6 (WiFi Settings)
2. Press ACTION button
3. **Expected**: WiFi sync triggered (not manual flag)
4. Navigate to Mode 8 (Shutdown)
5. Press ACTION button
6. **Expected**: Shutdown confirmation (not manual flag)
7. **Expected**: No false flags in log file

### Test 6: Display Updates in USB Mode
1. Activate USB mode
2. Connect/disconnect USB cable multiple times
3. **Expected**: Display updates smoothly
4. **Expected**: No glitching or freezing
5. Press ACTION to exit
6. **Expected**: Display updates immediately

## Files Modified

1. `components/usb_flash_partition/usb_msc_sdcard.c`
   - Removed USB disconnect checks from `usb_msc_sdcard_resume_logging()`
   - Simplified logging messages
   - Added user workflow reminder

2. `components/oled_display/oled_display.c`
   - Removed USB connection check from button handler
   - Simplified exit logic
   - Updated display instructions (numbered steps)

## Next Steps

1. **Build and flash**: `idf.py build flash monitor`
2. **Test USB workflow**: Activate → Connect → Eject → Disconnect → ACTION
3. **Test manual flag**: Verify continuous flagging in Dashboard/Near Miss modes
4. **Test display**: Verify no glitching when pressing ACTION in USB mode
5. **Check performance**: If USB is still slow, implement dedicated mode

## User Feedback Addressed

✅ **Query 7a**: "take out the ability of action on mode 7 to unplug from PC"
   - **Fixed**: Removed automatic disconnect checks
   - **Result**: ACTION just switches SD card back to APP mode

✅ **Query 7b**: "USB mode seems very slow"
   - **Status**: Identified cause (all tasks still running)
   - **Solution**: Dedicated mode (not yet implemented)
   - **Waiting**: User confirmation if needed

✅ **Query 8**: "we still need the button to log the near miss as long as the button is pressed"
   - **Status**: Already implemented correctly!
   - **Location**: `main/main.c` lines 726-755
   - **Behavior**: Continuous flagging in Dashboard/Near Miss modes only

## Summary

The USB mode is now **simpler and clearer**:
- No confusing automatic checks
- Clear user workflow: Eject → Disconnect → ACTION
- Better display instructions
- Manual flag works correctly (continuous in Dashboard/Near Miss)
- Display updates smoothly (fixed in previous session)

The only remaining issue is USB transfer speed, which can be addressed with a dedicated mode if needed.

---

**Status**: ✅ Ready for testing
**Build**: ✅ Successful
**User workflow**: ✅ Simplified
**Manual flags**: ✅ Already correct
**Display glitch**: ✅ Already fixed
