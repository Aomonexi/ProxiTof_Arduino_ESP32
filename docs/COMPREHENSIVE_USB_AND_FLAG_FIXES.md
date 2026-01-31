# Comprehensive USB and Flag Fixes - January 30, 2026

## Three Critical Issues to Fix

### Issue 1: USB Mode is Slow - Need Dedicated Mode
**Problem**: USB transfer is slow because device is still trying to do everything (sensors, display updates, GPS, etc.) while in USB mode.

**Solution**: Make USB mode a **dedicated mode** that suspends all non-essential tasks:
- Stop sensor reading
- Stop GPS processing  
- Minimal display updates (just USB status)
- Stop data processing
- Focus CPU entirely on USB transfer

### Issue 2: ACTION Button Should Not Disconnect from PC
**Problem**: ACTION button in USB mode tries to disconnect from PC, which causes problems and confusion.

**Solution**: Change ACTION button behavior:
- **Remove**: Automatic disconnect from PC
- **Keep**: Cancel USB mode and return to logging
- **User must**: Eject on PC first, then disconnect cable, then press ACTION
- **Simpler**: ACTION just switches SD card back to APP mode, doesn't touch USB connection

### Issue 3: False Manual Flags at End of Data
**Problem**: Manual flag (1) appears at end of every log file even when button wasn't pressed. Happens during:
- Shutdown
- Flush operations
- End of logging session

**Root Cause**: `flag_manual = sensor_manager_is_button_held() ? 1 : 0` checks button state EVERY frame, including during shutdown when button might be accidentally pressed or held.

**Solution**: 
- Only set manual flag on **button press event** (edge detection), not continuous checking
- Clear manual flag after logging one entry
- Don't check button during shutdown/flush
- Add debouncing to prevent accidental triggers

## Implementation Plan

### Fix 1: Dedicated USB Mode

**Changes needed**:
1. When USB mode activates:
   - Stop sensor manager updates
   - Stop GPS processing
   - Stop data processor
   - Minimal OLED updates (1Hz instead of 10Hz)
   - Increase TinyUSB task priority

2. When USB mode deactivates:
   - Resume sensor manager
   - Resume GPS
   - Resume data processor
   - Resume normal OLED updates

**Files to modify**:
- `main/main.c` - Add USB mode flag, suspend tasks
- `components/usb_flash_partition/usb_msc_sdcard.c` - Increase task priority

### Fix 2: Simplify ACTION Button in USB Mode

**Changes needed**:
1. Remove all USB disconnect logic from ACTION button
2. ACTION button only:
   - Switches SD card from USB mode to APP mode
   - Resumes logging
   - Returns to normal operation
3. User workflow:
   - Eject on PC
   - Disconnect USB cable
   - Press ACTION to return to logging

**Files to modify**:
- `components/oled_display/oled_display.c` - Simplify button handler
- `components/usb_flash_partition/usb_msc_sdcard.c` - Remove disconnect checks
- `main/main.c` - Remove USB disconnect logic from button handler

### Fix 3: Fix False Manual Flags

**Changes needed**:
1. Add manual flag state machine:
   ```c
   static bool manual_flag_pending = false;
   static uint32_t manual_flag_time = 0;
   ```

2. Detect button **press event** (edge), not continuous state:
   ```c
   static bool last_button_state = false;
   bool current_button_state = sensor_manager_is_button_held();
   
   // Detect button press (rising edge)
   if (current_button_state && !last_button_state) {
       manual_flag_pending = true;
       manual_flag_time = current_time;
   }
   
   last_button_state = current_button_state;
   ```

3. Use pending flag in logging:
   ```c
   log_entry.flag_manual = manual_flag_pending ? 1 : 0;
   
   // Clear flag after logging
   if (manual_flag_pending) {
       manual_flag_pending = false;
   }
   ```

4. Don't check button during:
   - Shutdown sequence
   - Flush operations
   - USB mode

**Files to modify**:
- `main/main.c` - Add flag state machine, edge detection

## Priority

1. **Fix 3 (False Manual Flags)** - CRITICAL - Data integrity issue
2. **Fix 2 (Simplify ACTION)** - HIGH - User confusion and problems
3. **Fix 1 (Dedicated USB Mode)** - MEDIUM - Performance improvement

## Next Steps

1. Implement Fix 3 first (false manual flags)
2. Test thoroughly with shutdown and flush
3. Implement Fix 2 (simplify ACTION button)
4. Test USB mode workflow
5. Implement Fix 1 (dedicated USB mode)
6. Test USB transfer speed

Would you like me to proceed with implementing these fixes?
