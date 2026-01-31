# USB MSC Crash Fix & Cleanup - Jan 29, 2026

## Issues Fixed

### Issue 1: System Crash When Activating USB Transfer Mode ❌→✅
**Problem**: Device crashed and rebooted when pressing ACTION in Mode 7.

**Error**:
```
E (28134) STORAGE_MGR: Failed to create directory: /sdcard/20260129 (errno: 19 - No such device)
assert failed: spinlock_acquire spinlock.h:142
```

**Root Cause**: 
- When switching to USB mode, SD card is unmounted from app and mounted to USB
- Storage manager continued trying to write logs to unmounted SD card
- File rotation triggered during USB mode → tried to create directory on unmounted SD card → CRASH

**Solution**:
1. Stop logging BEFORE switching to USB mode
2. Wait for final flush to complete
3. Then switch SD card to USB mode
4. Restart logging when switching back to APP mode

**Files Modified**:
- `components/usb_flash_partition/usb_msc_sdcard.c`
  - `usb_msc_sdcard_pause_logging()`: Added `storage_manager_stop_logging()` before mode switch
  - `usb_msc_sdcard_resume_logging()`: Added `storage_manager_start_logging()` after mode switch

---

### Issue 2: MSC Active at Startup ❌→✅
**Problem**: `MSC_active=YES` at startup, but should be `NO` until Mode 7 is activated.

**Root Cause**:
```c
.mount_point = TINYUSB_MSC_STORAGE_MOUNT_USB,  // Started in USB mode
msc_active = true;  // Started active
```

**Solution**: Changed to start in APP mode (logging active, USB inactive)
```c
.mount_point = TINYUSB_MSC_STORAGE_MOUNT_APP,  // Start in APP mode
msc_active = false;  // Start inactive
```

**Result**: Now shows `MSC_active=NO` at startup ✅

---

### Issue 3: Confusing Callback Messages ❌→✅
**Problem**: Callback showed wrong messages:
```
I (26074) USB_MSC_SD: 📤 Switching to USB mode...
I (26074) USB_MSC_SD: 📥 Switching to APP mode - resuming data logging  ← WRONG!
```

**Solution**: Simplified and clarified callback messages:
- USB mode: "💾 USB mode activated - SD card accessible via USB"
- APP mode: "📊 APP mode activated - data logging resumed"
- Removed redundant "mount operation starting/completed" messages

---

### Issue 4: Excessive Startup Messages 🧹
**Problem**: Too many confusing messages about features that don't exist:
- "Flash partition available for USB access" (no flash partition)
- "USB_SYNC_DATA command" (doesn't exist)
- Long "USB System Overview" (unnecessary)

**Solution**: Cleaned up to essential messages only:
```
I (5754) MAIN: Initializing USB Transfer mode...
I (5754) MAIN: ✓ USB Transfer mode ready
I (5764) MAIN: 💡 Navigate to Mode 7 and press ACTION to activate
I (5974) MAIN: 🚀 Starting data logging...
I (5974) MAIN: ✓ Data logging started
```

---

## How USB Transfer Mode Works Now

### Startup (Mode 0-6)
```
MSC_active = NO
Logging = ACTIVE
SD card = Mounted to APP
```

### Activate USB Transfer (Mode 7 → Press ACTION)
```
1. Stop logging (flush all data)
2. Unmount SD card from APP
3. Mount SD card to USB
4. MSC_active = YES
5. PC detects USB drive
```

### Deactivate USB Transfer (Mode 7 → Press ACTION again)
```
1. Unmount SD card from USB
2. Mount SD card to APP
3. Restart logging
4. MSC_active = NO
```

---

## Testing Checklist

### Test 1: Startup State ✅
1. Power on device
2. Check serial monitor
3. **Expected**: `MSC_active=NO`, logging active

### Test 2: Activate USB Transfer ✅
1. Navigate to Mode 7 (USB Transfer)
2. Press ACTION button
3. **Expected**: 
   - Logging stops
   - `MSC_active=YES`
   - PC detects USB drive
   - Can access SD card files

### Test 3: Deactivate USB Transfer ✅
1. In Mode 7, press ACTION button again
2. **Expected**:
   - USB drive disappears from PC
   - `MSC_active=NO`
   - Logging resumes
   - New log file created

### Test 4: No Crash During Switch ✅
1. Let device log for 30 seconds (trigger file rotation)
2. Activate USB Transfer mode
3. **Expected**: No crash, smooth transition

---

## Files Modified

1. **components/usb_flash_partition/usb_msc_sdcard.c**
   - Fixed: Start in APP mode instead of USB mode
   - Fixed: Stop logging before USB mode switch
   - Fixed: Restart logging after APP mode switch
   - Fixed: Simplified callback messages

2. **main/main.c**
   - Cleaned up: USB initialization messages
   - Removed: References to non-existent features
   - Simplified: Startup logging messages

---

## Build Instructions

```bash
# Build and flash
idf.py build flash monitor
```

---

## Summary

All issues resolved:
- ✅ No crash when activating USB Transfer mode
- ✅ MSC inactive at startup (only active in Mode 7)
- ✅ Clear, concise messages
- ✅ Proper logging pause/resume
- ✅ PC detects USB drive when activated

Ready to use USB Transfer mode safely! 🚀
