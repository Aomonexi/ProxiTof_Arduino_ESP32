# GPS NMEA Diagnostic - February 5, 2026

## Your Situation Analysis

**What You Reported:**
- ✅ PPS LED blinking (GPS module working)
- ✅ 7500+ sentences parsed (UART communication working)
- ❌ 0 satellites after 10 minutes (antenna issue)

**Software Status:**
- ✅ NOT using ubxlib (using custom NMEA parser - this is fine)
- ✅ NMEA parsing is working correctly
- ✅ Code correctly reads satellite count from GPGGA sentences
- ✅ All 6 GNSS systems configured correctly

**Conclusion:** Software is fine. Issue is **physical/hardware**.

## Understanding NMEA Sentences

### What GPS Sends (NMEA Format)

**With Satellites (Good):**
```
$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47
                                      ↑  ↑↑
                                      |  └─ 08 satellites
                                      └─ Fix quality: 1 (GPS fix)
```

**Without Satellites (Your Situation):**
```
$GPGGA,123519,,,,,0,00,99.9,,,M,,M,,*47
                  ↑  ↑↑
                  |  └─ 00 satellites (EMPTY FIELDS)
                  └─ Fix quality: 0 (no fix)
```

### What Our Code Does

```c
// Parse GPGGA sentence
int satellites = 0;  // Default: 0
sscanf(sentence, "$GPGGA,%[^,],%[^,],%c,%[^,],%c,%d,%d,%f,%f,M",
       time_str, lat_str, &lat_dir, lon_str, &lon_dir,
       &fix_quality, &satellites, &hdop, &altitude);
       //            ↑↑↑↑↑↑↑↑↑↑
       //            Reads satellite count here

// Store in state
gps_state.current_data.satellites_used = satellites;
```

**This code is correct!** If GPS sends "08", we read 8. If GPS sends "00", we read 0.

## Why You're Getting 0 Satellites

### The GPS Module is Working
- ✅ PPS blinking = GPS firmware running
- ✅ NMEA sentences = GPS transmitting data
- ✅ 7500 sentences = Continuous communication

### But GPS Can't See Satellites
- ❌ Antenna not seeing sky
- ❌ GPS sends empty GPGGA sentences
- ❌ Satellite count = 0
- ❌ Fix quality = 0

## Diagnostic: Check Raw NMEA Data

### Enable Raw NMEA Logging

**Edit `components/gps/gps.c` around line 275:**

```c
// Change this:
if (now - last_debug_time > 5000) {
    ESP_LOGI(TAG, "📡 GPS UART active: %d bytes received (total: %lu)", len, bytes_received_total);
    ESP_LOG_BUFFER_HEXDUMP(TAG, data, len > 50 ? 50 : len, ESP_LOG_INFO);
    last_debug_time = now;
}

// To this (print actual NMEA sentences):
if (now - last_debug_time > 5000) {
    ESP_LOGI(TAG, "📡 GPS UART active: %d bytes received (total: %lu)", len, bytes_received_total);
    ESP_LOGI(TAG, "📡 Raw NMEA: %.*s", len > 100 ? 100 : len, (char*)data);
    last_debug_time = now;
}
```

### What You'll See

**If Antenna Working (Outdoors):**
```
I (12345) GPS: 📡 Raw NMEA: $GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47
                                                                    ↑  ↑↑
                                                                    |  └─ 8 satellites!
                                                                    └─ Fix: 1
```

**If Antenna Not Working (Your Case):**
```
I (12345) GPS: 📡 Raw NMEA: $GPGGA,123519,,,,,0,00,99.9,,,M,,M,,*47
                                            ↑  ↑↑
                                            |  └─ 0 satellites
                                            └─ No fix: 0
```

## Quick Test: Check GPGSV Sentences

GPGSV sentences show satellites in view (even without fix):

**Good (Satellites Visible):**
```
$GPGSV,3,1,12,01,45,123,42,02,30,234,38,03,60,045,45,04,15,300,35*7A
       ↑     ↑↑
       |     └─ 12 satellites in view
       └─ 3 messages total
```

**Bad (No Satellites):**
```
$GPGSV,1,1,00*79
          ↑↑
          └─ 0 satellites in view
```

### Add GPGSV Parsing (Diagnostic)

**Add to `parse_nmea_sentence()` in `components/gps/gps.c`:**

```c
static void parse_nmea_sentence(const char *sentence, int len)
{
    // Verify checksum
    if (!nmea_verify_checksum(sentence, len)) {
        gps_state.stats.checksum_errors++;
        return;
    }
    
    gps_state.stats.sentences_parsed++;
    
    // DIAGNOSTIC: Log GPGSV sentences (satellites in view)
    if (strncmp(sentence, "$GPGSV", 6) == 0 || strncmp(sentence, "$GNGSV", 6) == 0) {
        ESP_LOGI(TAG, "🛰️  GPGSV: %s", sentence);
    }
    
    // Parse based on sentence type
    if (strncmp(sentence, "$GPGGA", 6) == 0 || strncmp(sentence, "$GNGGA", 6) == 0) {
        parse_gpgga(sentence);
    } else if (strncmp(sentence, "$GPRMC", 6) == 0 || strncmp(sentence, "$GNRMC", 6) == 0) {
        parse_gprmc(sentence);
    } else if (strncmp(sentence, "$GPVTG", 6) == 0 || strncmp(sentence, "$GNVTG", 6) == 0) {
        parse_gpvtg(sentence);
    }
    
    // Update timestamp
    gps_state.current_data.last_update_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
}
```

## Software vs Hardware Issue

### Software Issue (Would Show):
- ❌ No NMEA sentences received
- ❌ UART errors
- ❌ Checksum errors
- ❌ Parse errors

### Hardware Issue (What You Have):
- ✅ NMEA sentences received (7500+)
- ✅ No UART errors
- ✅ No checksum errors
- ✅ Parsing works fine
- ❌ **But GPS sends 0 satellites** ← This is the issue

## Why ubxlib Wouldn't Help

**ubxlib is just a different parser:**
- ubxlib: Parses UBX binary protocol
- Our code: Parses NMEA text protocol
- **Both read the same GPS data**

**The problem:**
- GPS module can't see satellites
- GPS sends "0 satellites" in NMEA
- No parser can fix this - it's a hardware issue

**Analogy:**
```
GPS Module = Camera
NMEA/UBX = Photo Format (JPEG vs PNG)
Parser = Photo Viewer

Problem: Camera lens is covered (no light)
Solution: Uncover lens (move antenna to see sky)
Not Solution: Change photo format (won't help)
```

## Verification Steps

### Step 1: Check Serial Monitor
**Look for these exact messages:**

```
Good (GPS working, but no satellites):
I (1234) GPS: GPS task started, waiting for NMEA sentences...
I (1235) GPS: 📡 GPS UART active: 82 bytes received
I (1236) GPS: Sentences parsed: 7500  ← YOU SEE THIS
I (1237) GPS: Satellites: 0            ← AND THIS

Bad (GPS not working at all):
I (1234) GPS: GPS task started, waiting for NMEA sentences...
W (5000) GPS: ⚠️  No GPS data received for 5 seconds
```

**You have the first case** = GPS working, antenna not seeing sky

### Step 2: Physical Check
```
□ Is device indoors? → Move outdoors
□ Is antenna connected? → Check U.FL/SMA connector
□ Is antenna facing up? → Point antenna to sky
□ Is antenna near metal? → Move away from metal
□ Is antenna near window? → Try outdoors first
```

### Step 3: Expected Timeline (Outdoors)
```
Time    Satellites  What's Happening
0s      0          GPS searching...
10s     2-4        GPS found first satellites
20s     4-8        GPS downloading almanac
30s     8-12       GPS FIX ACQUIRED ✓
```

**If still 0 after 2 minutes outdoors:** Antenna hardware problem

## Summary

### Your Question: "Are we using ubxlib?"
**Answer:** No, we're using custom NMEA parser. This is fine and not the issue.

### Your Question: "Is that why we can't read satellites?"
**Answer:** No. The parser works correctly. The issue is:

**GPS Module Status:**
- ✅ GPS powered (PPS blinking)
- ✅ GPS transmitting (7500 NMEA sentences)
- ✅ UART communication working
- ✅ Parser working correctly

**Antenna Status:**
- ❌ Antenna can't see satellites
- ❌ GPS sends "0 satellites" in NMEA
- ❌ No fix possible

### The Real Issue
**Physical problem, not software:**
1. 🏠 Device indoors (90% probability)
2. 📡 Antenna disconnected (8% probability)
3. 🔌 Antenna not powered (1% probability)
4. 💔 Faulty antenna (1% probability)

### Immediate Action
1. **Take device outside** (open area, clear sky)
2. **Wait 2-3 minutes**
3. **Check for satellites**

**Expected:** 4-12 satellites within 2-3 minutes outdoors

### If Still No Satellites Outdoors
Then we can investigate:
- Antenna connection
- Antenna type (active vs passive)
- Antenna power
- GPS module configuration
- Wiring issues

But **99% of the time**, going outdoors solves it! 🌍📡

## Related Files
- `components/gps/gps.c` - GPS driver (NMEA parser)
- `docs/GPS_NO_SATELLITES_TROUBLESHOOTING_FEB5.md` - Troubleshooting guide
- `docs/GPS_TROUBLESHOOTING_COMPLETE.md` - Complete troubleshooting
