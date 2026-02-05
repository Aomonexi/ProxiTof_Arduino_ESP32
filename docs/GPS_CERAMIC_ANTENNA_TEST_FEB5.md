# GPS Ceramic Antenna Testing Guide - February 5, 2026

## Overview
Complete guide to verify that your ceramic antenna connected to IPEX connector is working correctly with NEO-M9N GPS module.

## What We Configured

### IPEX Antenna Selection (Already Done)
**The firmware now explicitly configures NEO-M9N to use IPEX:**

```c
uint8_t ubx_cfg_ant[] = {
    0xB5, 0x62,  // UBX header
    0x06, 0x13,  // CFG-ANT
    0x04, 0x00,  // Length: 4 bytes
    0x1B, 0x00,  // flags: enable supervisor + monitoring
    0xF3, 0x9F,  // pins: INTERNAL antenna (IPEX) ← This selects IPEX!
    CK_A, CK_B   // Checksum
};
```

**What this does:**
- ✅ Tells GPS to use **internal antenna** (IPEX connector)
- ✅ Ignores SMA connector (even if present)
- ✅ Enables antenna supervisor (monitors health)
- ✅ Enables short/open circuit detection
- ✅ Reports antenna status via NMEA

## How to Test Ceramic Antenna

### Method 1: Outdoor Satellite Test (Definitive)

**This is the ONLY way to know for sure the antenna works.**

#### Step 1: Flash Firmware
```bash
idf.py flash
idf.py monitor
```

#### Step 2: Check Configuration
**Look for these log messages:**
```
I (1234) GPS: Initializing GPS on UART1...
I (1345) GPS: ✓ Configured antenna (internal/IPEX active, SCD/OCD enabled)
I (1446) GPS: ✓ Enabled all 6 GNSS systems:
I (1447) GPS:   - GPS (L1C/A): 8 channels
I (1448) GPS:   - GLONASS (L1OF): 8 channels
I (1449) GPS:   - Galileo (E1B/C): 4 channels
I (1450) GPS:   - BeiDou (B1I): 8 channels
I (1451) GPS:   - QZSS (L1C/A): 2 channels
I (1452) GPS:   - SBAS (L1C/A): 1 channel
```

#### Step 3: Go Outside
**CRITICAL: GPS cannot work indoors!**

```
Requirements:
□ Open area (parking lot, field, balcony)
□ Clear sky view (no roof, no trees directly above)
□ Antenna facing up (flat, horizontal)
□ Away from metal objects
□ Away from walls/buildings
```

#### Step 4: Wait and Watch

**Expected Timeline (Outdoors with Working Antenna):**
```
Time    Satellites  NMEA Sentence                                    Status
────────────────────────────────────────────────────────────────────────────
0s      0          $GNRMC,,V,,,,,,,,,N,V*37                         Searching...
10s     2-4        $GNGGA,123519,,,,,1,04,5.0,,,M,,M,,*xx          Acquiring...
20s     4-8        $GNGGA,123519,4807.038,N,01131.000,E,1,06,...   Calculating...
30s     8-12       $GNGGA,123519,4807.038,N,01131.000,E,1,08,...   FIX! ✓
```

**Log Messages:**
```
I (10000) GPS: 📡 GPS UART active: 289 bytes received
I (20000) GPS: 📡 GPS UART active: 289 bytes received
I (30000) GPS: ✓ First GPS fix acquired in 28 seconds
```

#### Step 5: Verify Antenna Working

**✅ Antenna Working (Success):**
```
NMEA: $GNGGA,123519,4807.038,N,01131.000,E,1,08,1.2,545.4,M,46.9,M,,*47
                                                  ↑  ↑↑
                                                  |  └─ 8 satellites!
                                                  └─ Fix quality: 1

Logs:
I (30123) GPS: ✓ First GPS fix acquired in 28 seconds
I (31000) GPS: 📡 GPS UART active: 289 bytes received
```

**❌ Antenna NOT Working (Failure):**
```
NMEA: $GNRMC,,V,,,,,,,,,N,V*37
              ↑
              Still empty after 5+ minutes outdoors

Logs:
I (300000) GPS: 📡 GPS UART active: 289 bytes received
(No "First GPS fix acquired" message)
```

### Method 2: Antenna Status Messages

**NEO-M9N can report antenna status via GNTXT sentences.**

**Our firmware now logs these automatically:**

#### ✅ Antenna OK (Working)
```
I (xxxx) GPS: ✅ Antenna Status: OK (connected and working)
```

#### ⚠️ Antenna Open (Disconnected)
```
W (xxxx) GPS: ⚠️  Antenna Status: OPEN (disconnected or not detected)
```

#### ❌ Antenna Short (Damaged)
```
E (xxxx) GPS: ❌ Antenna Status: SHORT (short circuit detected!)
```

**Note:** Not all GPS modules send these messages. If you don't see them, use Method 1 (outdoor test).

### Method 3: Physical Inspection

**Check IPEX Connection:**

```
Visual Inspection:
□ IPEX connector fully seated? (should click/snap)
□ Connector pins not bent?
□ Cable not damaged?
□ Cable not pinched?
□ Antenna not cracked?

Connection Test:
1. Power off device
2. Gently tug antenna cable
3. Should NOT come loose easily
4. If loose, reconnect firmly (should click)
5. Power on and test
```

## Ceramic Antenna Characteristics

### What is a Ceramic Antenna?

**Physical:**
- Small square/rectangular patch
- Usually white or beige ceramic material
- Flat, about 15-25mm square
- IPEX (U.FL) connector on cable

**Electrical:**
- Passive (no power needed)
- Frequency: 1575.42 MHz (GPS L1)
- Gain: 0-3 dBi (low gain)
- Polarization: RHCP (Right-Hand Circular)

### Performance Expectations

**Good Conditions (Outdoors, Clear Sky):**
- Satellites: 8-12 visible
- Time to first fix: 26-30 seconds (cold start)
- Position accuracy: 2-5 meters
- Works well for cycling applications

**Poor Conditions (Indoors, Urban Canyon):**
- Satellites: 0-4 visible (may not get fix)
- Time to first fix: Never (indoors)
- Position accuracy: N/A
- **Will NOT work indoors**

### Ceramic vs Active Antenna

| Feature | Ceramic (Passive) | Active (SMA) |
|---------|-------------------|--------------|
| Power | No power needed | Needs 3.3V |
| Gain | 0-3 dBi (low) | 15-28 dBi (high) |
| Size | Small (15-25mm) | Large (50-200mm) |
| Connector | IPEX (U.FL) | SMA |
| Indoor | No | Maybe (if strong) |
| Outdoor | Yes | Yes (better) |
| Cost | $2-5 | $10-30 |

**Your ceramic antenna is perfect for outdoor cycling!**

## Troubleshooting

### Still 0 Satellites Outdoors

**If you're outdoors with clear sky and still 0 satellites:**

#### 1. Check IPEX Connection
```
□ Connector fully seated? (should click)
□ Try disconnecting and reconnecting
□ Check for bent pins
□ Check cable for damage
```

#### 2. Check Antenna Placement
```
□ Antenna facing up? (flat, horizontal)
□ Clear sky view? (no roof, trees, buildings above)
□ Away from metal? (metal blocks GPS signals)
□ Not covered? (hand, body, case)
```

#### 3. Check for Hardware Jumper
```
Some GPS modules have physical jumper:
□ Look for solder bridge labeled "ANT1/ANT2"
□ Look for jumper labeled "IPEX/SMA"
□ Check GPS module documentation
□ May need to change jumper position
```

#### 4. Try Different Location
```
Test in multiple outdoor locations:
□ Open field (best)
□ Parking lot (good)
□ Balcony (okay)
□ Near window (marginal)

If works in one location but not another:
→ First location had obstructions
```

#### 5. Test with SMA Antenna (If Available)
```
If you have SMA antenna:
1. Disconnect IPEX antenna
2. Connect SMA antenna
3. Go outside and test
4. Check for satellites

If SMA works but IPEX doesn't:
→ IPEX antenna or connection problem
→ Check IPEX connector
→ Try different ceramic antenna
```

### Antenna Status Messages

**If you see "OPEN" status:**
```
W (xxxx) GPS: ⚠️  Antenna Status: OPEN (disconnected or not detected)

Causes:
□ IPEX connector not fully seated
□ Antenna cable damaged
□ Antenna not compatible (wrong frequency)
□ GPS module defaulting to SMA (hardware issue)

Solutions:
1. Reconnect IPEX firmly (should click)
2. Check cable for damage
3. Try different antenna
4. Check for hardware jumper
```

**If you see "SHORT" status:**
```
E (xxxx) GPS: ❌ Antenna Status: SHORT (short circuit detected!)

Causes:
□ Antenna damaged (internal short)
□ Cable damaged (wires touching)
□ Connector damaged

Solutions:
1. Disconnect antenna immediately
2. Inspect cable for damage
3. Replace antenna
4. Do NOT use damaged antenna (can damage GPS module)
```

## Expected Results

### Success Criteria

**✅ Ceramic Antenna Working:**
```
1. Configuration:
   I (1345) GPS: ✓ Configured antenna (internal/IPEX active, SCD/OCD enabled)

2. Outdoors (2-3 minutes):
   I (30123) GPS: ✓ First GPS fix acquired in 28 seconds

3. NMEA Sentences:
   $GNGGA,123519,4807.038,N,01131.000,E,1,08,1.2,545.4,M,46.9,M,,*47
                                         ↑  ↑↑
                                         |  └─ 8 satellites
                                         └─ Fix: 1

4. Antenna Status (if reported):
   I (xxxx) GPS: ✅ Antenna Status: OK (connected and working)
```

### Failure Indicators

**❌ Ceramic Antenna NOT Working:**
```
1. Outdoors 5+ minutes:
   Still 0 satellites

2. NMEA Sentences:
   $GNRMC,,V,,,,,,,,,N,V*37
           ↑
           Empty (no position, no satellites)

3. Antenna Status (if reported):
   W (xxxx) GPS: ⚠️  Antenna Status: OPEN (disconnected or not detected)

4. No fix message:
   (Never see "First GPS fix acquired")
```

## Summary

### To Verify Ceramic Antenna Works:

**1. Flash firmware** (already built)
```bash
idf.py flash
idf.py monitor
```

**2. Check configuration logs**
```
I (1345) GPS: ✓ Configured antenna (internal/IPEX active, SCD/OCD enabled)
```

**3. Go outside** (critical!)
```
- Open area, clear sky
- Antenna facing up
- Wait 2-3 minutes
```

**4. Check for satellites**
```
Success: 8-12 satellites, GPS fix acquired
Failure: 0 satellites after 5+ minutes
```

**5. If failure, check:**
```
□ IPEX connector fully seated?
□ Antenna cable damaged?
□ Actually outdoors with clear sky?
□ Antenna facing up?
□ Hardware jumper (if present)?
```

### The firmware is ready. Now you need to test outdoors! 🌍📡

**Remember:** GPS **CANNOT** work indoors. You **MUST** go outside to test.

## Related Documentation
- `docs/GPS_ANTENNA_CONFIGURATION_FEB5.md` - Antenna configuration details
- `docs/GPS_NO_SATELLITES_TROUBLESHOOTING_FEB5.md` - Troubleshooting guide
- `docs/GPS_ANTENNA_FIX_SUMMARY_FEB5.md` - Quick summary
- `docs/GPS_COMPLETE_CONFIGURATION_FEB5.md` - Complete GPS configuration
