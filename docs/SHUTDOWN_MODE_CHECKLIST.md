# Shutdown Mode - Implementation Checklist

**Date**: January 27, 2026  
**Status**: ✅ **BUILD SUCCESSFUL - READY TO FLASH AND TEST**

---

## ✅ Implementation Complete

### Code Changes

| File | Status | Changes |
|------|--------|---------|
| `components/oled_display/include/oled_display.h` | ✅ | Added OLED_MODE_SHUTDOWN enum, added shutdown confirmation function |
| `components/oled_display/oled_display.c` | ✅ | Added shutdown state variables, display logic, confirmation handler |
| `main/main.c` | ✅ | Added esp_sleep.h include, shutdown button handling, deep sleep sequence |
| `docs/Current Modes and Layouts.md` | ✅ | Added Mode 6 documentation, updated summaries |
| `docs/SHUTDOWN_MODE_IMPLEMENTATION.md` | ✅ | Complete implementation documentation |
| `components/oled_display/CMakeLists.txt` | ✅ | No changes needed (all in existing files) |

### Build Status

```
✅ Build successful
✅ No compilation errors
✅ No warnings
✅ Binary size: 0x120390 bytes (1,180,560 bytes)
✅ Free space: 0x5fc70 bytes (392,304 bytes / 25%)
```

---

## Implementation Summary

### Features Implemented

1. ✅ **Mode 6: Shutdown** - New display mode added to cycle
2. ✅ **Two-step confirmation** - Prevents accidental shutdown
3. ✅ **10-second timeout** - Auto-cancels if no confirmation
4. ✅ **Cancel anytime** - MODE button cancels and returns to Dashboard
5. ✅ **Visual warnings** - Inverted yellow zone for high visibility
6. ✅ **Countdown timer** - Shows remaining time to confirm
7. ✅ **LED indicators** - RED LED during shutdown
8. ✅ **Data protection** - Stops logging, flushes all data
9. ✅ **Deep sleep** - Proper power-down sequence
10. ✅ **Console logging** - Clear step-by-step output

### Safety Mechanisms

1. ✅ **Accidental press protection** - Requires two button presses
2. ✅ **Timeout protection** - Confirmation expires after 10 seconds
3. ✅ **Cancel protection** - Can cancel at any stage
4. ✅ **Data integrity** - All data flushed before shutdown
5. ✅ **State reset** - Confirmation state cleared on mode change

---

## Testing Plan

### Phase 1: Basic Functionality ⏳

- [ ] Flash firmware to device
- [ ] Verify device boots normally
- [ ] Cycle through all modes (Dashboard → ToF → IMU → Status → Near Miss → WiFi → **Shutdown**)
- [ ] Verify Shutdown mode appears after WiFi Settings
- [ ] Verify Shutdown mode displays warning screen

### Phase 2: Initial Warning Screen ⏳

- [ ] Enter Shutdown mode
- [ ] Verify yellow zone is inverted
- [ ] Verify "!SHUTDOWN MODE!" text is visible
- [ ] Verify "Press ACTION" instruction is visible
- [ ] Verify blue zone shows:
  - "This will:"
  - "1. Stop logging"
  - "2. Save all data"
  - "3. Power off"
  - "Press MODE cancel"

### Phase 3: First Confirmation ⏳

- [ ] Press ACTION button (GPIO 3)
- [ ] Verify screen changes to confirmation mode
- [ ] Verify yellow zone shows "!CONFIRM SHUTDOWN!"
- [ ] Verify "Press ACTION again" instruction
- [ ] Verify countdown timer starts at 10 seconds
- [ ] Verify timer counts down (10, 9, 8, 7, ...)
- [ ] Verify LED turns RED
- [ ] Verify console shows "Shutdown button pressed"

### Phase 4: Cancellation ⏳

- [ ] Enter confirmation mode (first ACTION press)
- [ ] Press MODE button (GPIO 20)
- [ ] Verify returns to Dashboard mode
- [ ] Verify confirmation state is reset
- [ ] Verify can re-enter Shutdown mode
- [ ] Verify console shows "Shutdown cancelled"

### Phase 5: Timeout ⏳

- [ ] Enter confirmation mode (first ACTION press)
- [ ] Wait 10+ seconds without pressing ACTION
- [ ] Verify display shows "TIMEOUT - Cancelled"
- [ ] Verify confirmation state is reset
- [ ] Verify can re-enter confirmation mode

### Phase 6: Final Confirmation & Shutdown ⏳

- [ ] Enter confirmation mode (first ACTION press)
- [ ] Press ACTION button again within 10 seconds
- [ ] Verify display shows "!SHUTTING DOWN!"
- [ ] Verify display shows "Saving data..."
- [ ] Verify display shows "Please wait"
- [ ] Verify console output:
  ```
  W (xxxxx) MAIN: 🔴 SHUTDOWN CONFIRMED - POWERING DOWN
  I (xxxxx) MAIN: Stopping data logging...
  I (xxxxx) MAIN: Flushing data to SD card...
  I (xxxxx) MAIN: Stopping sensors...
  I (xxxxx) MAIN: ✓ All data saved
  I (xxxxx) MAIN: ✓ Safe to power off
  I (xxxxx) MAIN: You can now switch off the device
  I (xxxxx) MAIN: Entering deep sleep...
  ```
- [ ] Verify LED turns RED then OFF
- [ ] Verify system enters deep sleep
- [ ] Verify device stops responding

### Phase 7: Data Integrity ⏳

- [ ] Before shutdown: Note current log file size
- [ ] Perform shutdown sequence
- [ ] Power off device
- [ ] Remove SD card
- [ ] Check SD card on computer:
  - [ ] Log file exists
  - [ ] File size matches or is larger (flushed data)
  - [ ] File is not corrupted
  - [ ] CSV format is valid
  - [ ] All data is readable
  - [ ] No partial writes
  - [ ] Last timestamp is close to shutdown time

### Phase 8: Edge Cases ⏳

- [ ] **Empty buffer:** Shutdown with no logged data
- [ ] **Full buffer:** Shutdown with full log buffer
- [ ] **No SD card:** Shutdown without SD card (should not crash)
- [ ] **Sensors offline:** Shutdown with ToF/IMU offline
- [ ] **Multiple cycles:** Cancel → Retry → Cancel → Retry → Confirm
- [ ] **Fast button press:** Press ACTION twice very quickly
- [ ] **During logging:** Shutdown while actively logging data
- [ ] **During USB MSC:** Shutdown while USB flash drive active

### Phase 9: LED Behavior ⏳

- [ ] Normal operation: Distance-based LED color
- [ ] First ACTION press: LED turns RED
- [ ] During confirmation: LED stays RED
- [ ] During shutdown: LED stays RED
- [ ] After deep sleep: LED turns OFF
- [ ] After cancel: LED returns to distance-based color

### Phase 10: Console Output ⏳

- [ ] Verify all log messages appear
- [ ] Verify log levels are correct (INFO, WARN)
- [ ] Verify timestamps are sequential
- [ ] Verify no error messages
- [ ] Verify shutdown sequence is complete
- [ ] Verify "Safe to power off" message appears

---

## Known Issues

### None Currently ✅

All code compiles successfully with no warnings or errors.

---

## Post-Testing Actions

### If All Tests Pass ✅

1. [ ] Mark all checklist items as complete
2. [ ] Document any observations
3. [ ] Update firmware version number
4. [ ] Create release notes
5. [ ] Commit changes to git

### If Issues Found ⚠️

1. [ ] Document the issue
2. [ ] Note reproduction steps
3. [ ] Check console output for errors
4. [ ] Review relevant code sections
5. [ ] Fix and retest

---

## Quick Test Procedure

**Minimum test to verify shutdown works:**

1. Flash firmware
2. Boot device
3. Cycle to Shutdown mode (press MODE 6 times)
4. Press ACTION button (first confirmation)
5. Wait for countdown to appear
6. Press ACTION button again (final confirmation)
7. Verify "SHUTTING DOWN!" appears
8. Wait for deep sleep
9. Check SD card for data integrity

**Expected time:** ~30 seconds

---

## Files to Review Before Testing

1. `main/main.c` - Lines 799-860 (shutdown sequence)
2. `components/oled_display/oled_display.c` - Lines 789-833 (display), 970-1014 (handler)
3. `components/oled_display/include/oled_display.h` - Lines 18-24 (enum), 113-114 (function)

---

## Build Information

**Compiler:** xtensa-esp32s3-elf-gcc (ESP-IDF v5.5.2)  
**Target:** ESP32-S3  
**Build Date:** January 27, 2026  
**Binary Size:** 1,180,560 bytes  
**Free Space:** 392,304 bytes (25%)

---

## Next Steps

1. ✅ **Build complete** - Ready to flash
2. ⏳ **Flash to device** - `idf.py flash`
3. ⏳ **Monitor output** - `idf.py monitor`
4. ⏳ **Run tests** - Follow testing plan above
5. ⏳ **Verify data integrity** - Check SD card after shutdown
6. ⏳ **Document results** - Update this checklist

---

**Status:** ✅ **READY FOR TESTING**  
**Confidence Level:** HIGH (all code compiles, no warnings)  
**Risk Level:** LOW (safe shutdown with data protection)
