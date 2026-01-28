# GPS Final Configuration Summary

**Date**: January 28, 2026  
**Status**: ✅ Code Updated, ⚠️ Hardware Rewiring Required

---

## Problem Identified

**GPIO 43 and 44 are UART0 (console) pins** on ESP32-S3 DevKitC board.
- These pins are hardwired to the USB-to-Serial chip
- Cannot be used for GPS without disabling console
- That's why we saw "ToF: 20 19 21" instead of GPS data

**Source**: [ESP32-S3 DevKitC Pinout Guide](https://randomnerdtutorials.com/esp32-s3-devkitc-pinout-guide/)

---

## Solution Implemented

### New GPIO Pin Assignment

| Signal | Old (WRONG) | New (CORRECT) | Function |
|--------|-------------|---------------|----------|
| ESP32 TX | GPIO 44 ❌ | **GPIO 38** ✅ | ESP32 → GPS RX |
| ESP32 RX | GPIO 43 ❌ | **GPIO 47** ✅ | ESP32 ← GPS TX |
| VCC | 5V ❌ | **3.3V** ✅ | Power (Neo-M9N uses 3.3V!) |
| GND | GND | GND | Ground |

### UART Configuration

```c
#define GPS_UART_NUM UART_NUM_1     // UART1 (software UART)
#define GPS_TX_PIN GPIO_NUM_38      // Free GPIO
#define GPS_RX_PIN GPIO_NUM_47      // Free GPIO
#define GPS_BAUD_RATE 38400         // Neo-M9N default
```

---

## Neo-M9N GPS Specifications

**From u-blox NEO-M9N Datasheet:**

### Hardware Specifications
- **Power**: 3.3V (NOT 5V!) - 3.3V VCC and I/O
- **Current**: 25 mA typical, 67 mA max
- **Antenna**: Ceramic patch (integrated)
- **LED**: Power indicator (should be lit)

### UART Interface
- **Default Baud Rate**: 38400 bps
- **Data Format**: 8 data bits, no parity, 1 stop bit (8N1)
- **Protocol**: NMEA 0183
- **Supported Baud Rates**: 4800, 9600, 19200, 38400, 57600, 115200, 230400

### GNSS Performance
- **Constellations**: GPS, GLONASS, Galileo, BeiDou (concurrent)
- **Channels**: 92 tracking channels
- **Update Rate**: 1 Hz (default), up to 25 Hz max
- **Accuracy**: 2.5m CEP (horizontal)
- **Cold Start**: ~26 seconds (no almanac)
- **Warm Start**: ~2 seconds (almanac available)
- **Hot Start**: ~1 second (recent fix)
- **Sensitivity**: -160 dBm (tracking), -148 dBm (acquisition)

### NMEA Sentences
- **$GPGGA**: Position fix (lat, lon, alt, satellites, HDOP, fix quality)
- **$GPRMC**: Recommended minimum (date, time, speed, heading, status)
- **$GPVTG**: Track made good and ground speed
- **$GPGSA**: DOP and active satellites
- **$GPGSV**: Satellites in view

**Output Format**: ASCII text, 82 bytes typical per sentence

---

## Required Hardware Changes

### Step 1: Disconnect Old Wiring

```
GPS Module          ESP32-S3
----------          --------
TX        -X-X-X->  GPIO 43  (REMOVE)
RX        <-X-X-X-  GPIO 44  (REMOVE)
```

### Step 2: Connect New Wiring

```
GPS Module          ESP32-S3
----------          --------
TX        ───────>  GPIO 47  ✅ (NEW)
RX        <───────  GPIO 38  ✅ (NEW)
VCC       ───────>  3.3V     (CORRECTED - was 5V)
GND       ───────>  GND      (KEEP)
```

**Crossover is CORRECT**:
- GPS TX → ESP32 RX (GPIO 47)
- GPS RX ← ESP32 TX (GPIO 38)

---

## Code Changes (Already Applied)

### Files Modified

1. **main/main.c**:
   - GPS_TX_PIN: GPIO 44 → GPIO 38
   - GPS_RX_PIN: GPIO 43 → GPIO 47
   - GPS_BAUD_RATE: 9600 → 38400
   - Updated initialization messages

2. **docs/hardware-wiring-diagram.md**:
   - Added complete GPS section with specifications
   - Corrected SD card pins (14,15,2,16,17,18)
   - Updated quick reference table
   - Added revision history

3. **docs/GPS_WIRING_CORRECTION_CRITICAL.md**:
   - Documented the GPIO 43/44 console pin issue
   - Explained why we saw console output
   - Provided troubleshooting guide

4. **GPS_REWIRING_INSTRUCTIONS.txt**:
   - Simple step-by-step rewiring guide
   - Visual ASCII diagrams

---

## Testing Procedure

### Step 1: Rewire GPS Module
- Move GPS TX from GPIO 43 to GPIO 47
- Move GPS RX from GPIO 44 to GPIO 38
- Verify VCC (5V) and GND connections

### Step 2: Build and Flash
```bash
# Activate ESP-IDF environment
. C:\Users\ervan\esp\v5.5.1\esp-idf\export.ps1

# Build
idf.py build

# Flash and monitor
idf.py flash monitor
```

### Step 3: Verify Initialization
Look for in console:
```
I (XXX) MAIN: Initializing GPS module (Neo-M9N)...
I (XXX) MAIN: ⚠️  IMPORTANT: GPIO 43/44 are UART0 (console) - using GPIO 38/47 instead!
I (XXX) MAIN: Wiring: TX=GPIO38 (ESP32→GPS RX), RX=GPIO47 (ESP32←GPS TX)
I (XXX) GPS: ✓ GPS initialized successfully
```

### Step 4: Check UART Activity (Every 5 seconds)
```
I (5000) GPS: 📡 GPS UART active: 82 bytes received (total: 410)
```

### Step 5: Verify NMEA Data (Hex Dump)
**Good (GPS data)**:
```
I (5000) GPS: 0x3fceb7c0   24 47 50 47 47 41 2c 31  32 33 35 31 39 2c ...
                            $  G  P  G  G  A  ,  1   2  3  5  1  9  ,
```

**Bad (console output - means wrong pins)**:
```
❌ I (5000) GPS: 0x3fceb7c0   54 6f 46 3a 20 20 20 33  38 ...
                               T  o  F  :        3   8
```

### Step 6: Wait for GPS Fix (Outdoors)
```
I (30000) GPS: ✓ First GPS fix acquired in 26 seconds
```

### Step 7: Check GPS Mode (Mode 4)
- Press MODE button to cycle to GPS mode
- Should display:
  - Latitude/Longitude (real coordinates, not 0.000000)
  - Satellites: > 4
  - Fix status: "3D FIX" or "2D FIX" (not "SEARCHING")
  - HDOP: < 2.0 (good accuracy)

---

## Troubleshooting

### Still Seeing Console Output?
- Double-check wiring: GPS TX → GPIO 47, GPS RX → GPIO 38
- Verify you're not using GPIO 43/44
- Check for loose connections

### No Data Received?
- Verify GPS has power (LED should be lit)
- Check TX/RX are not swapped
- Try different baud rate: 9600, 38400, 115200
- Verify GPS module is working (test with USB-to-Serial adapter)

### No GPS Fix?
- Move outdoors or near window (needs sky view)
- Wait 1-5 minutes for cold start
- Check antenna is connected (ceramic patch on module)
- Verify satellites > 0 in GPS mode

### Garbled Data?
- Wrong baud rate - try 9600, 38400, or 115200
- Check UART configuration (8N1)
- Verify wiring is correct

---

## GPIO Pin Summary

### Reserved Pins (DO NOT USE)
- **GPIO 43, 44**: UART0 (console) - hardwired to USB chip
- **GPIO 0**: Strapping pin (boot mode)
- **GPIO 45, 46**: Strapping pins (boot mode)

### Used Pins (Current System)
- **GPIO 2, 14-18**: SD Card (SDMMC)
- **GPIO 3**: Flag button
- **GPIO 4, 8, 9**: ToF sensor (I2C)
- **GPIO 5, 6, 10-13**: IMU sensor (SPI)
- **GPIO 20**: Mode button
- **GPIO 38, 47**: GPS (UART) ✅ NEW
- **GPIO 48**: WS2812 LED
- **GPIO 0, 1, 7, 21, 35**: OLED display (SPI)
- **GPIO 45**: Buzzer (PWM)
- **GPIO 46**: Potentiometer (ADC)

### Available Pins (Future Expansion)
- GPIO 36, 37, 39, 40, 41, 42

---

## Success Criteria

GPS is working correctly when you see:

✅ Console shows: `I (5000) GPS: 📡 GPS UART active: XX bytes`  
✅ Hex dump shows: `24 47 50 47 47 41` ($GPGGA)  
✅ First fix acquired: `I (30000) GPS: ✓ First GPS fix acquired`  
✅ GPS mode displays real coordinates (not 0.000000)  
✅ Satellites > 4  
✅ Fix status: "3D FIX" or "2D FIX"  

---

## References

1. **ESP32-S3 Pinout**: https://randomnerdtutorials.com/esp32-s3-devkitc-pinout-guide/
2. **Neo-M9N Datasheet**: https://content.u-blox.com/sites/default/files/NEO-M9N-00B_DataSheet_UBX-19014285.pdf
3. **NMEA 0183 Protocol**: https://en.wikipedia.org/wiki/NMEA_0183
4. **u-blox M9 Integration Manual**: Available from u-blox website

---

**Status**: Ready for hardware rewiring and testing  
**Next Step**: Rewire GPS module to GPIO 38/47 and test

