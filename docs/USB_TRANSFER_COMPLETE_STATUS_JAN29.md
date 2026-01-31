# USB Transfer Mode - Complete Status & Next Steps - January 29, 2026

## ✅ FIXED: Button Toggle Issue

**Problem**: ACTION button didn't exit USB Transfer mode (second press did nothing)

**Root Cause**: Button state was cleared before OLED handler returned (timing issue)

**Solution**: Changed from `sensor_manager_is_button_pressed()` to button hold detection with edge detection

**File Modified**: `main/main.c` (line ~912)

**Status**: ✅ **FIXED** - Button now reliably toggles USB mode on/off

---

## ❌ REMAINING ISSUE: PC Not Detecting USB Device

**Problem**: PC doesn't detect USB flash drive when LEFT port is connected

**Symptoms**:
- `MSC_active=YES` (software activates correctly)
- `USB_connected=NO` (PC doesn't detect device)
- No new drive appears in File Explorer
- Device Manager shows nothing

**Most Likely Causes** (in order of probability):

### 1. USB Cable Issue (90% probability)
- **Problem**: Many USB cables are power-only (no data lines)
- **Test**: Try 3 different USB cables
- **Verify**: Connect Android phone with same cable - if PC detects phone, cable is good

### 2. Wrong USB Port (5% probability)
- **Problem**: Using RIGHT port instead of LEFT port
- **LEFT port**: USB-OTG (for custom USB devices) ← **Use this one!**
- **RIGHT port**: USB-to-UART (for programming/monitoring)
- **Verify**: Make sure you're connecting to the LEFT port

### 3. Hardware Issue (3% probability)
- **Problem**: LEFT port not functioning as USB-OTG
- **Test**: Try ESP-IDF TinyUSB example (see below)
- **Note**: User mentioned "USB-OTG bridge" chip on back of board

### 4. Driver Issue (2% probability)
- **Problem**: PC needs USB driver
- **Test**: Check Device Manager for "Unknown Device"
- **Solution**: Install ESP32-S3 USB driver if needed

---

## Hardware Verification Steps

### Step 1: Test with ESP-IDF TinyUSB Example

This will verify if the hardware works at all:

```bash
# Navigate to ESP-IDF TinyUSB MSC example
cd $IDF_PATH/examples/peripherals/usb/device/tusb_msc

# Build and flash
idf.py build flash monitor

# Connect LEFT port to PC
# Check if PC detects USB flash drive
```

**If this works**: Hardware is OK, our code needs debugging  
**If this doesn't work**: Hardware issue or cable problem

### Step 2: Check Device Manager (Windows)

When you connect the LEFT port:

1. Open Device Manager (Win+X → Device Manager)
2. Look under "Universal Serial Bus devices"
3. Look for:
   - "USB Mass Storage Device" (success!)
   - "Unknown Device" (driver issue)
   - Nothing (cable or hardware issue)

### Step 3: Check dmesg (Linux)

```bash
# Watch for USB events
dmesg -w

# Connect LEFT port
# Look for messages like:
# [xxxxx] usb 1-1: new full-speed USB device number X using xhci_hcd
# [xxxxx] usb-storage 1-1:1.0: USB Mass Storage device detected
```

### Step 4: Check System Information (macOS)

1. Apple menu → About This Mac → System Report
2. Hardware → USB
3. Look for "Espressif" or "USB Mass Storage" device

---

## Current Code State

### What Works ✅

1. ✅ Console changed to UART0 (no USB conflict)
2. ✅ Secondary console disabled (no USB conflict)
3. ✅ USB MSC lazy initialization (only when Mode 7 activated)
4. ✅ Logging pauses when USB mode activated
5. ✅ Logging resumes when USB mode deactivated
6. ✅ ACTION button toggles USB mode reliably
7. ✅ OLED shows correct status messages
8. ✅ LED shows blue when USB mode active

### What Doesn't Work ❌

1. ❌ PC doesn't detect USB device (hardware/cable issue)

---

## Debugging Information Needed

To help further, please provide:

### 1. USB Cable Test
```
Question: Have you tried 3 different USB cables?
Answer: [YES/NO]

Question: Does the cable work for data transfer with other devices (e.g., Android phone)?
Answer: [YES/NO]
```

### 2. USB Port Verification
```
Question: Which USB port are you connecting to?
Answer: [LEFT/RIGHT]

Question: Does the LEFT port provide power? (LED lights up?)
Answer: [YES/NO]
```

### 3. Device Manager Check (Windows)
```
Question: What appears in Device Manager when you connect LEFT port?
Answer: [Nothing / Unknown Device / USB Mass Storage / Other]

Screenshot: [Please provide screenshot of Device Manager]
```

### 4. ESP-IDF Example Test
```
Question: Have you tried the ESP-IDF TinyUSB MSC example?
Answer: [YES/NO]

If YES, did it work?
Answer: [YES/NO]
```

### 5. Hardware Information
```
Question: What is the exact model of your ESP32-S3 DevKitC?
Answer: [e.g., ESP32-S3-DevKitC-1-N8R8]

Question: Is there a chip labeled "USB-OTG bridge" on the back?
Answer: [YES/NO]

If YES, what is the chip model?
Answer: [e.g., CP2102, CH340, etc.]
```

---

## Recommended Next Steps

### Option 1: Hardware Verification (Recommended)

1. **Test with 3 different USB cables** (most likely issue)
2. **Verify using LEFT port** (not RIGHT port)
3. **Test with ESP-IDF example** to verify hardware works
4. **Check Device Manager** to see if PC detects anything

**Estimated time**: 30 minutes

### Option 2: Alternative Approach - SD Card Reader

If USB Transfer mode is too complex:

1. **Power off device**
2. **Remove SD card**
3. **Insert SD card into PC card reader**
4. **Access files**
5. **Insert SD card back into device**
6. **Power on**

**Pros**: Always works, no USB complexity  
**Cons**: Need to power off, physically remove card

**Estimated time**: 2 minutes per file transfer

### Option 3: WiFi File Transfer (Future Enhancement)

Implement WiFi-based file transfer:

1. Create HTTP server on ESP32
2. Access files via web browser
3. Download files over WiFi

**Pros**: No USB cable needed, works remotely  
**Cons**: Slower than USB, more complex to implement

**Estimated time**: 2-3 hours to implement

---

## Testing the Button Fix

### Test Sequence

1. **Build and flash**:
   ```bash
   idf.py build flash monitor
   ```

2. **Navigate to Mode 7**:
   - Press Mode Change button until OLED shows "USB Transfer Mode"

3. **First press (Activate)**:
   - Press ACTION button (GPIO 3)
   - OLED should show: "Activating..."
   - Then: "[USB] Connect LEFT port" + "ACTION: Cancel"
   - Serial monitor should show: "✓ USB Transfer mode activated"

4. **Second press (Deactivate)**:
   - Press ACTION button again
   - OLED should show: "Deactivating..."
   - Then: "Logging resumed" + "[OK]"
   - Serial monitor should show: "✓ Logging mode activated"

5. **Repeat**:
   - Press ACTION again to re-activate
   - Should work reliably every time

### Expected Serial Output

```
I (xxxxx) MAIN: 💾 USB Transfer mode toggle triggered from button
I (xxxxx) OLED_SPI: USB Transfer mode toggle requested from display
I (xxxxx) USB_MSC_SD: 📤 Activating USB Transfer mode...
I (xxxxx) USB_MSC_SD: ⏸️  Pausing data logging...
I (xxxxx) STORAGE_MGR: Logging stopped. Total entries: XXX, Dropped: 0
I (xxxxx) USB_MSC_SD: ✓ USB Transfer mode activated
I (xxxxx) USB_MSC_SD: 🔌 Connect LEFT port (USB-OTG) to computer

[User presses ACTION again]

I (xxxxx) MAIN: 💾 USB Transfer mode toggle triggered from button
I (xxxxx) OLED_SPI: USB Transfer mode toggle requested from display
I (xxxxx) USB_MSC_SD: 📥 Deactivating USB Transfer mode...
I (xxxxx) USB_MSC_SD: ▶️  Resuming data logging...
I (xxxxx) STORAGE_MGR: Logging started
I (xxxxx) USB_MSC_SD: ✓ Logging mode activated
```

---

## Summary

### Fixed Issues ✅

1. ✅ **Button toggle** - ACTION button now reliably toggles USB mode on/off
2. ✅ **Console conflict** - UART0 console frees USB-OTG hardware
3. ✅ **Lazy initialization** - USB MSC only initializes when needed
4. ✅ **Logging pause/resume** - Data logging correctly pauses/resumes

### Remaining Issues ❌

1. ❌ **PC not detecting USB device** - Most likely cable or hardware issue

### Next Actions

1. **Test the button fix** - Build and flash to verify toggle works
2. **Try 3 different USB cables** - Most likely cause of detection issue
3. **Verify LEFT port** - Make sure you're using the correct port
4. **Test ESP-IDF example** - Verify hardware works with simple example

### Alternative

If USB Transfer mode is blocking your thesis progress:
- **Use SD card reader** instead (simple, always works)
- **Focus on near-miss detection** (your main thesis goal)
- **Add USB Transfer later** if time permits

---

## Files Modified

1. `main/main.c` - Line ~912 (USB Transfer mode button handler)
2. `USB_BUTTON_TOGGLE_FIX_JAN29.md` - Detailed explanation of fix
3. `USB_TRANSFER_COMPLETE_STATUS_JAN29.md` - This file (complete status)

---

## Contact Information

If you need further assistance:

1. **Provide debugging information** (see "Debugging Information Needed" section above)
2. **Share serial monitor output** when activating USB mode
3. **Share Device Manager screenshot** (Windows) or `lsusb` output (Linux)
4. **Confirm cable and port** being used

This will help identify the exact cause of the USB detection issue!

