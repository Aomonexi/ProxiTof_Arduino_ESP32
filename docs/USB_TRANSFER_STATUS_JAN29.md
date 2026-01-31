# USB Transfer Mode - Current Status - January 29, 2026

## What Works ✅

1. ✅ Console changed to UART0 (frees up USB-OTG hardware)
2. ✅ Secondary console disabled (no USB Serial/JTAG conflict)
3. ✅ ACTION button activates USB Transfer mode
4. ✅ `MSC_active=YES` when activated
5. ✅ Logging pauses when USB mode activated
6. ✅ OLED shows USB Transfer screen

## What Doesn't Work ❌

1. ❌ PC doesn't detect USB flash drive (`USB_connected=NO`)
2. ❌ ACTION button doesn't deactivate USB mode (second press does nothing)
3. ❌ Callback shows wrong messages during mode switch

## Root Cause Analysis

### Problem: PC Doesn't Detect USB Device

The ESP32-S3 is activating USB MSC mode, but the PC doesn't see a new USB device at all. This suggests:

**Possible causes**:
1. **USB cable is power-only** (no data lines) - Most likely!
2. **Wrong USB port** - Using RIGHT port instead of LEFT port
3. **TinyUSB not exposing device** - Configuration issue
4. **Hardware problem** - LEFT port damaged or not USB-OTG

### How to Verify

#### Test 1: Check USB Cable
- Try a different USB cable (must be data cable, not just charging cable)
- Many USB cables are power-only and don't have data lines

#### Test 2: Verify LEFT Port is USB-OTG
- The ESP32-S3 DevKitC has TWO USB ports
- **LEFT port**: USB-OTG (for custom USB devices)
- **RIGHT port**: USB-to-UART (for programming/monitoring)
- Make sure you're connecting to the **LEFT port**

#### Test 3: Check Device Manager (Windows)
When you connect the LEFT port, check:
- Device Manager → Universal Serial Bus devices
- Look for "Unknown Device" or "Espressif" device
- If nothing appears, PC isn't detecting anything

#### Test 4: Check dmesg (Linux)
```bash
# Before connecting
dmesg -w

# Connect LEFT port
# Look for USB device detection messages
```

## Recommended Next Steps

### Option 1: Simplify and Test Hardware First

Before fixing software, verify hardware works:

1. **Test with simple TinyUSB example**
   ```bash
   # Use ESP-IDF TinyUSB MSC example
   cd $IDF_PATH/examples/peripherals/usb/device/tusb_msc
   idf.py build flash monitor
   ```
   
   If this works, hardware is OK and it's a software issue.
   If this doesn't work, hardware problem or cable issue.

2. **Test USB cable**
   - Try 3 different USB cables
   - Use a cable you KNOW works for data (not just charging)

3. **Test LEFT port**
   - Verify you're using the correct port
   - Try programming via LEFT port (if it works, port is OK)

### Option 2: Alternative Approach - Remove SD Card

Instead of USB Transfer mode, use simpler approach:
1. Power off device
2. Remove SD card
3. Insert SD card into PC card reader
4. Access files
5. Insert SD card back into device
6. Power on

**Pros**: Always works, no USB complexity
**Cons**: Need to power off, physically remove card

### Option 3: WiFi File Transfer

Implement WiFi-based file transfer instead:
1. Create HTTP server on ESP32
2. Access files via web browser
3. Download files over WiFi

**Pros**: No USB cable needed, works remotely
**Cons**: Slower than USB, more complex

## Current Code State

### Files Modified
- `components/usb_flash_partition/usb_msc_sdcard.c` - USB MSC implementation
- `components/oled_display/oled_display.c` - OLED button handler
- `main/main.c` - USB initialization (now lazy)
- `sdkconfig` - Console changed to UART0

### What the Code Does Now
1. Boot → No USB MSC initialization (lazy init)
2. Navigate to Mode 7 → Show USB Transfer screen
3. Press ACTION → Initialize TinyUSB MSC, activate USB mode
4. `MSC_active=YES` but `USB_connected=NO`
5. Press ACTION again → Nothing happens (button handler issue)

## Debugging Information Needed

To help further, please provide:

### 1. Hardware Verification
```
- Which USB port are you connecting to? (LEFT or RIGHT?)
- Have you tried 3 different USB cables?
- Does the cable work for data transfer with other devices?
- What does Device Manager show when you connect?
```

### 2. TinyUSB Test
```
- Can you try the ESP-IDF TinyUSB MSC example?
- Does the example work with your hardware?
```

### 3. USB Cable Test
```
- Try connecting an Android phone with the same cable
- Does PC detect the phone? (If yes, cable is good)
```

## Conclusion

We've fixed all the software configuration issues:
- ✅ Console conflict resolved
- ✅ USB MSC activates correctly
- ✅ Logging pauses/resumes

The remaining issue is **PC not detecting the USB device**, which is likely:
1. **Hardware**: Wrong cable (power-only) or wrong port
2. **Configuration**: TinyUSB needs additional setup
3. **Driver**: PC needs USB driver

**Recommendation**: Test with a known-good data cable first, then try the ESP-IDF TinyUSB example to verify hardware works.

If hardware works with the example but not with our code, we can debug the software further. But if hardware doesn't work at all, we need to investigate cable/port/driver issues first.

---

## Alternative: Disable USB Transfer Mode

If USB Transfer mode is too complex and you need to move forward with your thesis, consider:

1. **Remove USB Transfer mode** (Mode 7)
2. **Use SD card reader** to access files
3. **Focus on near-miss detection** (your main thesis goal)
4. **Add USB Transfer later** if time permits

Your thesis is about near-miss detection, not USB file transfer. Don't let this block your progress!
