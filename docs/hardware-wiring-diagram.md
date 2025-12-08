# ProxiTof Hardware Wiring Diagram

**Last Updated**: December 9, 2025  
**Board**: ESP32-S3-DevKitC-1 (N16R8 - 16MB Flash, 8MB PSRAM)  
**Status**: Initial Setup - ToF, IMU, SD Card, Button

---

## Quick Reference Pin Map

| Component | Interface | GPIO Pins | Status |
|-----------|-----------|-----------|--------|
| VL53L8CX ToF | I2C | 8 (SDA), 9 (SCL), 4 (LPn) | ✅ Wired |
| BNO08x IMU | SPI | 11 (MOSI), 13 (MISO), 12 (SCK), 10 (CS), 5 (INT), 6 (RST) | ✅ Wired |
| SD Card | SPI (shared) | 11 (MOSI), 13 (MISO), 12 (SCK), 7 (CS) | ✅ Wired |
| Flag Button | GPIO | 3 (Input with Pull-up) | 🔄 In Progress |
| WS2812 LED | GPIO | 48 (Data) | ✅ Built-in (on board) |

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

### 3. SD Card Module (SPI - Shared Bus)

**Purpose**: Data logging storage

```
SD Card Module           ESP32-S3 DevKitC-1
┌─────────────┐          ┌──────────────────┐
│             │          │                  │
│  VCC (3.3V) │─────────▶│ 3.3V             │
│  GND        │─────────▶│ GND              │
│  MOSI       │◀─────────│ GPIO 11 (MOSI)   │ ◀─ Shared with IMU
│  MISO       │─────────▶│ GPIO 13 (MISO)   │ ◀─ Shared with IMU
│  SCK        │◀─────────│ GPIO 12 (SCK)    │ ◀─ Shared with IMU
│  CS         │◀─────────│ GPIO 7 (CS)      │ ◀─ Unique CS pin
│             │          │                  │
└─────────────┘          └──────────────────┘
```

**Pin Functions**:
- **VCC**: 3.3V power supply (some modules have onboard regulator for 5V)
- **GND**: Ground
- **MOSI**: SPI data to SD card (shared with IMU)
- **MISO**: SPI data from SD card (shared with IMU)
- **SCK**: SPI clock (shared with IMU)
- **CS**: Chip Select for SD card (GPIO 7, unique)

**SPI Bus Sharing**:
- IMU and SD card share MOSI, MISO, SCK lines
- Each device has its own CS pin
- Only one device active at a time (controlled by CS)

**SD Card Format**: FAT32 (required)

**Notes**:
- Ensure SD card is formatted as FAT32 before use
- Some SD modules have level shifters (can use 5V)
- Check your module's voltage requirements!
- Use quality SD cards (Class 10 or better) for reliable logging

---

### 4. Flag Button (GPIO Input)

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

## SPI Bus Architecture

The system uses a shared SPI bus for IMU and SD card:

```
                    ESP32-S3 SPI Master
                    ┌─────────────────┐
                    │  GPIO 11 (MOSI) │────┬─────────────┐
                    │  GPIO 13 (MISO) │────┼──────┐      │
                    │  GPIO 12 (SCK)  │────┼──┐   │      │
                    │  GPIO 10 (CS1)  │────│──│───│──┐   │
                    │  GPIO 7  (CS2)  │────│──│───│──│───│──┐
                    └─────────────────┘    │  │   │  │   │  │
                                           │  │   │  │   │  │
                    ┌──────────────────────┘  │   │  │   │  │
                    │  ┌──────────────────────┘   │  │   │  │
                    │  │  ┌───────────────────────┘  │   │  │
                    │  │  │                          │   │  │
                    ▼  ▼  ▼                          ▼   ▼  ▼
              ┌──────────────┐                ┌──────────────┐
              │  BNO08x IMU  │                │  SD Card     │
              │              │                │              │
              │  MOSI  MISO  │                │  MOSI  MISO  │
              │  SCK   CS    │                │  SCK   CS    │
              └──────────────┘                └──────────────┘
                   CS=LOW                          CS=LOW
                  (GPIO 10)                       (GPIO 7)
```

**Key Points**:
- MOSI, MISO, SCK are shared between devices
- Each device has unique CS (Chip Select) pin
- Only one device can be active at a time
- CS is active LOW (device selected when CS=LOW)

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
| 7 | SD CS | Output | SD Card |
| 8 | I2C SDA | Bidirectional | VL53L8CX |
| 9 | I2C SCL | Output | VL53L8CX |
| 10 | IMU CS | Output | BNO08x |
| 11 | SPI MOSI | Output | IMU + SD |
| 12 | SPI SCK | Output | IMU + SD |
| 13 | SPI MISO | Input | IMU + SD |
| 48 | LED Data | Output | WS2812 |

### Available Pins (for future expansion)

ESP32-S3 has many available GPIOs. Some good candidates for expansion:
- GPIO 1, 2: Available
- GPIO 14-21: Available
- GPIO 35-37: Available
- GPIO 38-42: Available (but check strapping pins)

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
- [ ] **GNSS Module (Neo-M9N)**: UART for GPS location tracking
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

---

## References

1. ESP32-S3 DevKitC-1 Pinout: https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/hw-reference/esp32s3/user-guide-devkitc-1.html
2. VL53L8CX Datasheet: STMicroelectronics
3. BNO08x Datasheet: CEVA/Hillcrest Labs
4. WS2812 Datasheet: WorldSemi

---

**Document Status**: Living Document - Update as hardware changes  
**Next Update**: After adding WS2812 LED and testing all sensors
