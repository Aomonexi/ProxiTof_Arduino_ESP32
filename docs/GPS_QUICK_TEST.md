# GPS Quick Test Guide - Neo-M9N

**Date**: February 3, 2026  
**Purpose**: Simple step-by-step test to verify GPS is working  
**Time Required**: 10 minutes

---

## What I Just Added

I added **optional GPS configuration** to your code that:
1. ✅ Enables all GNSS systems (GPS + GLONASS + Galileo + BeiDou)
2. ✅ Sets bike dynamic model (optimized for cycling)
3. ✅ Saves configuration to GPS flash memory (persists forever)

**Important**: This configuration runs **once** and is saved to the GPS module's flash memory. After the first boot, the GPS will remember these settings even if you power it off!

---

## Quick Test Procedure

### Step 1: Build and Flash (2 minutes)

```bash
# Build with new GPS configuration
idf.py build flash monitor
```

**Expected Output:**
```
I (1234) GPS: Initializing GPS on UART1 (TX=38, RX=47, baud=38400)
I (1235) GPS: Configuring GPS for optimal performance...
I (1336) GPS: ✓ Enabled all GNSS (GPS+GLONASS+Galileo+BeiDou)
I (1437) GPS: ✓ Set dynamic model to Bike
I (1938) GPS: ✓ Saved configuration to GPS flash memory
I (1939) GPS: GPS configuration complete (runs once, saved to flash)
I (1940) GPS: ✓ GPS initialized successfully
I (1941) GPS: 💡 Waiting for GPS fix (cold start: ~26s, hot start: ~1s)
I (1942) GPS: GPS task started, waiting for NMEA sentences...
```

### Step 2: Check UART Communication (30 seconds)

**Wait 5 seconds, then look for:**
```
I (5000) GPS: 📡 GPS UART active: 82 bytes received (total: 410)
I (5000) GPS: 0x3fceb7c0   24 47 50 47 47 41 2c 31  32 33 35 31 39 2c ...
                            $  G  P  G  G  A  ,  1   2  3  5  1  9  ,
```

**What This Means:**
- ✅ **"GPS UART active"** = GPS is powered and communicating
- ✅ **"$GPGGA"** visible in hex dump = GPS is sending NMEA sentences
- ✅ **Bytes received increasing** = Data flowing correctly

**If You Don't See This:**
- ❌ Check power (3.3V or 5V to GPS VCC)
- ❌ Check wiring (TX→RX crossover)
- ❌ Check GPS power LED is lit

### Step 3: Go Outdoors (REQUIRED!)

**GPS WILL NOT WORK INDOORS!**

Take your device:
- ✅ **Outdoors** (parking lot, balcony, rooftop, yard)
- ✅ **Clear sky view** (no buildings, trees, or metal above)
- ✅ **Antenna facing up** (toward sky)
- ✅ **Keep stationary** (don't move around)

### Step 4: Wait for Satellite Fix (5-10 minutes)

**Watch Serial Monitor:**

**After 30 seconds:**
```
I (30000) GPS: 📡 Satellites in view: 0
// Still searching...
```

**After 1 minute:**
```
I (60000) GPS: 📡 Satellites in view: 3
// Getting closer...
```

**After 2 minutes:**
```
I (120000) GPS: 📡 Satellites in view: 6
// Almost there...
```

**After 3-5 minutes:**
```
I (180000) GPS: 📡 Satellites in view: 8
I (180001) GPS: ✓ First GPS fix acquired in 180 seconds
```

**Success! GPS is working!** 🎉

### Step 5: Check GPS Mode on OLED (Optional)

1. Press MODE button to cycle to GPS mode (Mode 4)
2. Display should show:
   ```
   GPS: 3D FIX (8sat)
   HDOP: 1.2
   Lat: 1.234567N
   Lon: 103.123456E
   Alt: 45.2m
   Spd: 0.0km/h
   ```

---

## Troubleshooting

### Problem: No "GPS UART active" message

**Cause**: GPS not powered or wiring issue

**Fix:**
1. Check GPS power LED is lit
2. Verify wiring:
   ```
   ESP32 GPIO 38 (TX) → GPS RX
   ESP32 GPIO 47 (RX) ← GPS TX
   ESP32 3.3V or 5V   → GPS VCC
   ESP32 GND          → GPS GND
   ```
3. Try different power source (5V instead of 3.3V or vice versa)

### Problem: "GPS UART active" but no satellites

**Cause**: Not outdoors or obstructed sky view

**Fix:**
1. **Go outdoors!** (most common issue)
2. Check antenna is facing up
3. Move away from buildings and trees
4. Wait longer (first fix can take 5-10 minutes)

### Problem: Satellites found but no fix

**Cause**: Not enough satellites (need 4+ for 3D fix)

**Fix:**
1. Wait longer (satellites are being acquired)
2. Improve sky view (move to more open area)
3. Check HDOP value (should be < 5.0)

### Problem: Fix acquired but then lost

**Cause**: Moved indoors or obstructed

**Fix:**
1. Stay outdoors with clear sky view
2. Keep device stationary
3. Check antenna connection

---

## What the Configuration Does

### 1. Enable All GNSS Systems

**Before** (factory default):
- GPS only (USA satellites)
- ~8-12 satellites visible
- Fix time: 30-60 seconds

**After** (with configuration):
- GPS + GLONASS + Galileo + BeiDou
- ~20-30 satellites visible
- Fix time: 15-30 seconds
- Better accuracy and reliability

### 2. Bike Dynamic Model

**Before** (portable model):
- Generic motion model
- Works for walking, driving, etc.

**After** (bike model):
- Optimized for bicycle speeds (0-30 km/h)
- Better tracking during turns
- Improved accuracy for cycling

### 3. Save to Flash

**Why This Matters:**
- Configuration saved to GPS module's internal flash
- Persists across power cycles
- Only needs to run once
- GPS remembers settings forever

---

## Expected Timeline

**First Boot (Cold Start):**
```
0:00 - Power on, GPS initializes
0:01 - Configuration sent to GPS
0:02 - GPS starts searching for satellites
0:30 - First satellites detected (0-3 visible)
1:00 - More satellites (3-6 visible)
2:00 - Even more satellites (6-8 visible)
3:00 - Fix acquired! (8+ satellites, HDOP < 2.0)
```

**Subsequent Boots (Hot Start):**
```
0:00 - Power on, GPS initializes
0:01 - GPS loads saved configuration
0:02 - GPS starts searching (already has almanac)
0:05 - Fix acquired! (much faster with almanac)
```

**After 4 Hours Off (Warm Start):**
```
0:00 - Power on
0:01 - GPS loads configuration
0:02 - GPS searches (almanac still valid)
0:30 - Fix acquired (faster than cold start)
```

---

## Success Criteria

✅ **GPS is working if you see:**
1. "GPS UART active" messages every 5 seconds
2. Hex dump shows "$GP" or "$GN" NMEA sentences
3. Satellite count increasing over time
4. "First GPS fix acquired" message
5. GPS mode shows valid lat/lon coordinates

❌ **GPS is NOT working if:**
1. No "GPS UART active" messages (power/wiring issue)
2. No satellites after 10 minutes outdoors (antenna/environment issue)
3. Garbled data in hex dump (baud rate issue)

---

## Next Steps After GPS Works

1. **Test while riding**:
   - GPS should maintain fix while cycling
   - Speed and heading should update
   - Location should track your movement

2. **Check data logging**:
   - GPS coordinates saved to CSV files
   - Verify lat/lon in log files
   - Check GPS timestamp accuracy

3. **Start data collection**:
   - Follow Data Collection Field Guide
   - GPS will geo-tag all near-miss events
   - Useful for mapping dangerous locations

---

## FAQ

**Q: Do I need to configure GPS every time I power on?**  
A: No! Configuration is saved to GPS flash memory. It only runs once.

**Q: Can I use GPS indoors?**  
A: No. GPS needs clear sky view to see satellites. Won't work indoors (except maybe near windows).

**Q: How long does first fix take?**  
A: 3-10 minutes for cold start (first time). 1-5 seconds for hot start (subsequent times).

**Q: What if GPS never gets fix?**  
A: Most likely environmental issue. Try different outdoor location with better sky view.

**Q: Can I test GPS without going outdoors?**  
A: No. GPS fundamentally requires line-of-sight to satellites in space. Indoor testing won't work.

**Q: What's the difference between 2D and 3D fix?**  
A: 2D fix (3 satellites) = lat/lon only. 3D fix (4+ satellites) = lat/lon + altitude. Both work for cycling.

**Q: Why does GPS take so long the first time?**  
A: GPS needs to download almanac data (satellite positions) from satellites. This takes 5-10 minutes. After that, it's much faster.

---

**Good luck with GPS testing! Remember: OUTDOORS with CLEAR SKY VIEW is essential!** 🛰️

**Document Version**: 1.0  
**Date**: February 3, 2026  
**Status**: Ready for testing

