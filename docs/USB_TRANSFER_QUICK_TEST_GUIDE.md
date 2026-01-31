# USB Transfer Mode - Quick Test Guide - January 29, 2026

## 🚀 Quick Start: Test the Button Fix

### 1. Build and Flash (2 minutes)

```bash
idf.py build flash monitor
```

### 2. Navigate to USB Transfer Mode (10 seconds)

Press **Mode Change button** (GPIO 20) repeatedly until OLED shows:
```
USB Transfer Mode
Press ACTION
```

### 3. Test Button Toggle (30 seconds)

#### First Press - Activate USB Mode
Press **ACTION button** (GPIO 3)

**Expected OLED**:
```
USB Transfer
Initializing...
  ↓
  [USB]
Connect LEFT port
ACTION: Cancel
```

**Expected Serial Monitor**:
```
I (xxxxx) MAIN: 💾 USB Transfer mode toggle triggered from button
I (xxxxx) USB_MSC_SD: 📤 Activating USB Transfer mode...
I (xxxxx) USB_MSC_SD: ✓ USB Transfer mode activated
```

#### Second Press - Deactivate USB Mode
Press **ACTION button** again

**Expected OLED**:
```
USB Transfer
Deactivating...
  ↓
Logging resumed
  [OK]
```

**Expected Serial Monitor**:
```
I (xxxxx) MAIN: 💾 USB Transfer mode toggle triggered from button
I (xxxxx) USB_MSC_SD: 📥 Deactivating USB Transfer mode...
I (xxxxx) USB_MSC_SD: ✓ Logging mode activated
```

#### Third Press - Re-activate
Press **ACTION button** again

Should activate USB mode again (same as first press)

### 4. Result

✅ **SUCCESS**: Button toggles USB mode on/off reliably  
❌ **FAILURE**: Button doesn't respond or only works once

---

## 🔌 Quick Start: Test USB Detection

### 1. Activate USB Mode

Follow steps above to activate USB Transfer mode

### 2. Connect USB Cable

**CRITICAL**: Use the **LEFT port** (USB-OTG), not the RIGHT port!

```
ESP32-S3 DevKitC Board:
┌─────────────────────────┐
│  [LEFT]      [RIGHT]    │  ← Two USB ports
│   USB-OTG    USB-UART   │
│                         │
│   Use LEFT port! ───────┘
└─────────────────────────┘
```

### 3. Check PC Detection

#### Windows
1. Open File Explorer
2. Look for new drive (e.g., "D:", "E:", "F:")
3. If no drive appears, check Device Manager:
   - Win+X → Device Manager
   - Look under "Universal Serial Bus devices"
   - Look for "USB Mass Storage Device" or "Unknown Device"

#### Linux
```bash
# Check for new USB device
lsusb

# Check for new drive
lsblk

# Check kernel messages
dmesg | tail -20
```

#### macOS
1. Check Finder for new drive
2. Or: Apple menu → About This Mac → System Report → USB

### 4. Result

✅ **SUCCESS**: New drive appears, can access files  
❌ **FAILURE**: No drive appears, nothing in Device Manager

---

## 🔧 Troubleshooting: PC Not Detecting USB

### Problem: No USB Device Detected

Try these in order:

#### 1. Try Different USB Cable (90% fix rate)

**Test 3 different cables**:
- Cable 1: [  ] Tried - Result: ___________
- Cable 2: [  ] Tried - Result: ___________
- Cable 3: [  ] Tried - Result: ___________

**How to verify cable is good**:
- Connect Android phone with same cable
- If PC detects phone → Cable is good
- If PC doesn't detect phone → Cable is bad (power-only)

#### 2. Verify LEFT Port (5% fix rate)

**Check you're using the correct port**:
- [  ] Using LEFT port (USB-OTG) ✅
- [  ] Using RIGHT port (USB-UART) ❌

**How to identify**:
- LEFT port: Usually labeled "USB" or "OTG"
- RIGHT port: Usually labeled "UART" or "COM"
- When in doubt: Try both ports!

#### 3. Test with ESP-IDF Example (3% fix rate)

**Verify hardware works**:
```bash
cd $IDF_PATH/examples/peripherals/usb/device/tusb_msc
idf.py build flash monitor
```

Connect LEFT port to PC. Does it work?
- [  ] YES → Hardware OK, our code needs debugging
- [  ] NO → Hardware issue or cable problem

#### 4. Check Device Manager (2% fix rate)

**Look for driver issues**:
- Open Device Manager (Win+X → Device Manager)
- Look under "Universal Serial Bus devices"
- What do you see?
  - [  ] Nothing → Cable or hardware issue
  - [  ] Unknown Device → Driver issue
  - [  ] USB Mass Storage Device → Success!

---

## 📋 Quick Checklist

### Before Asking for Help

Please complete this checklist:

#### Hardware
- [  ] Tried 3 different USB cables
- [  ] Verified using LEFT port (not RIGHT port)
- [  ] Tested cable with Android phone (works for data transfer)
- [  ] Checked Device Manager / lsusb output

#### Software
- [  ] Built and flashed latest code
- [  ] Button toggle works (can activate/deactivate USB mode)
- [  ] Serial monitor shows "USB Transfer mode activated"
- [  ] Tried ESP-IDF TinyUSB example

#### Debugging Info
- USB cable brand/model: ___________
- USB port used: [LEFT / RIGHT]
- Device Manager shows: ___________
- ESP32-S3 board model: ___________
- USB-OTG bridge chip (if any): ___________

---

## 🎯 Expected Behavior

### When Everything Works

1. **Press ACTION in Mode 7**
   - OLED shows "Activating..."
   - Serial: "USB Transfer mode activated"
   - LED turns blue

2. **Connect LEFT port to PC**
   - PC detects new USB device
   - New drive appears (e.g., "D:")
   - Can access SD card files

3. **Access files on PC**
   - Open drive in File Explorer
   - See CSV log files
   - Can copy files to PC

4. **Disconnect USB cable**
   - Remove cable from PC first
   - Then press ACTION to deactivate

5. **Press ACTION again**
   - OLED shows "Deactivating..."
   - Serial: "Logging mode activated"
   - LED returns to normal color

---

## 🆘 Still Not Working?

### Option 1: Use SD Card Reader

**Simplest solution** (always works):

1. Power off device
2. Remove SD card
3. Insert SD card into PC card reader
4. Access files
5. Insert SD card back into device
6. Power on

**Time**: 2 minutes per file transfer

### Option 2: Focus on Thesis

**Remember**: Your thesis is about **near-miss detection**, not USB file transfer!

If USB Transfer mode is blocking your progress:
- Use SD card reader instead
- Focus on collecting data and training ML model
- Add USB Transfer later if time permits

**Don't let USB complexity block your thesis progress!**

---

## 📞 Need Help?

If you've completed the checklist and it still doesn't work, provide:

1. **Serial monitor output** when activating USB mode
2. **Device Manager screenshot** (Windows) or `lsusb` output (Linux)
3. **USB cable brand/model** and verification test result
4. **ESP32-S3 board model** and any chips on the back
5. **ESP-IDF example test result** (did it work?)

This information will help identify the exact cause!

