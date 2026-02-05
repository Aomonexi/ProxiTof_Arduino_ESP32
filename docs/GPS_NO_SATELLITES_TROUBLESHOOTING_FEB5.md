# GPS No Satellites Troubleshooting - February 5, 2026

## Problem
- PPS LED blinking (GPS module powered and working)
- NMEA sentences being parsed (7500+ sentences)
- 10+ minutes elapsed
- **0 satellites visible**
- No GPS fix

## Root Cause Analysis

### PPS Blinking = GPS Module Working
✅ GPS module is powered
✅ GPS module firmware is running
✅ UART communication is working
✅ NMEA sentences are being received

### 0 Satellites = No Sky View
❌ GPS antenna cannot see satellites
❌ Antenna placement issue
❌ Possible indoor location
❌ Possible antenna not connected

## Quick Diagnostic Checklist

### 1. Antenna Connection ⚠️ CRITICAL
```
Check:
□ Is antenna connected to GPS module?
□ Is U.FL/SMA connector fully seated?
□ Is antenna cable damaged?
□ Is antenna powered (if active antenna)?
```

**Most Common Issue:** Antenna not connected or loose connection

### 2. Antenna Placement
```
Check:
□ Is antenna outdoors or near window?
□ Is antenna facing up (clear sky view)?
□ Is antenna away from metal objects?
□ Is antenna away from walls/roof?
```

**Required:** Clear view of sky (at least 45° above horizon)

### 3. Location
```
Check:
□ Are you indoors? (GPS won't work)
□ Are you in a basement? (GPS won't work)
□ Are you in a building? (GPS won't work)
□ Are you near tall buildings? (May block satellites)
```

**Required:** Outdoor location or near window

### 4. Antenna Type
```
Check:
□ Active antenna (needs 3.3V power)?
□ Passive antenna (no power needed)?
□ Is power connected (if active)?
```

**NEO-M9N:** Supports both active and passive antennas

## Immediate Actions

### Action 1: Move Outdoors
**Do this first!**

1. Take device outside
2. Place antenna with clear sky view
3. Wait 2-3 minutes
4. Check for satellites

**Expected:** Should see 4-12 satellites within 2-3 minutes outdoors

### Action 2: Check Antenna Connection
**If still no satellites outdoors:**

1. Power off device
2. Check antenna connector (U.FL or SMA)
3. Ensure connector is fully seated
4. Check for damaged cable
5. Power on and test again

### Action 3: Check Serial Logs
**Look for these messages:**

```
Good signs:
I (1234) GPS: 📡 GPS UART active: 82 bytes received
I (1235) GPS: Sentences parsed: 7500

Bad signs (none of these):
W (5000) GPS: ⚠️  No GPS data received for 5 seconds
E (1234) GPS: Failed to read UART
```

**If you see "No GPS data":** UART wiring issue
**If you see sentences but no satellites:** Antenna issue

## Understanding GPS Acquisition

### Cold Start (First Time)
```
Time to First Fix: 26-30 seconds
Requirements:
  - Clear sky view
  - Stationary position
  - Outdoor location
  - 4+ satellites visible

Timeline:
  0-10s:  Searching for satellites
  10-20s: Downloading almanac data
  20-30s: Calculating position
  30s+:   First fix acquired
```

### What You Should See

**After 30 seconds outdoors:**
```
┌────────────────────────┐
│ GPS: 3D FIX (8sat)     │ ← Should show satellites
│ HDOP: 1.2              │
├────────────────────────┤
│ Lat: 1.234567N         │
│ Lon: 103.123456E       │
│ ...                    │
└────────────────────────┘
```

**What you're seeing now:**
```
┌────────────────────────┐
│ GPS: NO FIX (0sat)     │ ← 0 satellites = antenna issue
│ HDOP: 99.9             │
├────────────────────────┤
│ Waiting for fix...     │
│ Sentences: 7500        │ ← GPS working, but no satellites
└────────────────────────┘
```

## Advanced Diagnostics

### Check NMEA Sentences
**Look at serial monitor for:**

```
Good (satellites visible):
$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47
                                      ↑  ↑↑
                                      |  └─ 8 satellites
                                      └─ Fix quality: 1 (GPS fix)

Bad (no satellites):
$GPGGA,123519,,,,,0,00,99.9,,,M,,M,,*47
                  ↑  ↑↑
                  |  └─ 0 satellites
                  └─ Fix quality: 0 (no fix)
```

### Check Satellite Info
**Look for GPGSV sentences:**

```
Good (satellites visible):
$GPGSV,3,1,12,01,45,123,42,02,30,234,38,03,60,045,45,04,15,300,35*7A
          ↑↑
          └─ 12 satellites in view

Bad (no satellites):
$GPGSV,1,1,00*79
          ↑↑
          └─ 0 satellites in view
```

## Configuration Check

### Verify GPS Settings
**Check `/sdcard/config.txt`:**

```ini
# GPS should be enabled
GPS_DYNAMIC_MODEL=10        # Bike mode
gps_update_rate=5           # 5 Hz update rate
```

### Verify Wiring
**GPS Neo-M9N on UART1:**
```
ESP32-S3    GPS Module
GPIO 17  →  RX (ESP32 transmits to GPS)
GPIO 18  ←  TX (GPS transmits to ESP32)
3.3V     →  VCC
GND      →  GND
Antenna  →  U.FL/SMA connector
```

**Common Wiring Mistakes:**
- ❌ TX/RX swapped (would show "No GPS data")
- ❌ Antenna not connected (would show 0 satellites) ← **YOUR ISSUE**
- ❌ Wrong voltage (5V instead of 3.3V)
- ❌ No ground connection

## Testing Procedure

### Test 1: Outdoor Test (2 minutes)
```
1. Take device outside
2. Place antenna flat with clear sky view
3. Wait 2 minutes
4. Check display for satellites

Expected: 4-12 satellites within 2 minutes
If still 0: Antenna problem
```

### Test 2: Antenna Connection Test
```
1. Power off device
2. Disconnect antenna
3. Reconnect antenna (ensure click/snap)
4. Power on device
5. Go outside and test

Expected: Should hear/feel connector click
If no click: Connector not fully seated
```

### Test 3: Different Location Test
```
1. Try different outdoor locations:
   - Open field (best)
   - Parking lot (good)
   - Balcony (okay)
   - Near window (marginal)

Expected: More open = more satellites
If still 0 everywhere: Antenna hardware issue
```

## Expected Timeline

### Outdoor with Good Antenna
```
Time    Satellites  Status
0s      0          Searching...
10s     2-4        Acquiring...
20s     4-8        Calculating...
30s     8-12       FIX ACQUIRED ✓
```

### Indoor or Bad Antenna
```
Time    Satellites  Status
0s      0          Searching...
10s     0          Searching...
60s     0          Searching...
600s    0          Searching... ← YOUR SITUATION
```

## Solutions by Scenario

### Scenario 1: Indoor Testing
**Problem:** GPS doesn't work indoors
**Solution:** Move outdoors or near window

### Scenario 2: Antenna Not Connected
**Problem:** Antenna loose or disconnected
**Solution:** Reconnect antenna firmly

### Scenario 3: Wrong Antenna Type
**Problem:** Active antenna without power
**Solution:** Check antenna power connection

### Scenario 4: Obstructed View
**Problem:** Buildings/trees blocking satellites
**Solution:** Move to open area

### Scenario 5: Faulty Antenna
**Problem:** Antenna damaged or defective
**Solution:** Replace antenna

## Quick Fix Commands

### Increase GPS Logging (Temporary)
**To see more GPS debug info:**

Edit `components/gps/gps.c` line ~280:
```c
// Change from 5 seconds to 1 second
if (now - last_debug_time > 1000) {  // Was 5000
    ESP_LOGI(TAG, "📡 GPS: %d sats, sentences: %lu", 
             gps_state.current_data.satellites_used,
             gps_state.stats.sentences_parsed);
    last_debug_time = now;
}
```

### Force GPS Reset
**If GPS seems stuck:**

1. Power off device
2. Wait 10 seconds
3. Power on device
4. Go outside immediately

## Summary

**Your Issue:** 0 satellites after 10 minutes = **Antenna cannot see sky**

**Most Likely Causes (in order):**
1. 🏠 **Indoor location** (90% of cases)
2. 📡 **Antenna not connected** (8% of cases)
3. 🔌 **Antenna not powered** (if active antenna) (1% of cases)
4. 💔 **Faulty antenna** (1% of cases)

**Immediate Action:**
1. ✅ **Take device outside** (open area, clear sky)
2. ✅ **Check antenna connection** (ensure fully seated)
3. ✅ **Wait 2-3 minutes** (should see satellites)

**Expected Result:**
- Within 2-3 minutes outdoors: 4-12 satellites
- Within 30 seconds: First GPS fix
- OLED shows: "GPS: 3D FIX (8sat)"

**If Still No Satellites Outdoors:**
- Check antenna connector (U.FL/SMA)
- Try different antenna (if available)
- Check antenna cable for damage
- Verify antenna power (if active antenna)

## Related Documentation
- `docs/GPS_TROUBLESHOOTING_COMPLETE.md` - Complete troubleshooting guide
- `docs/GPS_QUICK_TEST.md` - Quick GPS testing
- `docs/GPS_WIRING_CORRECTION_CRITICAL.md` - Wiring verification
- `docs/complete-wiring-diagram.md` - Complete wiring diagram
