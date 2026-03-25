# ProxiTof Complete Wiring Reference - CONSOLIDATED

**Last Updated**: March 24, 2026  
**Board**: ESP32-S3-DevKitC-1 (N16R8 - 16MB Flash, 8MB PSRAM)  
**Status**: All Components Documented and Operational

---

## 🎯 Quick Reference Pin Map

| Component | Interface | GPIO Pins | Status | Notes |
|-----------|-----------|-----------|--------|-------|
| **VL53L8CX ToF** | I2C | SDA=8, SCL=9, LPN=36 | ✅ Working | 4x4 distance grid @ 60Hz |
| **BNO08x IMU** | SPI | MOSI=11, MISO=13, SCK=12, CS=10, INT=5, RST=6 | ✅ Working | 9-DOF @ 100Hz |
| **SD Card** | **SDMMC (4-bit)** | CLK=14, CMD=15, D0=2, D1=16, D2=17, D3=18 | ✅ Working | **Conflict-free pins** |
| **DS3231 RTC** | I2C | SDA=8, SCL=9 (shared with ToF) | ✅ Working | Battery-backed time |
| **OLED Display** | SPI | SCK=7, MOSI=1, DC=35, RST=21 (no CS) | ✅ Working | 128x64 SSD1306 (4-wire SPI, 6-pin module) |
| **Neo-M9N GPS** | UART | **TX=38, RX=47** | ✅ Working | Location @ 5Hz (configurable) |
| **Buzzer** | PWM | GPIO 45 | ✅ Ready | Near-miss audio alert |
| **Potentiometer** | ADC | **GPIO 4 (ADC1_CH3)** | ✅ Ready | Volume control (0-3.3V) **MOVED from GPIO 46** |
| **Flag Button** | GPIO | GPIO 3 (with pull-up) | ✅ Working | Manual event flagging |
| **Mode Button** | GPIO | GPIO 20 (with pull-up) | ✅ Working | OLED display mode cycling |
| **WS2812 LED** | GPIO | GPIO 48 | ✅ Working | Status indicator |

---

## 🔌 Complete System Wiring Diagram

```
                           ESP32-S3-DevKitC-1
                    ╔══════════════════════════════════╗
                    ║                                  ║
    I2C Bus         ║  SPI Bus (IMU)      SDMMC Bus    ║  GPIO/UART/PWM
    -------         ║  --------------     ----------   ║  --------------
                    ║                                  ║
VL53L8CX ToF ──────╫── GPIO 8  (SDA)                  ║
             ──────╫── GPIO 9  (SCL)                  ║
             ──────╫── GPIO 36 (LPN)                  ║
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
                    ║  GPIO 7  (SCK/SCL) ───────────── ║
                    ║  GPIO 1  (MOSI/SDA) ──────────── ║
                    ║  GPIO 35 (DC)   ──────────────── ║
                    ║  GPIO 21 (RST)  ──────────────── ║
                    ║  (No CS pin on this module)       ║
                    ║                                  ║
                    ║              Neo-M9N GPS (UART)  ║
                    ║              ----------------    ║
                    ║  GPIO 38 (TX)   ──────────────── ║
                    ║  GPIO 47 (RX)   ──────────────── ║
                    ║                                  ║
                    ║  GPIO 45 ──────────────── Buzzer ║ (PWM)
                    ║  GPIO 4  ──────────────── Pot    ║ (ADC1 - Volume Control)
                    ║  GPIO 3  ──────────────── Button ║ (Flag Event)
                    ║  GPIO 20 ──────────────── Button ║ (Mode Switch)
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
LPN                 ←    GPIO 36 (Low Power Enable)
```

**⚠️ IMPORTANT - LPN Pin:**
- **LPN connected to GPIO 36** (free pin, perfect for simple output control)
- **GPIO 4 now used for potentiometer** (ADC1_CH3 - WiFi compatible)
- **GPIO 36**: Free pin, no conflicts with other peripherals

**Configuration:**
- I2C Address: 0x29 (7-bit)
- Operating Mode: 4x4 zone array
- Sample Rate: 60Hz
- Range: 0-4000mm
- Power Mode: Software controlled via LPN pin

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

**✅ VERIFIED PIN ASSIGNMENT:**
- **Conflict-free GPIOs** (no interference with GPS or other sensors)
- **GPIO 38/47 are FREE** for GPS UART
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

### 4.5. Mode Switch Button (GPIO) - OLED Display Mode Cycling

```
Push Button         →    ESP32-S3
───────────              ─────────
Terminal 1          →    GPIO 20
Terminal 2          →    GND
```

**Configuration:**
- Type: Normally Open (NO) momentary
- Pull-up: Internal (INPUT_PULLUP)
- Active: LOW (pressed = 0V)
- Debounce: 200ms in software
- Function: Cycles through OLED display modes (Dashboard → ToF Focus → IMU Focus → Status → Dashboard)

**Display Modes:**
1. **Dashboard** - Full overview (ToF grid + IMU + Stats)
2. **ToF Focus** - Large 4x4 distance grid with bar graphs
3. **IMU Focus** - Detailed orientation and acceleration
4. **Status** - System health and component status

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
| ToF not detected | I2C wiring, LPN pin | Verify GPIO 8,9,36 connections |
| IMU not detected | SPI wiring, P0/P1 pins | Ensure P0=P1=3.3V, BT=float |
| SD card not mounting | SDMMC wiring, format | Check GPIO 38-43, use FAT32 |
| Button not working | GPIO 3 connection | Verify button to GPIO 3 + GND |
| LED not working | Built-in connection | Should work automatically |

---

## 📊 GPIO Pin Usage Summary

### Used Pins (27 total)
| GPIO | Function | Component |
|------|----------|-----------|
| 1 | SPI MOSI (SDA on module) | OLED Display |
| 2 | SDMMC D0 | SD Card |
| 3 | Button Input | Flag Button |
| 4 | **ADC (ADC1_CH3)** | **Potentiometer (Volume Control)** |
| 5 | IMU INT | BNO08x |
| 6 | IMU RST | BNO08x |
| 7 | SPI SCK (SCL on module) | OLED Display |
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
| 20 | Button Input | Mode Switch Button |
| 21 | RST | OLED Display |
| 35 | DC | OLED Display |
| **36** | **ToF LPN** | **VL53L8CX (moved from GPIO 4)** |
| 38 | UART TX | Neo-M9N GPS |
| 45 | PWM | Buzzer |
| 47 | UART RX | Neo-M9N GPS |
| 48 | LED Data | WS2812 |

### Available for Future Expansion (8+ pins)
- GPIO 0, 37, 39-42, 46
- Perfect for: Additional sensors, external buttons, displays

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
GPIO 36 (LPN) --> LPN (Low Power/Reset)
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

### 3. SD Card Module (SDMMC - 4-bit Mode)

```
ESP32-S3          SD Card SDIO
--------          ------------
GPIO 14 (CLK)  --> CLK       (SDMMC Clock)
GPIO 15 (CMD)  --> CMD       (Command/Response)
GPIO 2  (D0)   --> D0        (Data Line 0)
GPIO 16 (D1)   --> D1        (Data Line 1)
GPIO 17 (D2)   --> D2        (Data Line 2)
GPIO 18 (D3)   --> D3        (Data Line 3)
3.3V           --> VCC
GND            --> GND
```

**Important:**
- **Dedicated SDMMC interface** (no sharing with other devices)
- **4-bit parallel data** for high-speed transfers
- **Native SD protocol** (not SPI emulation)
- SD card must be **FAT32 formatted**
- Requires **SDIO breakout board** (not standard SPI SD module)
- **GPIO 38/47 are FREE** for GPS (not used by SD card)

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

### 6. OLED Display (SSD1306 128x64 SPI — 6-Pin Module)

```
ESP32-S3          OLED SSD1306 (6-pin module)
--------          ----------------------------
GND            --> Pin 1: GND
3.3V           --> Pin 2: VCC (⚠️ 3.3V NOT 5V!)
GPIO 7  (SCK)  --> Pin 3: SCL (SPI Clock = D0)
GPIO 1  (MOSI) --> Pin 4: SDA (SPI Data  = D1)
GPIO 21 (RST)  --> Pin 5: RES (Reset)
GPIO 35 (DC)   --> Pin 6: DC  (Data/Command)
```

**⚠️ IMPORTANT — Module Pin Labels vs Actual Function:**

This 0.96" yellow-blue dual-color SSD1306 module has 6 pins labeled `GND VCC SCL SDA RES DC` on the PCB. The labels "SCL" and "SDA" look like I2C, but this module is configured for 4-wire SPI mode (confirmed by onboard resistor configuration R3=1, R4=1). The actual mapping is:

| Module Label | SSD1306 Chip Pin | Actual SPI Function |
|--------------|------------------|---------------------|
| SCL | D0 | SPI Clock (SCK) |
| SDA | D1 | SPI Data (MOSI) |

- There is **no CS pin** on this module — the code sets CS to -1
- **No MISO needed** — display is write-only
- **Do not confuse SCL/SDA with I2C** — this is a pure SPI connection

**Notes:**
- 6-pin SPI OLED module (not I2C)
- VCC must be 3.3V (NOT 5V)
- Uses SPI3 bus (separate from IMU on SPI2)
- Dual-color zones: Top 16 pixels = YELLOW (alerts), Bottom 48 pixels = BLUE (data)

### 7. Neo-M9N GPS Module (UART) - GNSS Positioning

```
ESP32-S3          Neo-M9N GPS Breakout Board
--------          --------------------------
GPIO 38 (TX)   --> RX (GPS receives commands from ESP32)
GPIO 47 (RX)   <-- TX (GPS sends NMEA data to ESP32)
5V or 3.3V     --> VCC/VIN (see power note below)
GND            --> GND
```

**Configuration:**
- **UART**: UART1 (not console UART0)
- **Baud Rate**: 38400 (Neo-M9N default)
- **Update Rate**: 1 Hz default (can be increased to 5-25 Hz via UBX commands)
- **Dynamic Model**: Configurable via config file (Bike/Pedestrian/Portable/Automotive)
- **Multi-GNSS**: GPS + GLONASS + Galileo + BeiDou (automatic)

**⚠️ POWER INPUT - IMPORTANT:**
Your GPS breakout board determines the voltage:

**Option A: Board with 5V regulator (SparkFun, most breakouts)**
```
ESP32 5V → GPS "5V" or "VIN" pin
```
- Board has onboard 3.3V regulator
- Accepts 3.3V-6V input
- Regulates down to 3.3V for Neo-M9N chip
- **Use this if your board has a "5V" pin** ✅

**Option B: Board without regulator (bare module)**
```
ESP32 3.3V → GPS "VCC" or "3.3V" pin
```
- Direct connection to Neo-M9N chip
- Must be exactly 3.3V
- No onboard regulator
- Less common for breakout boards

**How to tell:**
- If board has **two power pins** (5V and 3.3V) → Use 5V pin
- If board has **one power pin** labeled "VIN" or "5V" → Use 5V
- If board has **one power pin** labeled "VCC" or "3.3V" → Use 3.3V
- Check your board's silkscreen or documentation

**Logic Levels (Always 3.3V):**
- TX/RX pins are **always 3.3V logic** regardless of power input
- Safe to connect directly to ESP32-S3 GPIO pins
- No level shifters needed

**✅ VERIFIED PIN ASSIGNMENT:**
- **GPIO 38/47 are FREE** (not used by SD card or other peripherals)
- **GPIO 43/44 are UART0** (console) - DO NOT USE for GPS
- **Crossover wiring is CORRECT** (TX→RX, RX→TX)

**⚠️ UART Wiring Explanation (TX/RX Crossover)**:
- **This is CORRECT!** UART connections are always crossover
- ESP32 **TX** (transmit) connects to GPS **RX** (receive)
- ESP32 **RX** (receive) connects to GPS **TX** (transmit)
- Think of it as: "My transmitter talks to your receiver"
- **DO NOT connect TX to TX or RX to RX** - that won't work!

**GPS Update Rate Recommendations:**
- **1 Hz (default)**: Best for battery life, adequate for walking/cycling
- **5 Hz**: Good balance for cycling, smooth position updates
- **10 Hz**: High precision for fast movement, higher power consumption
- **25 Hz**: Maximum rate, only needed for very fast movement or research

**Note**: Current implementation uses default 1 Hz. To increase update rate, send UBX CFG-RATE command to GPS module (future enhancement).

**Visual Explanation**:
```
ESP32-S3                    Neo-M9N GPS
--------                    -----------
GPIO 38 (TX) ----data----> RX (receives ESP32's commands)
GPIO 47 (RX) <---data----- TX (sends NMEA sentences to ESP32)
```

**Neo-M9N Specifications** (from u-blox datasheet):

**Hardware:**
- **Power Input**: 3.3V-5V (depends on breakout board)
  - **With regulator**: 5V input → onboard regulator → 3.3V to chip
  - **Without regulator**: 3.3V direct to chip
  - **Logic levels**: Always 3.3V (TX/RX safe for ESP32)
- **Current**: 25 mA typical, 67 mA max (at chip level)
- **Antenna**: Ceramic patch (integrated) or external via U.FL/SMA
- **LED**: Power indicator (should be lit when powered)
- **Battery**: Backup battery for hot start (charges when powered)

**UART Interface:**
- **Default Baud Rate**: 38400 bps (8N1)
- **Supported Rates**: 4800, 9600, 19200, 38400, 57600, 115200, 230400
- **Protocol**: NMEA 0183 (ASCII text)
- **Data Format**: 8 data bits, no parity, 1 stop bit

**GNSS Performance:**
- **Constellations**: GPS, GLONASS, Galileo, BeiDou (concurrent)
- **Channels**: 92 tracking channels
- **Update Rate**: 1 Hz (default), up to 25 Hz max
- **Horizontal Accuracy**: 2.0m CEP (multi-GNSS)
- **Velocity Accuracy**: 0.05 m/s
- **Heading Accuracy**: 0.3 degrees
- **Sensitivity**: 
  - Tracking: -167 dBm
  - Reacquisition: -160 dBm
  - Cold start: -148 dBm
  - Hot start: -159 dBm

**Time to First Fix:**
- **Cold Start**: 24-29 seconds (no almanac, no position)
- **Warm Start**: 2-3 seconds (almanac available)
- **Hot Start**: 1-2 seconds (recent fix, backup battery active)
- **Backup Battery**: Maintains hot start for 4 hours after power-off

**NMEA Sentences Parsed:**
- **$GPGGA**: Position fix data (lat, lon, alt, satellites, HDOP, fix quality)
- **$GPRMC**: Recommended minimum (date, time, speed, heading, status)
- **$GPVTG**: Track made good and ground speed
- **$GPGSA**: DOP and active satellites
- **$GPGSV**: Satellites in view

**Example NMEA Output:**
```
$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47
       ^^^^^^ ^^^^^^^^^ ^ ^^^^^^^^^^ ^ ^^ ^^^ ^^^^^
       Time   Latitude  N Longitude  E Sat HDOP Alt
```

**What GPS Provides:**
- **Location**: Latitude/Longitude (decimal degrees)
- **Altitude**: Meters above sea level
- **Speed**: km/h or m/s (ground speed)
- **Heading**: Degrees (0-360, course over ground)
- **Time**: UTC from satellites (accurate to microseconds)
- **Date**: Year, month, day from satellites
- **Fix Quality**: No fix, GPS, DGPS, RTK, etc.
- **Satellites**: Number of satellites used (4+ needed for 3D fix)
- **HDOP**: Horizontal dilution of precision (<2.0 = good)

**Why GPS for Near-Miss Detection:**
- **Location Context**: Where did near-miss occur? (geo-tagging)
- **Speed Validation**: Confirm IMU-derived speed with GPS speed
- **Route Tracking**: Map near-miss hotspots and dangerous areas
- **Time Synchronization**: Accurate timestamps from satellites
- **Data Enrichment**: Add location metadata to CSV logs
- **Post-Analysis**: Heatmaps, route safety analysis, location clustering

**Testing GPS:**
```
Expected Console Output:
I (5000) GPS: 📡 GPS UART active: 82 bytes received (total: 410)
I (5000) GPS: 0x3fceb7c0   24 47 50 47 47 41 2c 31  32 33 35 31 39 2c ...
                            $  G  P  G  G  A  ,  1   2  3  5  1  9  ,

I (30000) GPS: ✓ First GPS fix acquired in 26 seconds
```

**GPS Mode (Mode 4) Display:**
- Latitude/Longitude (real coordinates, not 0.000000)
- Satellites: > 4 (more is better)
- Fix status: "3D FIX" or "2D FIX" (not "SEARCHING")
- HDOP: < 2.0 (good accuracy)
- Speed: km/h and m/s
- Heading: degrees
- Altitude: meters

**Troubleshooting:**
- **No data received**: Check wiring (TX→RX crossover), verify 3.3V power, check LED
- **Garbled data**: Wrong baud rate - try 9600, 38400, or 115200
- **No GPS fix**: Move outdoors (needs sky view), wait 1-5 minutes for cold start
- **Slow fix**: First fix takes 26-60 seconds, subsequent fixes are 1-2 seconds
- **Indoor testing**: GPS may not get fix indoors, but should still send NMEA sentences

**Critical Notes:**
- **GPIO 43/44 are UART0 (console)** - DO NOT USE for GPS!
- **Use GPIO 38/47** - Free pins, no conflicts
- **3.3V power** - Neo-M9N operates at 3.3V (not 5V)
- **Crossover wiring** - TX→RX, RX→TX (correct!)
- **Outdoor testing** - GPS needs clear sky view for fix
- **Backup battery** - Charges slowly, provides hot start for 4 hours

### 8. Buzzer (PWM Audio Alert)

```
ESP32-S3          Buzzer (Active/Passive)
--------          -----------------------
GPIO 45 (PWM)  --> IN (Signal)
5V or 3.3V     --> VCC (check buzzer specs)
GND            --> GND
```

**Buzzer Types:**

**Option A: Active Buzzer (Simpler)**
```
- Has built-in oscillator
- Just needs ON/OFF signal
- Fixed frequency (usually 2-4 kHz)
- Louder, but less control
- Wiring: GPIO 45 → IN, VCC → 5V, GND → GND
```

**Option B: Passive Buzzer (More Control)**
```
- Requires PWM signal
- Variable frequency (100 Hz - 10 kHz)
- Can create tones, melodies
- Volume control via PWM duty cycle
- Wiring: GPIO 45 → IN, VCC → 5V, GND → GND
```

**Alert Patterns:**
```
Near-Miss Detected:
  - Frequency: 2000 Hz (high-pitched)
  - Pattern: 3 short beeps (200ms on, 100ms off)
  - Volume: Adjustable via potentiometer

Critical Proximity:
  - Frequency: 1000 Hz (lower pitch)
  - Pattern: Continuous tone
  - Volume: Maximum (override potentiometer)

Button Press:
  - Frequency: 3000 Hz
  - Pattern: Single short beep (100ms)
  - Volume: Medium
```

**PWM Configuration:**
```c
// LEDC (LED Control) peripheral for PWM
ledc_timer_config_t timer = {
    .speed_mode = LEDC_LOW_SPEED_MODE,
    .duty_resolution = LEDC_TIMER_10_BIT,
    .timer_num = LEDC_TIMER_0,
    .freq_hz = 2000,  // 2 kHz tone
    .clk_cfg = LEDC_AUTO_CLK
};

ledc_channel_config_t channel = {
    .gpio_num = 45,
    .speed_mode = LEDC_LOW_SPEED_MODE,
    .channel = LEDC_CHANNEL_0,
    .timer_sel = LEDC_TIMER_0,
    .duty = 512,  // 50% duty cycle
    .hpoint = 0
};
```

### 9. Potentiometer (Volume Control) - OPTIONAL

```
ESP32-S3          Potentiometer (10kΩ)
--------          --------------------
GPIO 4 (ADC)   --> Wiper (middle pin)
3.3V           --> One end
GND            --> Other end
```

**How It Works:**
```
Potentiometer creates voltage divider:
- Fully CCW: 0V → Volume = 0% (silent)
- Middle: 1.65V → Volume = 50%
- Fully CW: 3.3V → Volume = 100% (max)

ADC reads voltage (0-3.3V) → Convert to duty cycle (0-100%)
```

**ADC Configuration:**
```c
// Configure ADC
adc1_config_width(ADC_WIDTH_BIT_12);  // 12-bit resolution (0-4095)
adc1_config_channel_atten(ADC1_CHANNEL_3, ADC_ATTEN_DB_11);  // 0-3.3V range

// Read volume
int adc_value = adc1_get_raw(ADC1_CHANNEL_3);  // 0-4095
int volume_percent = (adc_value * 100) / 4095;  // 0-100%
int pwm_duty = (volume_percent * 1023) / 100;  // 0-1023 (10-bit PWM)

// Set buzzer volume
ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, pwm_duty);
ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
```

**Alternative: Digital Volume Control**
```
Instead of potentiometer, use buttons:
- GPIO 46: Volume Up
- GPIO 47: Volume Down
- Store volume in NVS (non-volatile storage)
- Display volume on OLED
```

**⚠️ IMPORTANT - Potentiometer Moved to GPIO 4:**
- **OLD**: GPIO 46 (ADC2_CH5) - conflicts with WiFi
- **NEW**: GPIO 4 (ADC1_CH3) - works with WiFi enabled!
- **ToF LPN**: No longer connected (sensor works without it)
- **ADC1 vs ADC2**: ADC1 can be used while WiFi is active

**Correct Potentiometer Wiring:**
```
ESP32-S3          Potentiometer (10kΩ)
--------          --------------------
GPIO 4 (ADC)   --> Wiper (middle pin)  ✅ CORRECT (ADC1_CH3)
3.3V           --> One end
GND            --> Other end
```

**Why GPIO 4 (ADC1)?**
- GPIO 4 = ADC1_CH3 (WiFi compatible!)
- GPIO 46 = ADC2_CH5 (conflicts with WiFi)
- GPIO 4 supports 12-bit ADC (0-4095 range)
- Perfect for 0-3.3V potentiometer reading
- **ToF sensor works without LPN pin** (always-on mode)

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

- **GPIO 14**: CLK (Clock up to 40MHz)
- **GPIO 15**: CMD (Command/Response line)
- **GPIO 2, 16, 17, 18**: D0-D3 (4-bit parallel data)

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
| OLED Display | 3.3V | 15mA | 30mA |
| Neo-M9N GPS | 3.3V | 25mA | 67mA |
| Buzzer | 5V | 30mA | 50mA |
| WS2812 LED | 3.3V | 20mA | 60mA |
| **Total 3.3V** | **3.3V** | **~220mA** | **~913mA** |
| **Total 5V** | **5V** | **~30mA** | **~50mA** |

**Recommendation:** 
- Use USB power (5V input → 3.3V regulator on DevKit)
- ESP32-S3 DevKit can provide both 3.3V and 5V
- Total power: ~250mA typical, ~963mA peak
- USB 2.0 provides 500mA, USB 3.0 provides 900mA
- **Use USB 3.0 port or powered USB hub for reliable operation**

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
- Verify all SDMMC pins (GPIO 14, 15, 2, 16, 17, 18) are connected
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

**Document Version:** 1.1  
**Last Updated:** March 24, 2026  
**Hardware:** ESP32-S3-DevKitC-1, VL53L8CX, BNO08x, SD Card Module
