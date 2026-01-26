# OLED Display - Final Resolution & Working Configuration

**Date**: January 26, 2026  
**Status**: ✅ **WORKING!** Display turns on successfully  
**Display Type**: SSD1306 128x64 OLED (4-wire SPI, 6-pin, NO CS)

---

## 🎉 SUCCESS - Display is Now Working!

After troubleshooting multiple issues, the OLED display is now successfully initialized and displaying content.

---

## Display Hardware Specifications

### Physical Pinout (From Datasheet)

```
     J1 Connector (CON7)
    ┌─────────────┐
    │             │  Pin 7: CS      (Not present on your module)
    │             │  Pin 6: D/C     ← Data/Command select
    │             │  Pin 5: RES     ← Reset
    │             │  Pin 4: D1      ← MOSI/Data (SPI data line)
    │             │  Pin 3: D0      ← SCK/Clock (SPI clock)
    │             │  Pin 2: VCC_IN  ← Power (3.3V)
    │             │  Pin 1: GND     ← Ground
    └─────────────┘
```

**Your Module:** 6-pin version (pins 1-6 only, no CS pin)

### Pin Mapping Clarification

**⚠️ CRITICAL - Confusing Pin Labels:**

| Pin# | Label on Display | Actual Function | Common Name | ESP32-S3 GPIO |
|------|------------------|-----------------|-------------|---------------|
| 1 | GND | Ground | GND | GND |
| 2 | VCC_IN | Power (3.3V) | VCC | 3.3V |
| 3 | **D0** | **SPI Clock** | **SCK** | **GPIO 7** |
| 4 | **D1** | **SPI Data** | **MOSI** | **GPIO 1** |
| 5 | RES | Reset | RST | GPIO 21 |
| 6 | D/C | Data/Command | DC | GPIO 35 |
| 7 | CS | Chip Select | CS | ❌ Not present |

**⚠️ WARNING:** 
- **D0 ≠ "Data 0"** → It's the **Clock (SCK)**!
- **D1 ≠ "Data 1"** → It's the **Data line (MOSI)**!
- These labels are confusing but standard for this display module

---

## SPI Mode: 4-Wire SPI (No CS)

### What is 4-Wire SPI?

**Standard SPI (with CS):**
```
MOSI (Data Out)
MISO (Data In)
SCK  (Clock)
CS   (Chip Select)
```

**4-Wire SPI for Display (no CS):**
```
MOSI (Data Out) ← D1 on display
SCK  (Clock)    ← D0 on display
DC   (Data/Cmd) ← D/C on display (replaces CS)
RST  (Reset)    ← RES on display
```

**Why no CS?**
- Display is the only device on this SPI bus (SPI3)
- DC pin serves similar purpose (selects data vs command mode)
- Simpler wiring, fewer pins needed

**Why no MISO?**
- Display is **write-only** (ESP32 → Display)
- Display never sends data back to ESP32
- MISO not needed for one-way communication

---

## Complete Wiring Diagram

```
ESP32-S3-DevKitC-1                    SSD1306 OLED Display
                                      (6-pin, 4-wire SPI)
┌─────────────────────┐              ┌──────────────────┐
│                     │              │                  │
│  GND            ────┼──────────────┼──→ Pin 1: GND    │
│  3.3V           ────┼──────────────┼──→ Pin 2: VCC_IN │
│                     │              │                  │
│  GPIO 7  (SCK)  ────┼──────────────┼──→ Pin 3: D0     │ ← Clock
│  GPIO 1  (MOSI) ────┼──────────────┼──→ Pin 4: D1     │ ← Data
│  GPIO 21 (RST)  ────┼──────────────┼──→ Pin 5: RES    │ ← Reset
│  GPIO 35 (DC)   ────┼──────────────┼──→ Pin 6: D/C    │ ← Data/Cmd
│                     │              │                  │
│  (SPI3 Bus)         │              │  Pin 7: CS       │ ← Not present
│                     │              │                  │
└─────────────────────┘              └──────────────────┘
```

**Power Requirements:**
- VCC = **3.3V** (NOT 5V! Will damage display)
- Current: ~15-30mA typical
- All 6 pins must be connected

---

## Issues Encountered & Solutions

### Issue 1: SPI Bus Conflict ❌ → ✅

**Problem:**
```
spi_bus_initialize(810): SPI bus already initialized.
assert failed: spi_master_init ssd1306_spi.c:65 (ret==ESP_OK)
```

**Root Cause:**
- BNO08x IMU initialized **SPI2** bus first
- OLED tried to initialize **SPI2** bus again
- ESP-IDF doesn't allow double initialization → **CRASH**

**Solution:**
- IMU uses **SPI2** (GPIO 11, 12, 13, 10)
- OLED uses **SPI3** (GPIO 1, 7, 21, 35)
- Modified `managed_components/ssd1306/ssd1306_spi.c`:

```c
// Before (caused conflict):
#if CONFIG_SPI2_HOST
#define HOST_ID SPI2_HOST
#elif CONFIG_SPI3_HOST
#define HOST_ID SPI3_HOST
#else
#define HOST_ID SPI2_HOST  // Default to SPI2
#endif

// After (fixed):
// Force SPI3 to avoid conflict with BNO08x IMU on SPI2
#define HOST_ID SPI3_HOST
```

**Result:** ✅ No more SPI bus conflicts!

---

### Issue 2: Wrong Parameter Order ❌ → ✅

**Problem:**
- Display still not initializing correctly
- Parameters passed in wrong order to `spi_master_init()`

**Library Function Signature:**
```c
void spi_master_init(SSD1306_t *dev, 
                     int16_t mosi,   // Parameter 1
                     int16_t sclk,   // Parameter 2
                     int16_t cs,     // Parameter 3 ← CS here!
                     int16_t dc,     // Parameter 4
                     int16_t reset); // Parameter 5
```

**Wrong Call (before):**
```c
spi_master_init(&dev, 
                OLED_PIN_MOSI,  // GPIO 1  ✓
                OLED_PIN_SCK,   // GPIO 7  ✓
                OLED_PIN_DC,    // GPIO 35 ✗ Wrong position!
                OLED_PIN_RST,   // GPIO 21 ✗ Wrong position!
                OLED_PIN_CS);   // -1      ✗ Wrong position!
```

**Correct Call (after):**
```c
spi_master_init(&dev, 
                OLED_PIN_MOSI,  // GPIO 1  ✓
                OLED_PIN_SCK,   // GPIO 7  ✓
                OLED_PIN_CS,    // -1      ✓ CS in correct position!
                OLED_PIN_DC,    // GPIO 35 ✓
                OLED_PIN_RST);  // GPIO 21 ✓
```

**Result:** ✅ Display initializes correctly!

---

### Issue 3: Component Not Found ❌ → ✅

**Problem:**
```
ERROR: Component "nopnop2002/esp_idf_ssd1306" not found
```

**Root Cause:**
- Library not in ESP Component Registry
- Tried to install from registry (doesn't exist there)

**Solution:**
- Clone directly from GitHub
- Point to correct subdirectory in repo

**File:** `components/oled_display/idf_component.yml`
```yaml
dependencies:
  ssd1306:
    git: https://github.com/nopnop2002/esp-idf-ssd1306.git
    path: components/ssd1306  # Library is in subdirectory!
```

**Result:** ✅ Library downloads and builds correctly!

---

## Final Working Configuration

### Code Configuration

**File:** `components/oled_display/oled_display.c`
```c
// GPIO Pin Definitions
#define OLED_PIN_SCK    GPIO_NUM_7   // D0 on display (Clock)
#define OLED_PIN_MOSI   GPIO_NUM_1   // D1 on display (Data/MOSI)
#define OLED_PIN_DC     GPIO_NUM_35  // D/C on display (Data/Command)
#define OLED_PIN_RST    GPIO_NUM_21  // RES on display (Reset)
#define OLED_PIN_CS     -1           // No CS pin on your display

// Correct initialization call:
spi_master_init(&dev, OLED_PIN_MOSI, OLED_PIN_SCK, 
                OLED_PIN_CS, OLED_PIN_DC, OLED_PIN_RST);
```

**File:** `managed_components/ssd1306/ssd1306_spi.c`
```c
// Force SPI3 to avoid conflict with BNO08x IMU on SPI2
#define HOST_ID SPI3_HOST
```

**File:** `components/oled_display/idf_component.yml`
```yaml
dependencies:
  ssd1306:
    git: https://github.com/nopnop2002/esp-idf-ssd1306.git
    path: components/ssd1306
```

---

## Serial Output (Success!)

```
I (5179) OLED_SPI: 
I (5179) OLED_SPI: ╔════════════════════════════════════════════╗
I (5199) OLED_SPI: ║   OLED Display Initialization (SPI)       ║
I (5199) OLED_SPI: ╚════════════════════════════════════════════╝
I (5219) OLED_SPI: 
I (5219) OLED_SPI: Display: 128x64 OLED SSD1306 (4-wire SPI)
I (5219) OLED_SPI: Dual-color: Yellow (top 16px) + Blue (bottom 48px)
I (5229) OLED_SPI: Library: nopnop2002/esp_idf_ssd1306
I (5229) OLED_SPI: 
I (5239) OLED_SPI: Initializing SPI interface...
I (5239) SSD1306: SPI HOST_ID=2  ← SPI3 (correct!)
I (5249) SSD1306: spi_bus_initialize=0  ← Success!
I (5259) SSD1306: spi_bus_add_device=0  ← Success!
I (5269) OLED_SPI: Initializing SSD1306 display...
I (5279) OLED_SPI: 
I (5279) OLED_SPI: ╔════════════════════════════════════════════╗
I (5289) OLED_SPI: ║   OLED Display Ready!                      ║
I (5299) OLED_SPI: ╚════════════════════════════════════════════╝
```

**✅ Display is ON and working!**

---

## Display Features

### Dual-Color Zones

**Yellow Zone (Top 16 pixels):**
- Rows 0-15 (Pages 0-1)
- Perfect for: Alerts, warnings, status messages
- High visibility, attention-grabbing

**Blue Zone (Bottom 48 pixels):**
- Rows 16-63 (Pages 2-7)
- Perfect for: Data visualization, sensor readings, graphs
- Calm, easy to read

### Display Modes (4 total)

1. **Dashboard** - Full overview (ToF grid + IMU + Stats)
2. **ToF Focus** - Large 4x4 distance grid with bar graphs
3. **IMU Focus** - Detailed orientation and acceleration
4. **Status** - System health and component status

**Mode Switching:** Press button on GPIO 20 to cycle through modes

---

## What's Working Now

✅ Display powers on  
✅ SPI3 bus initialized (no conflict with IMU)  
✅ SSD1306 controller initialized  
✅ Startup message displays: "ProxiTof v1.0" and "Initializing..."  
✅ Library functions available (text, graphics, bitmaps)  
✅ 60 FPS refresh rate capable  
✅ Mode switching button ready (GPIO 20)  

---

## What's Next (To Do)

### 1. Implement Dashboard Layout
- [ ] Yellow zone: Status bar (logging, time, GPS)
- [ ] Blue zone: ToF 4x4 grid with bar graphs
- [ ] Blue zone: IMU summary (Roll, Pitch, Yaw)
- [ ] Update at 60 Hz

### 2. Implement ToF Focus Mode
- [ ] Yellow zone: Min distance + approach rate
- [ ] Blue zone: Large 4x4 grid with numeric values
- [ ] All 16 zones clearly visible

### 3. Implement IMU Focus Mode
- [ ] Yellow zone: Jerk magnitude + status
- [ ] Blue zone: Detailed orientation (Roll, Pitch, Yaw)
- [ ] Blue zone: 3-axis acceleration

### 4. Implement Status Mode
- [ ] Yellow zone: System name + overall status
- [ ] Blue zone: Component status (ToF, IMU, SD, GPS, RTC)
- [ ] Blue zone: Buffer usage + event count

### 5. Bar Graph Visualization
- [ ] Implement `draw_bar_from_center()` function
- [ ] Closer object = wider bar (expands from center)
- [ ] Visual feedback for ToF grid

### 6. Real-Time Updates
- [ ] Integrate with sensor_manager data
- [ ] Update display at 60 Hz
- [ ] Smooth transitions between modes

---

## Key Learnings

### 1. SPI Bus Management
- ESP32-S3 has multiple SPI buses (SPI1, SPI2, SPI3)
- Each bus can only be initialized once
- Use separate buses for different devices to avoid conflicts

### 2. Display Pin Labels
- Pin labels on modules can be confusing (D0, D1)
- Always check datasheet for actual function
- D0 = Clock, D1 = Data (not "Data 0" and "Data 1")

### 3. Function Parameter Order
- Always check library function signatures
- Parameter order matters!
- Wrong order = wrong pins = doesn't work

### 4. Component Dependencies
- Not all libraries are in ESP Component Registry
- Can clone directly from GitHub
- Use `path:` to specify subdirectory in repo

### 5. 4-Wire SPI Mode
- CS pin not always needed
- DC pin serves similar purpose for displays
- MISO not needed for write-only devices

---

## Troubleshooting Guide

### Display Not Turning On?

**1. Check Power:**
```
✓ VCC = 3.3V (measure with multimeter)
✗ NOT 5V! (will damage display)
✓ GND connected
```

**2. Check All 6 Wires:**
```
✓ Pin 1: GND     → GND
✓ Pin 2: VCC_IN  → 3.3V
✓ Pin 3: D0      → GPIO 7  (Clock)
✓ Pin 4: D1      → GPIO 1  (Data)
✓ Pin 5: RES     → GPIO 21 (Reset)
✓ Pin 6: D/C     → GPIO 35 (Data/Command)
```

**3. Check Serial Output:**
```
Look for: "OLED Display Ready!"
If crash: Check SPI bus conflict
If timeout: Check wiring
```

**4. Verify SPI3 is Used:**
```
Serial should show: "SPI HOST_ID=2" (SPI3)
If shows "HOST_ID=1": Still using SPI2 (conflict!)
```

---

## Technical Specifications

**Display:**
- Controller: SSD1306
- Resolution: 128x64 pixels
- Colors: Dual-color (Yellow + Blue)
- Interface: 4-wire SPI (no CS)
- Refresh Rate: Up to 60 FPS

**SPI Configuration:**
- Bus: SPI3 (HOST_ID=2)
- Clock Speed: 10 MHz
- Mode: 0 (CPOL=0, CPHA=0)
- DMA: Enabled (SPI_DMA_CH_AUTO)

**Power:**
- Voltage: 3.3V (NOT 5V!)
- Current: 15-30mA typical
- Max Current: 50mA (all pixels on)

**Pins Used:**
- GPIO 1: MOSI (Data)
- GPIO 7: SCK (Clock)
- GPIO 21: RST (Reset)
- GPIO 35: DC (Data/Command)

---

## References

**Library:**
- GitHub: https://github.com/nopnop2002/esp-idf-ssd1306
- License: MIT
- Features: Text, graphics, bitmaps, scrolling

**Documentation:**
- SSD1306 Datasheet: Solomon Systech
- ESP-IDF SPI Master: https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/peripherals/spi_master.html

**Related Files:**
- `docs/OLED_DUAL_COLOR_GUIDE.md` - Display layout ideas
- `docs/OLED_MODE_BUTTON.md` - Mode switching guide
- `docs/complete-wiring-diagram.md` - Full system wiring

---

## Summary

**Problem:** OLED display not turning on  
**Root Causes:**
1. SPI bus conflict (both IMU and OLED using SPI2)
2. Wrong parameter order in function call
3. Component not in registry (needed GitHub clone)

**Solutions:**
1. Force OLED to use SPI3 (IMU stays on SPI2)
2. Fix parameter order: `spi_master_init(dev, mosi, sclk, cs, dc, rst)`
3. Clone from GitHub with correct path

**Result:** ✅ **Display is working!**

**Status:** Ready for dashboard implementation and real-time sensor visualization!

---

**Last Updated:** January 26, 2026  
**Tested On:** ESP32-S3-DevKitC-1 with ESP-IDF v5.5.2  
**Display Module:** SSD1306 128x64 OLED (6-pin, 4-wire SPI, no CS)

🎯 **Display is now operational and ready for sensor data visualization!**
