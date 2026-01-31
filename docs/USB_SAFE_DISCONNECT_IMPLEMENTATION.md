# USB Safe Disconnect Implementation - January 30, 2026

## Overview
Implemented safe disconnect logic to prevent data corruption when deactivating USB Transfer mode while PC is still connected or transferring files.

## Problem
When user presses ACTION button to exit USB Transfer mode, the system immediately switches SD card back to APP mode. If the PC is still connected or actively transferring files, this can cause:
- Data corruption on SD card
- Incomplete file transfers
- File system errors
- Lost data

## Solution Implemented

### 1. USB Host Connection Detection
Added `usb_msc_sdcard_is_host_connected()` function that uses TinyUSB's `tud_mounted()` to check if PC is still connected.

**File**: `components/usb_flash_partition/usb_msc_sdcard.c`
```c
bool usb_msc_sdcard_is_host_connected(void)
{
    return tud_mounted();
}
```

### 2. Safe Disconnect Check in Resume Function
Modified `usb_msc_sdcard_resume_logging()` to check USB connection status before switching modes:

**Changes**:
- Check if USB host is connected using `tud_mounted()`
- Log warning if USB still connected
- Show warning on OLED display (3 seconds)
- Continue with deactivation but warn user
- Add 2 second delay to give user time to see warning

**Code Location**: `components/usb_flash_partition/usb_msc_sdcard.c` line ~240

```c
// SAFETY CHECK: Warn if USB host is still connected
bool usb_connected = tud_mounted();
if (usb_connected) {
    ESP_LOGW(TAG, "⚠️  USB host still connected!");
    ESP_LOGW(TAG, "⚠️  Please disconnect USB cable from LEFT port first");
    ESP_LOGW(TAG, "⚠️  Continuing anyway - data may be corrupted if transfer in progress");
    
    // Give user time to see warning on OLED (handled by caller)
    vTaskDelay(pdMS_TO_TICKS(2000));
} else {
    ESP_LOGI(TAG, "✓ USB host not connected - safe to deactivate");
}
```

### 3. OLED Warning Display
Updated `oled_display_handle_action_button()` to show warning screen when USB still connected:

**File**: `components/oled_display/oled_display.c` line ~1310

**Warning Screen** (shown for 3 seconds):
```
USB Transfer
-----------------
!!! WARNING !!!

USB still
connected!

Disconnect first
```

**Deactivating Screen** (if USB connected):
```
USB Transfer
-----------------
Deactivating...

(USB connected)
```

### 4. Header File Update
Added function declaration to public API:

**File**: `components/usb_flash_partition/usb_flash_partition.h`
```c
/**
 * Check if USB host (PC) is currently connected
 */
bool usb_msc_sdcard_is_host_connected(void);
```

## User Experience Flow

### Safe Disconnect (USB not connected)
1. User presses ACTION button in USB Transfer mode
2. System checks: USB not connected ✓
3. OLED shows "Deactivating..."
4. SD card switches to APP mode
5. Logging resumes
6. OLED shows "Logging resumed [OK]"

### Unsafe Disconnect (USB still connected)
1. User presses ACTION button in USB Transfer mode
2. System checks: USB still connected ⚠️
3. OLED shows warning screen for 3 seconds:
   - "!!! WARNING !!!"
   - "USB still connected!"
   - "Disconnect first"
4. OLED shows "Deactivating... (USB connected)"
5. System logs warnings to serial console
6. SD card switches to APP mode (with 2 second delay)
7. Logging resumes
8. OLED shows "Logging resumed [OK]"

## Serial Console Output

### Safe Disconnect
```
I (12345) USB_MSC_SD: 📥 Deactivating USB Transfer mode...
I (12345) USB_MSC_SD: ✓ USB host not connected - safe to deactivate
I (12345) USB_MSC_SD: 🔄 Switching SD card to APP mode...
I (12845) USB_MSC_SD: ▶️  Resuming data logging...
I (12845) USB_MSC_SD: ✓ Logging mode activated
```

### Unsafe Disconnect
```
I (12345) USB_MSC_SD: 📥 Deactivating USB Transfer mode...
W (12345) USB_MSC_SD: ⚠️  USB host still connected!
W (12345) USB_MSC_SD: ⚠️  Please disconnect USB cable from LEFT port first
W (12345) USB_MSC_SD: ⚠️  Continuing anyway - data may be corrupted if transfer in progress
I (14345) USB_MSC_SD: 🔄 Switching SD card to APP mode...
I (14845) USB_MSC_SD: ▶️  Resuming data logging...
I (14845) USB_MSC_SD: ✓ Logging mode activated
```

## Technical Details

### TinyUSB Connection Detection
- `tud_mounted()` returns `true` when USB host (PC) has enumerated the device
- Returns `false` when USB cable disconnected or PC hasn't recognized device
- Does NOT detect if file transfer is in progress (TinyUSB limitation)

### Limitations
1. **Cannot detect active file transfer**: TinyUSB doesn't provide API to check if read/write operations are in progress
2. **User must manually disconnect**: System warns but doesn't prevent deactivation
3. **No automatic eject**: Cannot programmatically tell PC to safely eject drive

### Why Not Block Deactivation?
We chose to warn but allow deactivation because:
1. User might need to exit USB mode urgently
2. PC might be connected but idle (no active transfer)
3. Blocking would require user to physically disconnect cable first
4. Warning gives user informed choice

## Best Practices for Users

### Recommended Procedure
1. **On PC**: Safely eject the drive (Windows: "Safely Remove Hardware", macOS: Eject)
2. **Wait**: Give PC time to complete eject (1-2 seconds)
3. **Disconnect**: Unplug USB cable from LEFT port
4. **Press ACTION**: Now safe to deactivate USB Transfer mode

### If Warning Appears
1. **Stop**: Don't continue if actively transferring files
2. **Check PC**: Look for file transfer progress indicators
3. **Eject**: Use PC's safe eject feature
4. **Disconnect**: Unplug USB cable
5. **Try again**: Press ACTION button again

## Testing Recommendations

### Test Case 1: Safe Disconnect
1. Enter USB Transfer mode (ACTION button)
2. Connect USB cable to LEFT port
3. PC recognizes drive
4. Safely eject drive on PC
5. Disconnect USB cable
6. Press ACTION button
7. **Expected**: No warning, smooth deactivation

### Test Case 2: Unsafe Disconnect (Warning)
1. Enter USB Transfer mode
2. Connect USB cable to LEFT port
3. PC recognizes drive
4. **Without ejecting**, press ACTION button
5. **Expected**: Warning screen appears for 3 seconds
6. **Expected**: Serial console shows warnings
7. **Expected**: Mode deactivates after delay

### Test Case 3: Active Transfer (Risky)
1. Enter USB Transfer mode
2. Connect USB cable
3. Start copying large file TO device
4. While transfer in progress, press ACTION button
5. **Expected**: Warning appears
6. **Expected**: File transfer may fail or corrupt
7. **Recommendation**: Don't do this in production!

## Files Modified

1. `components/usb_flash_partition/usb_msc_sdcard.c`
   - Added `usb_msc_sdcard_is_host_connected()` function
   - Modified `usb_msc_sdcard_resume_logging()` with safety check

2. `components/usb_flash_partition/usb_flash_partition.h`
   - Added function declaration

3. `components/oled_display/oled_display.c`
   - Modified `oled_display_handle_action_button()` case `OLED_MODE_USB_TRANSFER`
   - Added warning screen display

## Related Documentation
- `docs/note-10-usb-host-device-mode-fix.md` - USB Host/Device mode fix
- `USB_BUTTON_EXIT_FIX_JAN29.md` - Button exit implementation
- `USB_TRANSFER_MODE_JAN29.md` - USB Transfer mode overview
- `DATA_SAFETY_STRATEGY.md` - Overall data safety approach

## Future Improvements

### Possible Enhancements
1. **Block deactivation**: Require confirmation if USB connected
2. **Countdown timer**: Give user 5 seconds to cancel
3. **Auto-detect idle**: Wait for no USB activity before switching
4. **PC notification**: Send SCSI command to request eject (if supported)
5. **LED indicator**: Flash red LED when USB connected during exit attempt

### TinyUSB API Limitations
Currently TinyUSB doesn't provide:
- Active transfer detection
- Pending operation count
- Busy/idle status
- Request queue depth

If these become available in future TinyUSB versions, we can enhance the safety checks.

## Status
✅ **IMPLEMENTED** - Safe disconnect warnings active
⏳ **TESTING NEEDED** - User needs to test both safe and unsafe scenarios
📝 **DOCUMENTED** - Complete documentation provided

## Next Steps
1. Build and flash firmware: `idf.py build flash`
2. Test safe disconnect (eject first, then disconnect)
3. Test unsafe disconnect (press ACTION while connected)
4. Verify warning appears on OLED
5. Check serial console for warning messages
6. Confirm no data corruption after unsafe disconnect
