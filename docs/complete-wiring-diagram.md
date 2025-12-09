# Complete Wiring Diagram - ProxiTof System

## Overview

This document shows the complete wiring for all components in the ProxiTof Near-Miss Detection System.

## Pin Summary Table

| Component | Interface | Pins Used | Notes |
|-----------|-----------|-----------|-------|
| VL53L8CX ToF | I2C | SDA=8, SCL=9, LPN=4 | Dedicated I2C bus |
| BNO08x IMU | SPI | MOSI=11, MISO=13, SCK=12, CS=10, INT=5, RST=6 | SPI2_HOST |
| SD Card | SPI | MOSI=11, MISO=13, SCK=12, CS=7 | **Shared SPI bus with IMU** |
| Button | GPIO | GPIO 3 (with internal pull-up) | Active low |
| RGB LED | GPIO | GPIO 48 | WS2812 LED |

## Detailed Wiring

### 1. VL53L8CX ToF Sensor (I2C)

```
ESP32-S3          VL53L8CX
--------          --------
GPIO 8  (SDA) --> SDA
GPIO 9  (SCL) --> SCL
GPIO 4  (LPN) --> LPN (Low Power/Reset)
3.3V          --> VDD
GND           --> GND
```

**Notes:**
- I2C address: 0x29 (7-bit)
- Internal pull-ups enabled
- External 2.2kΩ pull-ups recommended for long wires

### 2. BNO08x IMU Sensor (SPI)

```
ESP32-S3          BNO08x
--------          ------
GPIO 11 (MOSI) --> DI (MOSI)
GPIO 13 (MISO) --> SDA (MISO)
GPIO 12 (SCK)  --> SCK
GPIO 10 (CS)   --> CS
GPIO 5  (INT)  --> INT
GPIO 6  (RST)  --> RST
3.3V           --> VCC
3.3V           --> P0 (Protocol Select 0)
3.3V           --> P1 (Protocol Select 1)
GND            --> GND
FLOATING       --> BT (Boot Mode - leave disconnected)
```

**Critical Configuration:**
- **P0 = 3.3V** (HIGH)
- **P1 = 3.3V** (HIGH)
- **BT = FLOATING** (disconnected)
- This configures SPI mode

### 3. SD Card Module (SPI - Shared Bus)

```
ESP32-S3          SD Card
--------          -------
GPIO 11 (MOSI) --> DI/MOSI   (SHARED with IMU)
GPIO 13 (MISO) --> DO/MISO   (SHARED with IMU)
GPIO 12 (SCK)  --> CLK       (SHARED with IMU)
GPIO 7  (CS)   --> CS        (UNIQUE - different from IMU)
3.3V           --> VCC
GND            --> GND
```

**Important:**
- **Shares SPI bus with IMU** (MOSI, MISO, SCK)
- **Different CS pin** (GPIO 7 vs IMU's GPIO 10)
- SD card must be **FAT32 formatted**
- Use **3.3V compatible** SD card module

### 4. Button (GPIO)

```
ESP32-S3          Button
--------          ------
GPIO 3         --> One side
GND            --> Other side
```

**Notes:**
- Internal pull-up enabled (no external resistor needed)
- Active LOW (pressed = 0V, released = 3.3V)
- 200ms debounce in software

### 5. RGB LED (WS2812)

```
ESP32-S3          WS2812 LED
--------          ----------
GPIO 48        --> DIN (Data In)
3.3V           --> VCC
GND            --> GND
```

**Notes:**
- Built-in WS2812 RGB LED on ESP32-S3-DevKitC-1
- Controlled via RMT driver
- No external components needed

## Complete System Diagram

```
                    ESP32-S3-DevKitC-1
                    ==================
                    
I2C Bus:            SPI Bus (Shared):           GPIO:
--------            ------------------          -----
GPIO 8  ────────────> VL53L8CX ToF    GPIO 3  ────> Button
GPIO 9  ────────────>                  GPIO 48 ────> WS2812 LED
GPIO 4  ────────────>                  
                    GPIO 11 ──┬──────> BNO08x IMU (MOSI)
                              └──────> SD Card (MOSI)
                              
                    GPIO 13 ──┬──────> BNO08x IMU (MISO)
                              └──────> SD Card (MISO)
                              
                    GPIO 12 ──┬──────> BNO08x IMU (SCK)
                              └──────> SD Card (SCK)
                              
                    GPIO 10 ──────────> BNO08x IMU (CS)
                    GPIO 7  ──────────> SD Card (CS)
                    GPIO 5  ──────────> BNO08x IMU (INT)
                    GPIO 6  ──────────> BNO08x IMU (RST)

Power:
------
3.3V ──┬──> VL53L8CX
       ├──> BNO08x (VCC, P0, P1)
       ├──> SD Card
       └──> WS2812 LED
       
GND  ──┴──> All components
```

## SPI Bus Sharing Explained

### How It Works

The SPI bus uses **Chip Select (CS)** pins to control which device is active:

1. **When IMU CS is LOW (active):**
   - IMU listens to MOSI/MISO/SCK
   - SD card ignores the bus (CS is HIGH)

2. **When SD Card CS is LOW (active):**
   - SD card listens to MOSI/MISO/SCK
   - IMU ignores the bus (CS is HIGH)

### Benefits

- ✅ **Saves 3 GPIO pins** (MOSI, MISO, SCK shared)
- ✅ **Standard practice** for SPI devices
- ✅ **No performance impact** (devices take turns)
- ✅ **Automatic arbitration** by SPI protocol

### Timing

- IMU reads: ~100Hz (10ms intervals)
- SD card writes: Batched every 500ms
- No conflicts because operations are fast (<1ms each)

## Power Requirements

| Component | Voltage | Current (Typical) | Current (Peak) |
|-----------|---------|-------------------|----------------|
| ESP32-S3 | 3.3V | 80mA | 500mA |
| VL53L8CX ToF | 3.3V | 20mA | 40mA |
| BNO08x IMU | 3.3V | 10mA | 15mA |
| SD Card | 3.3V | 50mA | 200mA |
| WS2812 LED | 3.3V | 20mA | 60mA |
| **Total** | **3.3V** | **~180mA** | **~815mA** |

**Recommendation:** Use USB power (5V → 3.3V regulator on DevKit) or external 3.3V supply capable of 1A.

## Troubleshooting

### ToF Sensor Not Detected
- Check I2C wiring (SDA, SCL, LPN)
- Verify 3.3V power
- Try external 2.2kΩ pull-ups on SDA/SCL
- Check I2C address (should be 0x29)

### IMU Not Initializing
- **Critical:** Verify P0 and P1 are both at 3.3V
- Verify BT pin is FLOATING (not connected)
- Check SPI wiring (MOSI, MISO, SCK, CS)
- Check INT and RST pins

### SD Card Not Mounting
- Verify SD card is FAT32 formatted
- Check SPI wiring (shared with IMU)
- Verify CS pin is GPIO 7 (not GPIO 10)
- Try different SD card
- Check 3.3V power supply

### Button False Triggers
- Verify internal pull-up is enabled
- Check GPIO 3 connection
- Increase debounce time in code if needed

### LED Not Working
- Check GPIO 48 connection
- Verify WS2812 LED is powered
- Check if LED is built-in to DevKit

## Hardware Checklist

Before powering on:

- [ ] All power connections (3.3V, GND) verified
- [ ] No short circuits between pins
- [ ] BNO08x P0 and P1 at 3.3V
- [ ] BNO08x BT pin floating
- [ ] SD card formatted as FAT32
- [ ] Button wired with correct polarity
- [ ] All GPIO pins match code configuration

## Next Steps

1. **Double-check all connections** against this diagram
2. **Power on the system**
3. **Build and flash firmware** (`idf.py build flash monitor`)
4. **Verify initialization** in serial output
5. **Test each component** individually
6. **Start data logging!**

---

**Document Version:** 1.0  
**Last Updated:** December 9, 2024  
**Hardware:** ESP32-S3-DevKitC-1, VL53L8CX, BNO08x, SD Card Module
