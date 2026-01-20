# DS3231 RTC (HW-084) Verification Guide

## Overview

The DS3231 RTC module with battery backup (HW-084) is your primary time source. It keeps accurate time even when the ESP32 is powered off, thanks to the CR2032 battery.

## Current Configuration

### Hardware Setup
```
DS3231 (HW-084) Module:
- VCC → 3.3V
- GND → GND
- SDA → GPIO 8 (shared with ToF sensor)
- SCL → GPIO 9 (shared with ToF sensor)
- Battery: CR2032 (keeps time when powered off)
```

### Software Configuration
From `config.txt`:
```
DS3231_ENABLED=true
DS3231_SDA_PIN=8
DS3231_SCL_PIN=9
```

## Time Source Priority

The system uses this priority order:

1. **DS3231 RTC** (Primary) ← **This is what you want!**
   - Battery-backed, keeps time when powered off
   - Most reliable, no WiFi needed
   - Accurate to ±2 minutes per year

2. **Internal RTC** (Fallback)
   - Loses time when powered off
   - Less accurate
   - Used if DS3231 fails

3. **NTP via WiFi** (Sync source)
   - Updates DS3231 periodically
   - Only used to sync, not for daily operation
   - Can be disabled after initial sync

## Verification Steps

### 1. Check DS3231 Detection

Look for these messages in the serial output:

**✅ Good (DS3231 Working):**
```
I (1234) SMART_TIME: ✓ DS3231 RTC initialized (SDA=8, SCL=9)
I (1235) SMART_TIME: ✓ Time available from DS3231 RTC
I (1236) SMART_TIME: Current time: 2026-01-19 15:30:45
I (1237) SMART_TIME: Sources: DS3231=enabled, WiFi=enabled
```

**❌ Bad (DS3231 Not Working):**
```
W (1234) SMART_TIME: DS3231 initialization failed: ESP_ERR_TIMEOUT
W (1235) SMART_TIME: No valid time source available
I (1236) SMART_TIME: Sources: DS3231=disabled, WiFi=enabled
```

### 2. Check Time Source

Send STATUS command via USB:
```bash
echo "STATUS" > /dev/ttyACM0
```

Look for:
```
STATUS:DS3231=OK,TIME_SOURCE=DS3231 RTC,SYNC_COUNT=1,MSC_ACTIVE=NO
```

**Key indicators:**
- `DS3231=OK` → Module detected and working
- `TIME_SOURCE=DS3231 RTC` → Using DS3231 as primary source ✅
- `TIME_SOURCE=Internal RTC` → Fallback mode (DS3231 not working) ❌

### 3. Test Battery Backup

**Test procedure:**
1. Power on system, verify DS3231 is working
2. Set correct time (via WiFi or USB)
3. Power off completely (unplug USB)
4. Wait 5 minutes
5. Power on again
6. Check if time is still correct

**Expected result:**
- Time should be accurate (within a few seconds)
- No need to sync again
- System immediately has correct time

### 4. Verify Shared I2C Bus

The DS3231 shares the I2C bus with the ToF sensor. Check that both work:

```
I (1234) SENSOR_MGR: I2C bus initialized (SDA=8, SCL=9)
I (1235) SENSOR_MGR: ✓ VL53L8CX initialized at address 0x29
I (1236) SMART_TIME: Using shared I2C bus from sensor manager
I (1237) SMART_TIME: ✓ DS3231 RTC initialized (SDA=8, SCL=9)
```

**Both should work together without conflicts.**

## Setting Initial Time

### Method 1: Via WiFi (Automatic)

If WiFi is configured in `config.txt`:
```
WIFI_ENABLED=true
WIFI_SSID=YourNetwork
WIFI_PASSWORD=YourPassword
```

The system will:
1. Connect to WiFi on startup
2. Get time from NTP server
3. Update DS3231 with correct time
4. Disconnect WiFi (saves power)
5. DS3231 keeps time from now on

**You'll see:**
```
I (5000) SMART_TIME: Attempting NTP sync...
I (7000) TIME_SYNC: ✓ Time synchronized from NTP
I (7001) SMART_TIME: ✓ DS3231 updated from NTP
I (7002) SMART_TIME: ✓ NTP sync successful, WiFi disconnected
```

### Method 2: Via USB Command (Manual)

Send time via USB Serial JTAG:
```bash
echo "TIME:2026-01-19 15:30:00" > /dev/ttyACM0
```

**You'll see:**
```
I (1234) USB_IF: 📅 Received time sync command: 2026-01-19 15:30:00
I (1235) SMART_TIME: Setting time manually: 2026-01-19 15:30:00
I (1236) SMART_TIME: ✓ DS3231 updated with synchronized time
I (1237) SMART_TIME: ✓ Time synchronized successfully
```

### Method 3: Python Script (Automated)

Use the provided script:
```bash
python tools/usb_time_sync.py
```

This automatically sends current computer time to the device.

## Troubleshooting

### Problem: DS3231 Not Detected

**Symptoms:**
```
W (1234) SMART_TIME: DS3231 initialization failed: ESP_ERR_TIMEOUT
```

**Solutions:**
1. **Check Wiring:**
   - VCC → 3.3V (NOT 5V!)
   - GND → GND
   - SDA → GPIO 8
   - SCL → GPIO 9

2. **Check I2C Address:**
   - DS3231 uses address 0x68
   - Run I2C scanner to verify:
   ```
   I (1234) SENSOR_MGR: I2C scan found devices: 0x29, 0x68
   ```
   - Should see both 0x29 (ToF) and 0x68 (DS3231)

3. **Check Pull-up Resistors:**
   - DS3231 module should have built-in 4.7kΩ pull-ups
   - If not, add external pull-ups on SDA and SCL

4. **Check Battery:**
   - Insert CR2032 battery in DS3231 module
   - Battery should be fresh (3V)

### Problem: Time Resets After Power Off

**Symptoms:**
- Time is correct when powered on
- After power cycle, time resets to 2020 or random date

**Solutions:**
1. **Check Battery:**
   - DS3231 needs CR2032 battery installed
   - Battery voltage should be > 2.5V
   - Replace if old

2. **Verify DS3231 is Primary Source:**
   - Check STATUS command shows `TIME_SOURCE=DS3231 RTC`
   - If shows `Internal RTC`, DS3231 is not working

3. **Set Time Again:**
   - Use WiFi or USB to set time
   - Verify DS3231 update message appears
   - Power cycle to test

### Problem: Time Drifts Over Days

**Symptoms:**
- Time is accurate initially
- After several days, time is off by minutes

**Solutions:**
1. **Enable Periodic NTP Sync:**
   - In `config.txt`: `SYNC_INTERVAL_HOURS=24`
   - System will auto-sync daily via WiFi
   - DS3231 stays accurate

2. **Check DS3231 Quality:**
   - Genuine DS3231 modules are accurate
   - Cheap clones may drift more
   - Consider replacing module

## Operating Without WiFi

**Good news:** Once DS3231 is set, WiFi is optional!

**Workflow:**
1. **Initial Setup (One Time):**
   - Connect to WiFi
   - System syncs time from NTP
   - DS3231 is updated
   - Disconnect WiFi

2. **Daily Operation:**
   - DS3231 keeps accurate time
   - No WiFi needed
   - Battery backup maintains time
   - System always has correct time

3. **Periodic Sync (Optional):**
   - Connect to WiFi once a month
   - System auto-syncs to correct any drift
   - Disconnect WiFi again

**To disable WiFi completely:**
```
# In config.txt
WIFI_ENABLED=false
```

System will rely 100% on DS3231 (which is fine!).

## Battery Life

**CR2032 Battery:**
- Typical capacity: 220 mAh
- DS3231 current: ~3 µA (very low)
- **Expected life: 8-10 years**

**When to replace:**
- Time resets after power off
- Time drifts significantly
- Battery voltage < 2.5V

## Summary

### ✅ What Works Now

1. **DS3231 is Primary Source**: System uses DS3231 first, always
2. **Battery Backup**: Time persists through power cycles
3. **Shared I2C Bus**: Works alongside ToF sensor
4. **Auto-Sync**: Optional WiFi sync keeps DS3231 accurate
5. **Manual Sync**: USB commands work anytime

### 🎯 Recommended Setup

1. **Install CR2032 battery** in DS3231 module
2. **Connect to WiFi once** to set initial time
3. **Disable WiFi** if not needed (optional)
4. **Enjoy accurate time** without WiFi dependency

### 📊 Expected Behavior

**On Startup:**
```
I (1234) SMART_TIME: ✓ DS3231 RTC initialized
I (1235) SMART_TIME: ✓ Time available from DS3231 RTC
I (1236) SMART_TIME: Current time: 2026-01-19 15:30:45
```

**During Operation:**
```
I (60000) MAIN: 📊 Logging: 3600 entries, 45.2% buffer
# Time is always accurate, no WiFi needed
```

**After Power Cycle:**
```
I (1234) SMART_TIME: ✓ Time available from DS3231 RTC
I (1235) SMART_TIME: Current time: 2026-01-19 16:45:23
# Time continues from where it left off
```

Your DS3231 RTC is already configured correctly and will be your primary time source! 🎉