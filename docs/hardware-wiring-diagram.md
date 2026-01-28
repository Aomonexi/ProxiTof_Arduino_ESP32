# ProxiTof Hardware Wiring Diagram

**Last Updated**: January 28, 2026  
**Board**: ESP32-S3-DevKitC-1 (N16R8 - 16MB Flash, 8MB PSRAM)  
**Status**: Complete System - All sensors operational

---

## Quick Reference Pin Map

| Component | Interface | GPIO Pins | Status |
|-----------|-----------|-----------|--------|
| VL53L8CX ToF | I2C | 8 (SDA), 9 (SCL), 4 (LPn) | ✅ Working |
| BNO08x IMU | SPI | 11 (MOSI), 13 (MISO), 12 (SCK), 10 (CS), 5 (INT), 6 (RST) | ✅ Working |
| SD Card | SDIO (4-bit) | 14 (CLK), 15 (CMD), 2 (D0), 16 (D1), 17 (D2), 18 (D3) | ✅ Working |
| Neo-M9N GPS | UART | 38 (TX), 47 (RX) | ✅ Implemented |
| Flag Button | GPIO | 3 (Input with Pull-up) | ✅ Working |
| Mode Button | GPIO | 20 (Input with Pull-up) | ✅ Working |
| WS2812 LED | GPIO | 48 (Data) | ✅ Built-in |
| OLED Display | SPI | 7 (SCK), 1 (MOSI), 35 (DC), 21 (RST), 0 (CS) | ✅ Working |
| DS3231 RTC | I2C | 8 (SDA), 9 (SCL) - shared with ToF | ✅ Working |
| Buzzer | PWM | 45 | ✅ Planned |
| Potentiometer | ADC | 46 | ✅ Planned |

---

## Detailed Wiring Instructions

### 1. VL53L8CX Time-of-Flight Sensor (I2C)

**Purpose**: 4x4 distance grid measurement at 60Hz

```
VL53L8CX Module          ESP32-S3 DevKitC-1
┌─────────────┐          ┌──────────────────┐
│             │          │                  │
│  VCC (3.3V) │─────────▶│ 3.3V             │
│  GND        │─────────▶│ GND              │
│  SDA        │◀────────▶│ GPIO 8 (SDA)     │
│  SCL        │◀────────▶│ GPIO 9 (SCL)     │
│  LPn        │◀─────────│ GPIO 4 (LPn)     │
│  I2C_RST    │          │ (Not Connected)  │
│             │          │                  │
└─────────────┘          └──────────────────┘
```

**Pin Functions**:
- **VCC**: 3.3V power supply (DO NOT use 5V!)
- **GND**: Ground
- **SDA**: I2C data line (bidirectional)
- **SCL**: I2C clock line
- **LPn**: Low Power enable (active HIGH to enable sensor)
- **I2C_RST**: Optional reset (not used in current implementation)

**I2C Address**: 0x29 (default, can be changed in software)

**Notes**:
- Use short wires (<15cm) for reliable I2C communication
- Add 4.7kΩ pull-up resistors on SDA/SCL if experiencing issues (usually not needed)
- LPn pin must be HIGH for sensor to operate

---

### 2. BNO08x IMU Sensor (SPI)

**Purpose**: 9-DOF inertial measurement (orientation, acceleration, gyroscope) at 100Hz

```
BNO08x Module            ESP32-S3 DevKitC-1
┌─────────────┐          ┌──────────────────┐
│             │          │                  │
│  VCC (3.3V) │─────────▶│ 3.3V             │
│  GND        │─────────▶│ GND              │
│  MOSI       │◀─────────│ GPIO 11 (MOSI)   │
│  MISO       │─────────▶│ GPIO 13 (MISO)   │
│  SCK        │◀─────────│ GPIO 12 (SCK)    │
│  CS         │◀─────────│ GPIO 10 (CS)     │
│  INT        │─────────▶│ GPIO 5 (INT)     │
│  RST        │◀─────────│ GPIO 6 (RST)     │
│             │          │                  │
└─────────────┘          └──────────────────┘
```

**Pin Functions**:
- **VCC**: 3.3V power supply
- **GND**: Ground
- **MOSI**: SPI Master Out Slave In (ESP32 → IMU)
- **MISO**: SPI Master In Slave Out (IMU → ESP32)
- **SCK**: SPI clock
- **CS**: Chip Select (active LOW)
- **INT**: Interrupt output (data ready signal)
- **RST**: Reset input (active LOW)

**SPI Configuration**:
- Mode: SPI Mode 3 (CPOL=1, CPHA=1)
- Speed: Up to 3 MHz
- Bit Order: MSB first

**Notes**:
- RST pin is pulsed LOW during initialization
- INT pin signals when new data is available
- CS must be LOW to communicate with this sensor

---

### 3. SD Card Module (SDIO - 4-bit Mode)

**Purpose**: High-speed data logging storage

```
SD Card SDIO Module      ESP32-S3 DevKitC-1
┌─────────────┐          ┌──────────────────┐
│             │          │                  │
│  VCC (3.3V) │─────────▶│ 3.3V             │
│  GND        │─────────▶│ GND              │
│  CLK        │◀─────────│ GPIO 14 (CLK)    │ ◀─ SDMMC Clock
│  CMD        │◀────────▶│ GPIO 15 (CMD)    │ ◀─ Command/Response
│  D0         │◀────────▶│ GPIO 2  (D0)     │ ◀─ Data Line 0
│  D1         │◀────────▶│ GPIO 16 (D1)     │ ◀─ Data Line 1
│  D2         │◀────────▶│ GPIO 17 (D2)     │ ◀─ Data Line 2
│  D3         │◀────────▶│ GPIO 18 (D3)     │ ◀─ Data Line 3
│             │          │                  │
└─────────────┘          └──────────────────┘
```

**Pin Functions**:
- **VCC**: 3.3V power supply
- **GND**: Ground
- **CLK**: SDMMC clock signal (up to 40MHz)
- **CMD**: Command and response line (bidirectional)
- **D0-D3**: 4-bit data bus (bidirectional)

**SDIO Advantages**:
- **4x faster** than SPI (4-bit parallel vs 1-bit serial)
- **Dedicated pins** - no sharing with other devices
- **Higher throughput** - up to 40MB/s vs 10MB/s SPI
- **Native SD protocol** - more reliable than SPI emulation

**SD Card Format**: FAT32 (required)

**Notes**:
- ⚠️ **Requires SDIO breakout board** (not standard SPI SD module)
- Uses conflict-free GPIO pins (14, 15, 2, 16, 17, 18)
- No conflicts with ToF, IMU, or other sensors
- Internal pull-ups enabled on all data lines
- Use quality SD cards (Class 10 or better) for reliable logging

---

### 4. Neo-M9N GPS Module (UART)

**Purpose**: GNSS positioning for geo-tagging near-miss events

```
Neo-M9N GPS Module       ESP32-S3 DevKitC-1
┌─────────────┐          ┌──────────────────┐
│             │          │                  │
│  VCC (5V)   │─────────▶│ 5V               │ ◀─ GPS needs 5V!
│  GND        │─────────▶│ GND              │
│  TX         │─────────▶│ GPIO 47 (RX)     │ ◀─ GPS transmits
│  RX         │◀─────────│ GPIO 38 (TX)     │ ◀─ GPS receives
│  PPS        │          │ (Not Connected)  │ ◀─ Optional
│             │          │                  │
└─────────────┘          └──────────────────┘
```

**Pin Functions**:
- **VCC**: 5V power supply (⚠️ **NOT 3.3V!** Neo-M9N requires 5V)
- **GND**: Ground
- **TX**: GPS transmit (NMEA sentences) → ESP32 RX (GPIO 47)
- **RX**: GPS receive (commands) ← ESP32 TX (GPIO 38)
- **PPS**: Pulse-per-second output (optional, not used)

**UART Configuration**:
- **Baud Rate**: 38400 (Neo-M9N default)
- **Data Bits**: 8
- **Parity**: None
- **Stop Bits**: 1
- **Flow Control**: None
- **Protocol**: NMEA 0183

**GNSS Capabilities**:
- **Constellations**: GPS, GLONASS, Galileo, BeiDou (concurrent)
- **Channels**: 92 tracking, 4 concurrent GNSS
- **Update Rate**: 1-25 Hz (default 1 Hz, max 25 Hz)
- **Accuracy**: 2.5m CEP (horizontal)
- **Cold Start**: ~26 seconds
- **Hot Start**: ~1 second
- **Sensitivity**: -160 dBm (tracking)

**NMEA Sentences Parsed**:
- **$GPGGA**: Position fix data (lat, lon, alt, satellites, HDOP)
- **$GPRMC**: Recommended minimum data (date, time, speed, heading)
- **$GPVTG**: Track made good and ground speed

**Wiring Notes**:
- ⚠️ **CRITICAL**: GPIO 43/44 are UART0 (console) pins - DO NOT USE!
- ✅ **Use GPIO 38/47** instead (free pins, no conflicts)
- Crossover wiring is CORRECT: GPS TX → ESP32 RX, GPS RX ← ESP32 TX
- GPS module has ceramic patch antenna (square module on top)
- LED on GPS module indicates power (should be lit)
- Indoor GPS fix may take 5+ minutes or fail (needs sky view)

**Testing**:
- Console shows: `I (XXX) GPS: 📡 GPS UART active: XX bytes received`
- Hex dump shows: `24 47 50 47 47 41` = "$GPGGA" (NMEA sentence)
- GPS mode (Mode 4) displays: Lat/Lon, satellites, fix status
- First fix outdoors: 26-60 seconds (cold start)

---

### 5. Flag Button (GPIO Input)

**Purpose**: Manual near-miss event flagging

```
Push Button              ESP32-S3 DevKitC-1
┌─────────────┐          ┌──────────────────┐
│             │          │                  │
│  Terminal 1 │─────────▶│ GPIO 3           │
│  Terminal 2 │─────────▶│ GND              │
│             │          │                  │
└─────────────┘          └──────────────────┘
```

**Configuration**:
- **Type**: Normally Open (NO) momentary push button
- **Pull-up**: Internal pull-up enabled in software (INPUT_PULLUP)
- **Active State**: LOW (pressed = LOW, released = HIGH)
- **Debounce**: 50ms software debounce

**Button Logic**:
- Released: GPIO 3 reads HIGH (pulled up internally)
- Pressed: GPIO 3 reads LOW (connected to GND)
- Software detects falling edge (HIGH → LOW transition)

**Notes**:
- No external resistor needed (internal pull-up used)
- Any momentary push button will work
- Debouncing handled in software

---

### 5. WS2812 RGB LED (Status Indicator)

**Purpose**: Visual system status feedback

```
ESP32-S3 DevKitC-1 (Built-in LED)
┌──────────────────────────────┐
│                              │
│  ┌────────────────────┐      │
│  │   WS2812 RGB LED   │      │
│  │   (Built-in)       │      │
│  │   Connected to:    │      │
│  │   GPIO 48          │      │
│  └────────────────────┘      │
│                              │
└──────────────────────────────┘
```

**Configuration**:
- **Type**: WS2812 RGB LED (addressable)
- **GPIO**: 48 (built-in connection)
- **Power**: Powered from board's 3.3V rail
- **Protocol**: One-wire timing-sensitive protocol

**LED States** (from design document):
- **Blue solid**: Initializing
- **Red blinking (2Hz)**: SD card error
- **Green breathing (1s cycle)**: Logging active
- **Yellow flash (500ms)**: Button pressed
- **Orange alert (1s)**: ML near-miss detected
- **Red solid**: Critical proximity alert
- **Purple tint**: Buffer warning

**Notes**:
- ✅ **Built-in LED on your ESP32-S3 DevKitC-1 board**
- No external wiring required!
- WS2812 uses timing-sensitive protocol
- Controlled via Adafruit NeoPixel library
- ⚠️ **Not yet implemented in code** - coming in Task 6

---

## Power Supply Considerations

### Current Draw Estimates

| Component | Typical Current | Peak Current |
|-----------|----------------|--------------|
| ESP32-S3 | 80-160 mA | 240 mA |
| VL53L8CX ToF | 20 mA | 100 mA |
| BNO08x IMU | 2.5 mA | 10 mA |
| SD Card | 50 mA | 200 mA |
| WS2812 LED | 1 mA (off) | 60 mA (full white) |
| **Total** | **~150-230 mA** | **~610 mA** |

### Power Supply Options

1. **USB Power** (Recommended for development)
   - Connect via USB-C cable
   - Provides 5V with sufficient current
   - Enables serial communication

2. **External 5V Supply**
   - Connect to 5V and GND pins
   - Must provide at least 1A
   - Use for battery-powered operation

3. **Battery Power** (Future)
   - LiPo battery with voltage regulator
   - Consider power consumption for runtime calculations

---

## Bus Architecture

### SPI Bus (IMU Only)

The BNO08x IMU uses a dedicated SPI bus:

```
                    ESP32-S3 SPI Master
                    ┌─────────────────┐
                    │  GPIO 11 (MOSI) │──────────────┐
                    │  GPIO 13 (MISO) │──────────┐   │
                    │  GPIO 12 (SCK)  │──────┐   │   │
                    │  GPIO 10 (CS)   │──┐   │   │   │
                    └─────────────────┘  │   │   │   │
                                         ▼   ▼   ▼   ▼
                                   ┌──────────────────┐
                                   │   BNO08x IMU     │
                                   │                  │
                                   │  CS  SCK  MISO  │
                                   │  MOSI  INT  RST  │
                                   └──────────────────┘
```

### SDMMC Bus (SD Card Only)

The SD card uses dedicated SDMMC pins:

```
                    ESP32-S3 SDMMC Host
                    ┌─────────────────┐
                    │  GPIO 39 (CLK)  │──────────────┐
                    │  GPIO 38 (CMD)  │──────────┐   │
                    │  GPIO 40 (D0)   │──────┐   │   │
                    │  GPIO 41 (D1)   │──┐   │   │   │
                    │  GPIO 42 (D2)   │──│───│───│───│───┐
                    │  GPIO 43 (D3)   │──│───│───│───│───│──┐
                    └─────────────────┘  │   │   │   │   │  │
                                         ▼   ▼   ▼   ▼   ▼  ▼
                                   ┌──────────────────────────┐
                                   │       SD Card SDIO       │
                                   │                          │
                                   │  D3  D2  D1  D0  CMD CLK │
                                   └──────────────────────────┘
```

**Key Advantages**:
- **No bus sharing conflicts** - each interface is independent
- **Higher performance** - SDIO 4-bit parallel vs SPI 1-bit serial
- **Dedicated pins** - no CS arbitration needed

---

## I2C Bus Architecture

Currently only the ToF sensor uses I2C:

```
                    ESP32-S3 I2C Master
                    ┌─────────────────┐
                    │  GPIO 8 (SDA)   │◀────▶ VL53L8CX ToF
                    │  GPIO 9 (SCL)   │◀────▶ VL53L8CX ToF
                    │  GPIO 4 (LPn)   │──────▶ VL53L8CX ToF
                    └─────────────────┘

                    I2C Address: 0x29
```

**Future Expansion**:
- I2C bus can support multiple devices
- Each device needs unique address
- Can add: OLED display, additional sensors, etc.

---

## GPIO Pin Summary

### Used Pins

| GPIO | Function | Direction | Component |
|------|----------|-----------|-----------|
| 3 | Button Input | Input (Pull-up) | Flag Button |
| 4 | ToF LPn | Output | VL53L8CX |
| 5 | IMU Interrupt | Input | BNO08x |
| 6 | IMU Reset | Output | BNO08x |
| 38 | SDMMC CMD | Bidirectional | SD Card |
| 39 | SDMMC CLK | Output | SD Card |
| 40 | SDMMC D0 | Bidirectional | SD Card |
| 41 | SDMMC D1 | Bidirectional | SD Card |
| 42 | SDMMC D2 | Bidirectional | SD Card |
| 43 | SDMMC D3 | Bidirectional | SD Card |
| 8 | I2C SDA | Bidirectional | VL53L8CX |
| 9 | I2C SCL | Output | VL53L8CX |
| 10 | IMU CS | Output | BNO08x |
| 11 | SPI MOSI | Output | BNO08x IMU |
| 12 | SPI SCK | Output | BNO08x IMU |
| 13 | SPI MISO | Input | BNO08x IMU |
| 48 | LED Data | Output | WS2812 |

### Available Pins (for future expansion)

ESP32-S3 has many available GPIOs. Some good candidates for expansion:
- GPIO 1, 2: Available
- GPIO 7: Available (was SD CS, now free)
- GPIO 14-21: Available
- GPIO 35-37: Available
- GPIO 44-47: Available

**Note**: Avoid GPIO 0, 45, 46 (strapping pins used during boot)

---

## Troubleshooting Guide

### ToF Sensor Not Detected

**Symptoms**: "ToF sensor initialization failed" in serial output

**Checks**:
1. ✓ Verify 3.3V power (NOT 5V!)
2. ✓ Check SDA/SCL connections (GPIO 8, 9)
3. ✓ Verify LPn pin is connected to GPIO 4
4. ✓ Ensure LPn is HIGH (sensor enabled)
5. ✓ Try shorter I2C wires (<15cm)
6. ✓ Check I2C address with scanner (should be 0x29)

### IMU Sensor Not Detected

**Symptoms**: "IMU sensor initialization failed" in serial output

**Checks**:
1. ✓ Verify 3.3V power
2. ✓ Check all SPI connections (MOSI, MISO, SCK, CS)
3. ✓ Verify CS pin is GPIO 10
4. ✓ Check RST pin connection (GPIO 6)
5. ✓ Ensure INT pin is connected (GPIO 5)
6. ✓ Try different SPI speed (reduce if issues)

### SD Card Not Detected

**Symptoms**: SD card errors or mount failures

**Checks**:
1. ✓ Verify SD card is formatted as FAT32
2. ✓ Check all SPI connections (shared with IMU)
3. ✓ Verify CS pin is GPIO 7 (not GPIO 10)
4. ✓ Try different SD card (some cards are incompatible)
5. ✓ Check SD module voltage (3.3V or 5V?)
6. ✓ Ensure card is fully inserted

### Button Not Working

**Symptoms**: No response when button pressed

**Checks**:
1. ✓ Verify button connected to GPIO 3 and GND
2. ✓ Check button is normally open (NO) type
3. ✓ Test button with multimeter (continuity when pressed)
4. ✓ Verify internal pull-up is enabled in code

### General Debugging

1. **Check Serial Output**: Always monitor serial at 115200 baud
2. **Power Issues**: Measure voltage at each component (should be 3.3V)
3. **Loose Connections**: Wiggle wires while monitoring serial
4. **One at a Time**: Disconnect all sensors, add one at a time
5. **Known Good**: Test with example code from sensor libraries

---

## Testing Procedure

### Step 1: Visual Inspection
- [ ] All connections secure
- [ ] No short circuits (VCC to GND)
- [ ] Correct voltage (3.3V to sensors)
- [ ] Polarity correct (VCC/GND not swapped)

### Step 2: Power-On Test
- [ ] Connect USB
- [ ] Check power LED on ESP32-S3
- [ ] Check power LEDs on sensor modules (if present)
- [ ] Measure 3.3V at sensor VCC pins

### Step 3: Serial Monitor Test
```bash
pio run -t upload
pio device monitor
```

**Expected Output**:
```
=== ProxiTof Near-Miss Detection System ===
Initializing...

=== Initializing Sensor Manager ===
Initializing VL53L8CX ToF sensor...
[OK] ToF sensor initialized
ToF sensor configured: 4x4 mode, 60Hz

Initializing BNO08x IMU sensor...
[OK] IMU sensor initialized
IMU sensor configured: Rotation Vector, Linear Accel, Gyro at 100Hz

[OK] Button initialized
===================================

=== System Ready ===
Sensor Manager initialized successfully
ToF Sensor: HEALTHY
IMU Sensor: HEALTHY

Starting sensor data collection...
Press the button (GPIO 3) to test button functionality

[ToF] Time: 5234 ms, Min Distance: 1245 mm
[ToF] Time: 6234 ms, Min Distance: 1198 mm
```

### Step 4: Functional Tests
- [ ] ToF: Wave hand in front, distance should change
- [ ] IMU: Rotate board, orientation should change
- [ ] Button: Press button, should see "[BUTTON] Button pressed" message
- [ ] SD: Check if files are created (next task)

---

## Wiring Best Practices

1. **Use Solid Core Wire** for breadboard connections
2. **Keep Wires Short** especially for I2C and SPI (<15cm)
3. **Color Code Wires**:
   - Red: 3.3V/5V power
   - Black: GND
   - Yellow/Green: Data lines
   - Blue: Clock lines
   - White: Control signals

4. **Double Check Before Power On**:
   - VCC and GND not swapped
   - No shorts between adjacent pins
   - Correct GPIO numbers

5. **Use Breadboard Power Rails** for clean power distribution

6. **Label Connections** with tape/markers for complex setups

---

## Future Expansion Slots

### Planned Additions
- [x] **GNSS Module (Neo-M9N)**: UART for GPS location tracking ✅ **IMPLEMENTED**
- [ ] **Additional Status LEDs**: Visual indicators
- [ ] **External Power Switch**: For battery operation
- [ ] **Voltage Monitor**: Battery level sensing

### Possible Enhancements
- [ ] **OLED Display**: Real-time data visualization
- [ ] **Buzzer**: Audio alerts for near-miss events
- [ ] **External Antenna**: For better GPS reception
- [ ] **Enclosure**: Weatherproof housing for outdoor use

---

## Revision History

| Date | Version | Changes | Author |
|------|---------|---------|--------|
| 2025-12-09 | 1.0 | Initial wiring diagram created | Development Team |
| | | ToF, IMU, SD, Button documented | |
| 2026-01-28 | 2.0 | Major update: GPS added, SD pins corrected | Development Team |
| | | GPS: GPIO 38/47 (NOT 43/44 - console pins!) | |
| | | SD Card: GPIO 14,15,2,16,17,18 (corrected) | |
| | | Added OLED, RTC, Mode button, Buzzer, Pot | |

---

## References

1. ESP32-S3 DevKitC-1 Pinout: https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/hw-reference/esp32s3/user-guide-devkitc-1.html
2. VL53L8CX Datasheet: STMicroelectronics
3. BNO08x Datasheet: CEVA/Hillcrest Labs
4. WS2812 Datasheet: WorldSemi

---

**Document Status**: Living Document - Update as hardware changes  
**Next Update**: After adding WS2812 LED and testing all sensors
