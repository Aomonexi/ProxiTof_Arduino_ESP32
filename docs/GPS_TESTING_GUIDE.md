# GPS Testing Guide - How to Verify GPS is Working

**Date**: January 27, 2026  
**GPS Module**: Neo-M9N  
**UART**: UART_NUM_1 (TX=GPIO44, RX=GPIO43, Baud=9600)

---

## Quick Test Methods

### Method 1: Check Console Output at Startup

When the device boots, look for GPS initialization messages:

**Success:**
```
I (12345) MAIN: Initializing GPS module (Neo-M9N)...
I (12345) MAIN: Wiring: TX=GPIO44 (ESP32→GPS RX), RX=GPIO43 (ESP32←GPS TX)
I (12345) MAIN: UART crossover wiring is CORRECT (TX→RX, RX→TX)
I (12345) MAIN: ✓ GPS initialized (waiting for satellite fix)
I (12345) MAIN: 💡 GPS fix may take 1-5 minutes outdoors
I (12345) MAIN: 💡 Check GPS mode (Mode 4) to see status
```

**Failure:**
```
W (12345) MAIN: ❌ GPS initialization failed: ESP_ERR_INVALID_ARG
W (12345) MAIN: 💡 Check wiring: TX=44, RX=43, VCC=5V, GND=GND
W (12345) MAIN: 💡 System will continue without GPS
```

---

### Method 2: Check GPS Mode (Mode 4)

1. **Navigate to GPS mode**
   - Press MODE button (GPIO 20) until you reach Mode 4
   - Display shows "GPS" in yellow zone

2. **Check GPS status indicators**

**GPS Working - Searching for Fix:**
```
┌────────────────────────────────────────┐
│ GPS                (INVERTED)          │
│ SEARCHING                              │
├────────────────────────────────────────┤
│                                        │
│ Lat: ---.------                        │
│ Lon: ---.------                        │
│ Alt: ---- m                            │
│ Spd: --- km/h                          │
│ Sat: 0  HDOP: --.-                     │
│                                        │
└────────────────────────────────────────┘
```

**GPS Working - Has Fix:**
```
┌────────────────────────────────────────┐
│ GPS                (INVERTED)          │
│ 3D FIX                                 │
├────────────────────────────────────────┤
│                                        │
│ Lat: 37.774929                         │
│ Lon: -122.419418                       │
│ Alt: 15 m                              │
│ Spd: 0 km/h                            │
│ Sat: 8  HDOP: 1.2                      │
│                                        │
└────────────────────────────────────────┘
```

**GPS NOT Working:**
```
┌────────────────────────────────────────┐
│ GPS                (INVERTED)          │
│ NO DATA                                │
├────────────────────────────────────────┤
│                                        │
│ GPS module not                         │
│ responding                             │
│                                        │
│ Check wiring:                          │
│ TX=44 RX=43                            │
│ VCC=5V GND=GND                         │
└────────────────────────────────────────┘
```

---

### Method 3: Check Dashboard (Mode 0)

Look at the status line (Page 1) in Dashboard mode:

**GPS Status Indicators:**
- `3D` = 3D fix (4+ satellites, lat/lon/alt)
- `2D` = 2D fix (3 satellites, lat/lon only)
- `FIX` = Some fix (1-2 satellites)
- `---` = No fix or GPS not working

**Example:**
```
Page 1: "LOG 3D 450mm"  ← GPS has 3D fix
Page 1: "LOG --- 450mm" ← GPS not working
```

---

### Method 4: Check Status Mode (Mode 1)

Navigate to Status mode (Mode 1) and look at Page 5:

**GPS Working:**
```
Page 5: "GPS:FIX SAT:8"  ← 8 satellites locked
Page 5: "GPS:SEARCH"     ← Searching for satellites
```

**GPS NOT Working:**
```
Page 5: "GPS:NO DATA"    ← Module not responding
```

---

## GPS Reconnect Feature

If GPS fails to initialize or stops responding, you can try reconnecting:

1. **Navigate to GPS mode (Mode 4)**
2. **Press ACTION button (GPIO 3)**
3. **Wait for result (2-3 seconds)**

**Success:**
```
┌────────────────────────────────────────┐
│ GPS Reconnect      (INVERTED)          │
│                                        │
├────────────────────────────────────────┤
│                                        │
│ SUCCESS!                               │
│ GPS ready                              │
│                                        │
│                                        │
│                                        │
│                                        │
└────────────────────────────────────────┘
```

**Failure:**
```
┌────────────────────────────────────────┐
│ GPS Reconnect      (INVERTED)          │
│                                        │
├────────────────────────────────────────┤
│                                        │
│ FAILED                                 │
│ Check wiring                           │
│                                        │
│ TX=44 RX=43                            │
│ VCC=5V GND=GND                         │
│                                        │
└────────────────────────────────────────┘
```

---

## Troubleshooting GPS Issues

### Issue 1: GPS Shows "NO DATA" or "SEARCHING" Forever

**Possible Causes:**
1. GPS module not powered (check 5V and GND)
2. Wrong UART pins (should be TX=44, RX=43)
3. Swapped TX/RX (remember: ESP32 TX → GPS RX, ESP32 RX ← GPS TX)
4. Indoors (GPS needs clear sky view)
5. Cold start (first fix can take 1-5 minutes)

**Solutions:**
1. Check wiring with multimeter (5V on VCC pin)
2. Verify pin connections: GPIO44 → GPS RX, GPIO43 → GPS TX
3. Move device outdoors or near window
4. Wait 5 minutes for cold start
5. Try GPS reconnect (ACTION button in GPS mode)

---

### Issue 2: GPS Shows "FIX" but No Coordinates

**Possible Causes:**
1. Weak signal (only 1-2 satellites)
2. Obstructed view (buildings, trees)
3. GPS data not being parsed correctly

**Solutions:**
1. Move to open area with clear sky view
2. Wait for more satellites (need 4+ for 3D fix)
3. Check console for GPS parsing errors

---

### Issue 3: GPS Worked Before, Now Shows "NO DATA"

**Possible Causes:**
1. Loose connection
2. GPS module crashed
3. UART buffer overflow

**Solutions:**
1. Check all wire connections
2. Try GPS reconnect (ACTION button in GPS mode)
3. Restart device if reconnect fails

---

## GPS Wiring Verification

**Correct Wiring (UART Crossover):**
```
ESP32-S3          Neo-M9N GPS
--------          -----------
GPIO44 (TX) ----> RX (GPS receives)
GPIO43 (RX) <---- TX (GPS transmits)
5V          ----> VCC
GND         ----> GND
```

**Common Mistakes:**
- ❌ ESP32 TX → GPS TX (wrong, both transmit)
- ❌ ESP32 RX → GPS RX (wrong, both receive)
- ❌ Using 3.3V instead of 5V (GPS needs 5V)
- ❌ Wrong GPIO pins (must be 44 and 43)

---

## GPS Fix Time Expectations

**Cold Start (First Power-On):**
- Time: 26-30 seconds (typical)
- Can take up to 5 minutes in poor conditions
- Needs clear sky view

**Warm Start (Recent Fix):**
- Time: 1-2 seconds
- GPS remembers satellite positions
- Works even with partial sky view

**Hot Start (Continuous Operation):**
- Time: < 1 second
- GPS already tracking satellites
- Best performance

---

## GPS Data Quality Indicators

**HDOP (Horizontal Dilution of Precision):**
- < 1.0 = Excellent (ideal)
- 1.0-2.0 = Good (normal)
- 2.0-5.0 = Moderate (acceptable)
- 5.0-10.0 = Fair (poor geometry)
- > 10.0 = Poor (unreliable)

**Satellite Count:**
- 0-2 = No fix
- 3 = 2D fix (lat/lon only)
- 4+ = 3D fix (lat/lon/alt)
- 8+ = Excellent (best accuracy)

**Fix Type:**
- `NO FIX` = No satellites
- `2D FIX` = 3 satellites (no altitude)
- `3D FIX` = 4+ satellites (full position)

---

## Testing Checklist

- [ ] GPS initialization message appears at startup
- [ ] No error messages about GPS wiring
- [ ] GPS mode (Mode 4) shows "SEARCHING" or "FIX"
- [ ] Dashboard shows GPS status (3D, 2D, FIX, or ---)
- [ ] Status mode shows satellite count
- [ ] GPS reconnect button works (ACTION in GPS mode)
- [ ] Coordinates update when moving (if outdoors)
- [ ] Satellite count increases over time (0 → 4+)
- [ ] HDOP value is reasonable (< 5.0)
- [ ] Fix type changes from SEARCHING → 2D → 3D

---

## Console Commands for GPS Testing

If you have USB Serial JTAG connected (RIGHT port), you can send commands:

**Check GPS Status:**
```
GPS_STATUS
```

**Force GPS Reconnect:**
```
GPS_RECONNECT
```

**Show GPS Raw Data:**
```
GPS_RAW
```

*(Note: These commands may need to be implemented in usb_interface.c)*

---

## Expected GPS Performance

**Indoors:**
- Fix: Usually NO FIX
- Satellites: 0-2
- Time: Never or > 10 minutes
- Accuracy: N/A

**Near Window:**
- Fix: 2D or 3D (depends on window direction)
- Satellites: 3-6
- Time: 1-5 minutes
- Accuracy: 5-15 meters

**Outdoors (Clear Sky):**
- Fix: 3D FIX
- Satellites: 6-12
- Time: 30 seconds - 2 minutes
- Accuracy: 2-5 meters

**Outdoors (Obstructed):**
- Fix: 2D or 3D
- Satellites: 4-8
- Time: 1-3 minutes
- Accuracy: 5-10 meters

---

## Summary

**GPS is working if:**
✅ Initialization succeeds at startup  
✅ GPS mode shows "SEARCHING" or "FIX"  
✅ Dashboard shows GPS status (not "---")  
✅ Satellite count increases over time  
✅ Coordinates appear after getting fix  

**GPS is NOT working if:**
❌ Initialization fails at startup  
❌ GPS mode shows "NO DATA"  
❌ Dashboard always shows "---"  
❌ Satellite count stays at 0  
❌ No coordinates after 5+ minutes outdoors  

**Next Steps:**
1. Check console output at startup
2. Navigate to GPS mode (Mode 4)
3. Wait 2-5 minutes outdoors
4. Check if satellite count increases
5. Try GPS reconnect if needed
6. Verify wiring if still not working

---

**Document Version:** 1.0  
**Last Updated:** January 27, 2026  
**Status:** ✅ Complete GPS testing guide
