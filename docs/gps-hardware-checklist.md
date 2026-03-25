# GPS Hardware Checklist - NEO-M9N
**Print this page and check off each item**

---

## Problem
- [ ] GPS receives NMEA sentences but 0 satellites
- [ ] Tested outdoors for 5+ minutes
- [ ] UART communication working (7500+ sentences)

---

## Quick Fixes (Try These First!)

### 1. IPEX Antenna Connection (MOST LIKELY FIX)
- [ ] Power OFF system
- [ ] Locate IPEX connector on GPS module (tiny white/black connector)
- [ ] Disconnect antenna cable (pull straight up)
- [ ] Inspect connector pins (both cable and module)
- [ ] Reconnect FIRMLY (press until you hear/feel a CLICK)
- [ ] Pull test (should require firm force to disconnect)
- [ ] Power ON and test outdoors for 10 minutes

**If antenna pops off easily → not fully connected!**

### 2. Hardware Antenna Jumper
- [ ] Power OFF system
- [ ] Inspect GPS module PCB for solder jumper labeled "ANT" or "ANTENNA"
- [ ] If found, verify jumper is set to IPEX position (not SMA)
- [ ] If 0Ω resistor, ensure it's on IPEX pads (not SMA pads)
- [ ] Power ON and test outdoors for 10 minutes

---

## Advanced Checks (If Quick Fixes Don't Work)

### 3. D_SEL Pin (UART vs SPI Mode)
- [ ] Power OFF system
- [ ] Locate D_SEL pin (pin 19) on GPS module
- [ ] Check if D_SEL is pulled LOW (wrong!) or HIGH/floating (correct)
- [ ] If pulled LOW, remove pull-down resistor or solder bridge
- [ ] Power ON and test outdoors for 10 minutes

**D_SEL must be HIGH or floating for UART mode!**

### 4. SAFEBOOT_N Pin (Normal vs Safeboot Mode)
- [ ] Power OFF system
- [ ] Locate SAFEBOOT_N pin (pin 22) on GPS module
- [ ] Check if SAFEBOOT_N is pulled LOW (wrong!) or HIGH/floating (correct)
- [ ] If pulled LOW, remove pull-down resistor or solder bridge
- [ ] Power ON and test outdoors for 10 minutes

**SAFEBOOT_N must be HIGH or floating for normal operation!**

### 5. Power Supply
- [ ] Measure VCC with multimeter (should be 3.3V ± 0.1V)
- [ ] If 5V → STOP! Module may be damaged!
- [ ] If <3.0V → insufficient power supply
- [ ] Verify GND connection is solid

### 6. Antenna Type
- [ ] Verify antenna is passive ceramic patch (for IPEX)
- [ ] Verify antenna frequency is 1575.42 MHz (GPS L1 band)
- [ ] Check antenna for physical damage (cracks, broken traces)
- [ ] Ensure antenna is positioned with patch facing UP (not sideways)

### 7. Testing Location
- [ ] Testing in open field (no buildings, trees, or obstructions)
- [ ] Clear view of sky (not under roof, balcony, or overhang)
- [ ] Away from metal structures (cars, fences, metal roofs)
- [ ] Waited 10+ minutes (cold start takes time)

**GPS CANNOT work indoors!**

---

## Expected Results

### Before Fix (Current State)
```
Serial Monitor:
📡 GPS UART active: 128 bytes received
Satellites: 0
Fix: None
NMEA: $GNRMC,,V,,,,,,,,,N,V*37
      ^^^^^^ Empty fields = no data
```

### After Fix (Working)
```
Serial Monitor:
📡 GPS UART active: 256 bytes received
Satellites: 8
Fix: GPS (3D)
NMEA: $GNRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A
      ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
      Full data = working!
```

---

## Troubleshooting Decision Tree

```
Start: 0 satellites, UART working
│
├─ IPEX connector fully seated?
│  ├─ NO → Reconnect firmly (CLICK!) ← TRY THIS FIRST!
│  └─ YES → Continue
│
├─ Antenna jumper present?
│  ├─ YES → Set to IPEX position
│  └─ NO → Continue
│
├─ Testing outdoors?
│  ├─ NO → Go outside (clear sky view)
│  └─ YES → Continue
│
├─ Waited 10+ minutes?
│  ├─ NO → Wait longer (cold start)
│  └─ YES → Continue
│
├─ Try SMA antenna (if available)
│  ├─ Works → IPEX antenna faulty
│  └─ Doesn't work → Continue
│
└─ Check D_SEL and SAFEBOOT_N pins
   └─ Still no fix → GPS module may be defective
```

---

## Photos to Take (If Still Not Working)

- [ ] GPS module top view (show all connectors)
- [ ] IPEX antenna connection (close-up)
- [ ] GPS module bottom view (show jumpers/solder bridges)
- [ ] Testing location (show surroundings)

---

## Information to Provide

- [ ] GPS module manufacturer and model number
- [ ] Antenna type (ceramic patch, active, passive)
- [ ] Testing location description
- [ ] Wait time (how long tested)
- [ ] Serial monitor output (copy/paste NMEA sentences)
- [ ] Multimeter readings (VCC, D_SEL, SAFEBOOT_N)

---

## Software Verification (Should Already Be Correct)

- [ ] Baud rate: 38400 (NEO-M9N default)
- [ ] Pins: GPIO 38 (TX), GPIO 47 (RX)
- [ ] GNSS systems: All 6 enabled (GPS, GLONASS, Galileo, BeiDou, QZSS, SBAS)
- [ ] Antenna config: CFG-ANT sent to select IPEX
- [ ] Dynamic model: Configurable (Bike by default)

**Software is 100% correct - no changes needed!**

---

## Success Criteria

- [ ] Satellite count: 4-12 satellites visible
- [ ] GPS fix: 3D fix acquired
- [ ] NMEA sentences: Full data (not empty fields)
- [ ] Position: Latitude/longitude displayed
- [ ] Time: UTC time from satellites

---

## Estimated Fix Time

- **IPEX antenna reconnection:** 2 minutes
- **Hardware jumper adjustment:** 3 minutes
- **D_SEL/SAFEBOOT_N pin check:** 5 minutes
- **Total troubleshooting:** 10-15 minutes

---

## Most Likely Cause (80% Probability)

**IPEX antenna not fully connected!**

The connector is tiny and requires firm pressure to snap in. If it pops off with light pressure, it's not fully connected.

**Fix:** Disconnect and reconnect firmly until you hear/feel a click.

---

## Documentation References

- **Quick Fix Guide:** [GPS_QUICK_HARDWARE_FIX.md](GPS_QUICK_HARDWARE_FIX.md)
- **Detailed Troubleshooting:** [GPS_HARDWARE_TROUBLESHOOTING_FEB5.md](GPS_HARDWARE_TROUBLESHOOTING_FEB5.md)
- **Software Configuration:** [GPS_COMPLETE_CONFIGURATION_FEB5.md](GPS_COMPLETE_CONFIGURATION_FEB5.md)
- **Issue Summary:** [GPS_ISSUE_SUMMARY_FEB5.md](GPS_ISSUE_SUMMARY_FEB5.md)

---

**Remember:** Your software is working perfectly. Focus on hardware!

**Good luck!** 🛰️
