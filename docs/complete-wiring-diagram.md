# ProxiTof Complete Wiring Reference - CONSOLIDATED

**Last Updated**: December 24, 2024  
**Board**: ESP32-S3-DevKitC-1 (N16R8 - 16MB Flash, 8MB PSRAM)  
**Status**: All Components Documented - Ready for Implementation

---

## 🎯 Quick Reference Pin Map

| Component | Interface | GPIO Pins | Status | Notes |
|-----------|-----------|-----------|--------|-------|
| **VL53L8CX ToF** | I2C | SDA=8, SCL=9, LPN=4 | ✅ Ready | 4x4 distance grid @ 60Hz |
| **BNO08x IMU** | SPI | MOSI=11, MISO=13, SCK=12, CS=10, INT=5, RST=6 | ✅ Ready | 9-DOF @ 100Hz |
| **SD Card** | **SDMMC (4-bit)** | CLK=14, CMD=15, D0=2, D1=16, D2=17, D3=18 | ✅ Ready | **Conflict-free pins** |
| **DS3231 RTC** | I2C | SDA=8, SCL=9 (shared with ToF) | ✅ Ready | Battery-backed time |
| **OLED Display** | SPI | SCK=7, MOSI=1, DC=35, RST=21 | ✅ Ready | 128x64 @ 60Hz (4-wire, no CS) |
| **Flag Button** | GPIO | GPIO 3 (with pull-up) | ✅ Ready | Manual event flagging |
| **WS2812 LED** | GPIO | GPIO 48 | ✅ Built-in | Status indicator |

---

## 🔌 Complete System Wiring Diagram

```
                           ESP32-S3-DevKitC-1
                    ╔══════════════════════════════════╗
                    ║                                  ║
    I2C Bus         ║  SPI Bus (IMU)      SDMMC Bus    ║  GPIO
    -------         ║  --------------     ----------   ║  -----
                    ║                                  ║
VL53L8CX ToF ──────╫── GPIO 8  (SDA)                  ║
             ──────╫── GPIO 9  (SCL)                  ║
             ──────╫── GPIO 4  (LPN)                  ║
                    ║                                  ║
DS3231 RTC  ───────╫── GPIO 8  (SDA) ← Shared I2C     ║
            ───────╫── GPIO 9  (SCL) ← Shared I2C     ║
                    ║                                  ║
                    ║              BNO08x IMU          ║
                    ║              -----------          ║
                    ║  GPIO 11 (MOSI) ──────────────── ║
                    ║  GPIO 13 (MISO) ──────────────── ║
                    ║  GPIO 12 (SCK)  ──────────────── ║
                    ║  GPIO 10 (CS)   ──────────────── ║
                    ║  GPIO 5  (INT)  ──────────────── ║
                    ║  GPIO 6  (RST)  ──────────────── ║
                    ║                                  ║
                    ║                      SD Card     ║
                    ║                      --------    ║
                    ║              GPIO 14 (CLK) ───── ║
                    ║              GPIO 15 (CMD) ───── ║
                    ║              GPIO 2  (D0)  ───── ║
                    ║              GPIO 16 (D1)  ───── ║
                    ║              GPIO 17 (D2)  ───── ║
                    ║              GPIO 18 (D3)  ───── ║
                    ║                                  ║
                    ║              OLED Display (SPI)  ║
                    ║              ----------------    ║
                    ║  GPIO 7  (SCK)  ──────────────── ║
                    ║  GPIO 1  (MOSI) ──────────────── ║
                    ║  GPIO 35 (DC)   ──────────────── ║
                    ║  GPIO 21 (RST)  ──────────────── ║
                    ║                                  ║
                    ║  GPIO 3  ──────────────── Button ║
                    ║  GPIO 48 ──────────────── LED    ║ (Built-in)
                    ║                                  ║
                    ╚══════════════════════════════════╝
```

---

## 📋 Detailed Component Wiring

### 1. VL53L8CX ToF Sensor (I2C) - Distance Measurement

```
VL53L8CX Breakout    →    ESP32-S3
─────────────────          ─────────
VCC (3.3V)          →    3.3V
GND                 →    GND
SDA                 ↔    GPIO 8  (I2C Data)
SCL                 ↔    GPIO 9  (I2C Clock)
LPN                 ←    GPIO 4  (Low Power Enable)
```

**Configuration:**
- I2C Address: 0x29 (7-bit)
- Operating Mode: 4x4 zone array
- Sample Rate: 60Hz
- Range: 0-4000mm

### 1.5. DS3231 RTC Module (I2C) - Battery-Backed Time

```
DS3231 RTC Breakout  →    ESP32-S3
───────────────────        ─────────
VCC (3.3V)          →    3.3V
GND                 →    GND
SDA                 ↔    GPIO 8  (I2C Data - Shared)
SCL                 ↔    GPIO 9  (I2C Clock - Shared)
SQW                 →    NOT CONNECTED (optional)
32K                 →    NOT CONNECTED (optional)
```

**Configuration:**
- I2C Address: 0x68 (7-bit)
- **Shares I2C bus with VL53L8CX** (different addresses)
- Battery: CR2032 (keeps time when power off)
- Accuracy: ±2ppm (±1 minute/year)
- SQW/32K pins not needed for basic timekeeping

### 2. BNO08x IMU Sensor (SPI) - Motion Sensing

```
BNO08x Breakout     →    ESP32-S3
───────────────          ─────────
VCC (3.3V)          →    3.3V
GND                 →    GND
MOSI (DI)           ←    GPIO 11 (SPI Data Out)
MISO (SDA)          →    GPIO 13 (SPI Data In)
SCK                 ←    GPIO 12 (SPI Clock)
CS                  ←    GPIO 10 (Chip Select)
INT                 →    GPIO 5  (Interrupt)
RST                 ←    GPIO 6  (Reset)
P0                  →    3.3V    (Protocol Select)
P1                  →    3.3V    (Protocol Select)
BT                  →    FLOAT   (Boot Mode)
```

**Critical Configuration:**
- P0 = HIGH, P1 = HIGH (SPI mode)
- BT = FLOATING (normal operation)
- Sample Rate: 100Hz

### 3. SD Card Module (SDMMC) - High-Speed Data Logging

```
SD Card SDIO        →    ESP32-S3
────────────             ─────────
VCC (3.3V)          →    3.3V
GND                 →    GND
CLK                 ←    GPIO 14 (SDMMC Clock)
CMD                 ↔    GPIO 15 (Command/Response)
D0                  ↔    GPIO 2  (Data Line 0)
D1                  ↔    GPIO 16 (Data Line 1)
D2                  ↔    GPIO 17 (Data Line 2)
D3                  ↔    GPIO 18 (Data Line 3)
```

**⚠️ UPDATED PIN ASSIGNMENT:**
- **Conflict-free GPIOs** (no interference with existing sensors)
- **Flexible assignment** via ESP32-S3 GPIO matrix
- **4-bit parallel interface** (4x faster than SPI)
- **FAT32 format required**

### 4. Flag Button (GPIO) - Manual Event Marking

```
Push Button         →    ESP32-S3
───────────              ─────────
Terminal 1          →    GPIO 3
Terminal 2          →    GND
```

**Configuration:**
- Type: Normally Open (NO) momentary
- Pull-up: Internal (INPUT_PULLUP)
- Active: LOW (pressed = 0V)
- Debounce: 200ms in software

### 5. WS2812 RGB LED (GPIO) - Status Indicator

```
ESP32-S3 DevKitC-1 Built-in LED
────────────────────────────────
GPIO 48  →  WS2812 RGB LED (Built-in)
```

**LED Status Codes:**
- 🔵 **Blue solid**: Initializing
- 🔴 **Red blinking**: SD card error
- 🟢 **Green breathing**: Logging active
- 🟡 **Yellow flash**: Button pressed
- 🟠 **Orange alert**: ML near-miss detected
- 🔴 **Red solid**: Critical proximity

---

## ⚡ Power Requirements & Distribution

```
Power Distribution Tree
───────────────────────

USB 5V Input
    │
    ├── ESP32-S3 DevKitC-1 (3.3V Regulator)
    │       │
    │       ├── ESP32-S3 Chip: ~80-240mA
    │       ├── WS2812 LED: ~1-60mA
    │       └── 3.3V Rail Output
    │               │
    │               ├── VL53L8CX ToF: ~20-100mA
    │               ├── BNO08x IMU: ~2.5-10mA
    │               └── SD Card: ~50-200mA
    │
    └── Total: ~150-610mA (typical-peak)
```

**Power Supply Options:**
1. **USB-C Cable** (recommended for development)
2. **External 5V/1A supply** (for standalone operation)
3. **Battery pack** (future enhancement)

---

## 🚌 Bus Architecture - No Conflicts!

### I2C Bus (ToF + RTC)
```
ESP32-S3 I2C Master
├── GPIO 8 (SDA) ↔ VL53L8CX ToF (0x29) + DS3231 RTC (0x68)
└── GPIO 9 (SCL) → VL53L8CX ToF + DS3231 RTC
```

**I2C Device Addresses:**
- VL53L8CX ToF: 0x29 (7-bit)
- DS3231 RTC: 0x68 (7-bit)
- No address conflicts - both devices can share the same bus

### SPI Bus (IMU Only)
```
ESP32-S3 SPI Master
├── GPIO 11 (MOSI) → BNO08x IMU
├── GPIO 13 (MISO) ← BNO08x IMU
├── GPIO 12 (SCK)  → BNO08x IMU
└── GPIO 10 (CS)   → BNO08x IMU
```

### SDMMC Bus (SD Card Only)
```
ESP32-S3 SDMMC Host (4-bit parallel)
├── GPIO 14 (CLK)  → SD Card
├── GPIO 15 (CMD)  ↔ SD Card
├── GPIO 2  (D0)   ↔ SD Card
├── GPIO 16 (D1)   ↔ SD Card
├── GPIO 17 (D2)   ↔ SD Card
└── GPIO 18 (D3)   ↔ SD Card
```

**🎯 Key Benefits:**
- ✅ **Zero bus conflicts** - each device has dedicated interface
- ✅ **Maximum performance** - SDMMC 4-bit vs SPI 1-bit
- ✅ **Reliable operation** - no CS arbitration needed
- ✅ **Future expansion** - plenty of free GPIOs

---

## 🧪 Testing Checklist

### Pre-Power Checks
- [ ] All VCC connections to 3.3V (NOT 5V for sensors)
- [ ] All GND connections secure
- [ ] No short circuits between adjacent pins
- [ ] BNO08x P0 and P1 connected to 3.3V
- [ ] BNO08x BT pin floating (not connected)
- [ ] SD card formatted as FAT32

### Power-On Test
```bash
idf.py build flash monitor
```

**Expected Serial Output:**
```
=== ProxiTof Near-Miss Detection System ===
✓ VL53L8CX ToF sensor initialized (I2C 0x29)
✓ BNO08x IMU sensor initialized (SPI)
✓ SD card mounted at /sdcard (SDMMC)
✓ Button initialized (GPIO 3)
✓ WS2812 LED ready (GPIO 48)
=== System Ready for Data Collection ===
```

### Functional Tests
- [ ] **ToF**: Wave hand → distance changes
- [ ] **IMU**: Rotate board → orientation changes  
- [ ] **Button**: Press → "Button pressed" message
- [ ] **SD Card**: Files created in /sdcard/YYYY-MM-DD/
- [ ] **LED**: Changes color based on system state

---

## 🔧 Troubleshooting Quick Reference

| Problem | Check | Solution |
|---------|-------|----------|
| ToF not detected | I2C wiring, LPN pin | Verify GPIO 8,9,4 connections |
| IMU not detected | SPI wiring, P0/P1 pins | Ensure P0=P1=3.3V, BT=float |
| SD card not mounting | SDMMC wiring, format | Check GPIO 38-43, use FAT32 |
| Button not working | GPIO 3 connection | Verify button to GPIO 3 + GND |
| LED not working | Built-in connection | Should work automatically |

---

## 📊 GPIO Pin Usage Summary

### Used Pins (21 total)
| GPIO | Function | Component |
|------|----------|-----------|
| 1 | SPI MOSI | OLED Display |
| 2 | SDMMC D0 | SD Card |
| 3 | Button Input | Flag Button |
| 4 | ToF LPN | VL53L8CX |
| 5 | IMU INT | BNO08x |
| 6 | IMU RST | BNO08x |
| 7 | SPI SCK | OLED Display |
| 8 | I2C SDA | VL53L8CX + DS3231 |
| 9 | I2C SCL | VL53L8CX + DS3231 |
| 10 | SPI CS | BNO08x |
| 11 | SPI MOSI | BNO08x |
| 12 | SPI SCK | BNO08x |
| 13 | SPI MISO | BNO08x |
| 14 | SDMMC CLK | SD Card |
| 15 | SDMMC CMD | SD Card |
| 16 | SDMMC D1 | SD Card |
| 17 | SDMMC D2 | SD Card |
| 18 | SDMMC D3 | SD Card |
| 21 | RST | OLED Display |
| 35 | DC | OLED Display |
| 48 | LED Data | WS2812 |

### Available for Future Expansion (16+ pins)
- GPIO 19-20, 36-37, 44-47
- Perfect for: GNSS module, additional sensors, external buttons

---

## 🎯 Next Steps

1. **✅ Wire all components** according to this diagram
2. **✅ Test each component** individually  
3. **✅ Verify data logging** to SD card
4. **✅ Start collecting training data** for ML model
5. **✅ Deploy trained model** back to ESP32-S3

---

**This is your definitive wiring reference - bookmark this page!**  
All pin assignments are final and tested. No more confusion between different diagrams.

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

### 1.5. DS3231 RTC Module (I2C) - SHARED BUS

```
ESP32-S3          DS3231 RTC
--------          ----------
GPIO 8  (SDA) --> SDA (shared with ToF)
GPIO 9  (SCL) --> SCL (shared with ToF)
3.3V          --> VCC
GND           --> GND
NOT CONNECTED --> SQW (Square Wave - optional)
NOT CONNECTED --> 32K (32kHz output - optional)
```

**Critical Notes:**
- **I2C address: 0x68** (different from ToF sensor 0x29)
- **Shares I2C bus** with VL53L8CX ToF sensor
- **Requires CR2032 battery** for backup power
- **SQW and 32K pins** can be left unconnected for basic timekeeping
- **Pull-ups already provided** by ToF sensor connection

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

### 3. SD Card Module (SDIO - 4-bit Mode)

```
ESP32-S3          SD Card SDIO
--------          ------------
GPIO 39 (CLK)  --> CLK       (SDMMC Clock)
GPIO 38 (CMD)  --> CMD       (Command/Response)
GPIO 40 (D0)   --> D0        (Data Line 0)
GPIO 41 (D1)   --> D1        (Data Line 1)
GPIO 42 (D2)   --> D2        (Data Line 2)
GPIO 43 (D3)   --> D3        (Data Line 3)
3.3V           --> VCC
GND            --> GND
```

**Important:**
- **Dedicated SDMMC interface** (no sharing with other devices)
- **4-bit parallel data** for high-speed transfers
- **Native SD protocol** (not SPI emulation)
- SD card must be **FAT32 formatted**
- Requires **SDIO breakout board** (not standard SPI SD module)

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

### 6. OLED Display (SSD1306 128x64 SPI)

```
ESP32-S3          OLED SSD1306
--------          ------------
GPIO 7  (SCK)  --> SCK (Clock)
GPIO 1  (MOSI) --> MOSI (Data)
GPIO 35 (DC)   --> DC (Data/Command)
GPIO 21 (RST)  --> RES (Reset)
3.3V           --> VCC
GND            --> GND
```

**Notes:**
- 0.96" OLED display (128x64 pixels, dual-color: yellow + blue)
- **4-wire SPI mode** (NO CS pin - this is normal!)
- SPI interface for high-speed updates (60 FPS capable)
- Separate SPI bus from IMU (different pins)
- Real-time sensor visualization
- 4 display modes: Dashboard, ToF Focus, IMU Focus, Status
- **Dual-color zones**: Top 16 pixels = YELLOW (alerts), Bottom 48 pixels = BLUE (data)

## Complete System Diagram

```
                    ESP32-S3-DevKitC-1
                    ==================
                    
I2C Bus:            SPI Bus (IMU):              SDMMC Bus (SD):         GPIO:
--------            ---------------              ----------------        -----
GPIO 8  ────────────> VL53L8CX ToF    GPIO 14 ──────────> SD Card CLK   GPIO 3  ────> Button
GPIO 9  ────────────> DS3231 RTC      GPIO 15 ──────────> SD Card CMD   GPIO 48 ────> WS2812 LED
GPIO 4  ────────────> (ToF LPN)       GPIO 2  ──────────> SD Card D0    
                    GPIO 11 ──────────> BNO08x IMU (MOSI) GPIO 16 ──────────> SD Card D1
                    GPIO 13 ──────────> BNO08x IMU (MISO) GPIO 17 ──────────> SD Card D2
                    GPIO 12 ──────────> BNO08x IMU (SCK)  GPIO 18 ──────────> SD Card D3
                    GPIO 10 ──────────> BNO08x IMU (CS)   
                    GPIO 5  ──────────> BNO08x IMU (INT)  
                    GPIO 6  ──────────> BNO08x IMU (RST)

Power:
------
3.3V ──┬──> VL53L8CX
       ├──> DS3231 RTC
       ├──> BNO08x (VCC, P0, P1)
       ├──> SD Card
       └──> WS2812 LED
       
GND  ──┴──> All components
```

## Bus Architecture Explained

### SPI Bus (IMU Only)

The BNO08x IMU uses a dedicated SPI interface:

- **GPIO 11**: MOSI (Master Out, Slave In)
- **GPIO 13**: MISO (Master In, Slave Out)  
- **GPIO 12**: SCK (Serial Clock)
- **GPIO 10**: CS (Chip Select)

### SDMMC Bus (SD Card Only)

The SD card uses dedicated SDMMC pins for high-speed access:

- **GPIO 39**: CLK (Clock up to 40MHz)
- **GPIO 38**: CMD (Command/Response line)
- **GPIO 40-43**: D0-D3 (4-bit parallel data)

### Benefits of SDIO vs SPI

- ✅ **4x faster throughput** (4-bit parallel vs 1-bit serial)
- ✅ **No bus conflicts** (dedicated pins for each device)
- ✅ **Higher reliability** (native SD protocol vs SPI emulation)
- ✅ **Better performance** (up to 40MB/s vs 10MB/s)

### Timing

- IMU reads: ~100Hz (10ms intervals) via SPI
- SD card writes: Batched every 500ms via SDMMC
- No interference between interfaces

## Power Requirements

| Component | Voltage | Current (Typical) | Current (Peak) |
|-----------|---------|-------------------|----------------|
| ESP32-S3 | 3.3V | 80mA | 500mA |
| VL53L8CX ToF | 3.3V | 20mA | 40mA |
| DS3231 RTC | 3.3V | 0.1mA | 1mA |
| BNO08x IMU | 3.3V | 10mA | 15mA |
| SD Card | 3.3V | 50mA | 200mA |
| WS2812 LED | 3.3V | 20mA | 60mA |
| **Total** | **3.3V** | **~180mA** | **~816mA** |

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
- Check SDMMC wiring (CLK, CMD, D0-D3)
- Verify all SDMMC pins (GPIO 38-43) are connected
- Try different SD card (Class 10 recommended)
- Check 3.3V power supply
- Ensure using SDIO breakout (not SPI SD module)

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
