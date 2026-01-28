# GPS UART Troubleshooting Guide

**Date**: January 28, 2026  
**Module**: Neo-M9N GPS  
**Issue**: Finding correct UART port and baud rate

---

## Problem Summary

The GPS module is physically connected but we're receiving wrong data on the UART:
- **UART1** was receiving console output instead of GPS data
- Hex dump showed: `54 6f 46 3a 20 20 20 33 38` = "ToF:   38"
- This means UART1 is connected to console or wrong source

---

## Current Configuration

**File**: `main/main.c` (lines 39-43)

```c
#define GPS_UART_NUM UART_NUM_2     // Currently testing UART2
#define GPS_TX_PIN GPIO_NUM_44      // ESP32 TX → GPS RX
#define GPS_RX_PIN GPIO_NUM_43      // ESP32 RX ← GPS TX
#define GPS_BAUD_RATE 115200        // Currently testing 115200
```

**Wiring** (confirmed correct):
- ESP32 TX (GPIO 44) → GPS RX (crossover is correct)
- ESP32 RX (GPIO 43) ← GPS TX (crossover is correct)
- VCC → 5V
- GND → GND

---

## Testing Strategy

### Step 1: Test UART2 (Current)

**Status**: Building now

**What to look for in console**:
```
📡 GPS UART active: X bytes received (total: XXX)
```

**Expected GPS data** (NMEA sentences):
```
$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47
$GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A
```

**Hex dump should show**:
- Start with `24` (ASCII '$')
- Followed by `47 50` (ASCII 'GP') or `47 4E` (ASCII 'GN')
- NOT "ToF:" or other console output

### Step 2: If UART2 Still Wrong

Try UART0:

```c
#define GPS_UART_NUM UART_NUM_0     // Try UART0
```

⚠️ **Warning**: UART0 is typically used for console, so this might conflict with logging.

### Step 3: Baud Rate Testing

If we get NMEA sentences but they're garbled, try different baud rates:

**Common Neo-M9N baud rates**:
1. **9600** (factory default for many GPS modules)
2. **38400** (Neo-M9N default according to internet)
3. **115200** (high-speed mode)

Change in `main/main.c`:
```c
#define GPS_BAUD_RATE 9600    // Try this first if data is garbled
```

---

## Debug Output Analysis

### Good GPS Data (What We Want)

**Console output**:
```
I (5000) GPS: 📡 GPS UART active: 82 bytes received (total: 410)
```

**Hex dump** (first few bytes):
```
24 47 50 47 47 41 2c 31 32 33 35 31 39 2c ...
 $  G  P  G  G  A  ,  1  2  3  5  1  9  ,
```

**Parsed output**:
```
I (30000) GPS: ✓ First GPS fix acquired in 26 seconds
```

### Bad Data (Console Output)

**Console output**:
```
I (5000) GPS: 📡 GPS UART active: 12 bytes received (total: 60)
```

**Hex dump**:
```
54 6f 46 3a 20 20 20 33 38 0d 0a
 T  o  F  :        3  8  \r \n
```

This means UART is connected to console/debug output, not GPS.

### No Data

**Console output**:
```
W (5000) GPS: ⚠️  No GPS data received for 5 seconds
W (5000) GPS: 💡 Check: 1) GPS powered? 2) TX/RX wiring? 3) Baud rate (115200)?
```

**Possible causes**:
1. GPS not powered (check LED on GPS module)
2. TX/RX wires swapped or disconnected
3. Wrong baud rate
4. GPS module defective

---

## ESP32-S3 UART Port Reference

The ESP32-S3 has 3 UART ports:

| UART | Typical Use | Can Remap? | Notes |
|------|-------------|------------|-------|
| UART0 | Console/Programming | Yes | Default console, conflicts with logging |
| UART1 | General purpose | Yes | Was receiving console output in our case |
| UART2 | General purpose | Yes | Currently testing |

**All UARTs can be mapped to any GPIO pins** - the issue is not hardware limitation but configuration.

---

## Next Steps

### If UART2 Works ✅
- Check if we receive NMEA sentences starting with `$GPGGA` or `$GPRMC`
- If garbled, try different baud rates (9600, 38400)
- If clear, GPS is working!

### If UART2 Also Receives Console Output ❌
- This suggests console is being broadcast to multiple UARTs
- Try UART0 (might work if console is on UART1/UART2)
- Check `sdkconfig` for console UART configuration

### If No Data on Any UART ⚠️
- Verify GPS module has power (LED should be on)
- Use multimeter to check voltage on TX pin (should toggle)
- Try different baud rates
- Consider GPS module might be defective

---

## Console UART Configuration

Check which UART is used for console:

```bash
idf.py menuconfig
# Navigate to: Component config → ESP System Settings → Channel for console output
```

**Options**:
- Default: UART0
- Custom: Can be changed to UART1 or UART2

If console is on UART1, that explains why we received "ToF:" output on UART1!

**Solution**: Use a different UART for GPS (UART2 or UART0).

---

## Testing Checklist

- [ ] Build with UART2 configuration
- [ ] Flash to device
- [ ] Check console for "📡 GPS UART active" messages
- [ ] Check hex dump for `$GP` or `$GN` (ASCII 24 47 50)
- [ ] If wrong data, check which UART is console in menuconfig
- [ ] If no data, verify GPS power and wiring
- [ ] If garbled, try baud rates: 9600 → 38400 → 115200
- [ ] Once working, verify NMEA sentences are parsed correctly

---

## Expected Timeline

- **UART2 test**: 5 minutes (current build)
- **Baud rate tests**: 5 minutes each
- **UART0 test**: 10 minutes (if needed)
- **Total**: 15-30 minutes to find working configuration

---

## Success Criteria

GPS is working when you see:

```
I (5000) GPS: 📡 GPS UART active: 82 bytes received (total: 410)
I (5000) GPS: Hex dump shows: $GPGGA,123519,4807.038,N...
I (30000) GPS: ✓ First GPS fix acquired in 26 seconds
```

And in GPS mode (Mode 4):
- Latitude/Longitude displayed (not 0.000000)
- Satellites: > 0
- Fix status: "3D FIX" or "2D FIX" (not "SEARCHING")

---

## Reference: NMEA Sentence Format

**GPGGA** (Position fix):
```
$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47
       ^^^^^^ ^^^^^^^^^ ^ ^^^^^^^^^^ ^ ^ ^^ ^^^ ^^^^^ ^
       Time   Latitude  N Longitude  E Q Sat HDOP Alt   Checksum
```

**GPRMC** (Recommended minimum):
```
$GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A
       ^^^^^^ ^ ^^^^^^^^^ ^ ^^^^^^^^^^ ^ ^^^^ ^^^^ ^^^^^^
       Time   A Latitude  N Longitude  E Speed Head Date
```

All NMEA sentences:
- Start with `$` (0x24)
- End with `*XX\r\n` (checksum + CRLF)
- ASCII text format
- 9600-115200 baud, 8N1

---

**Last Updated**: January 28, 2026  
**Status**: Testing UART2 with 115200 baud
