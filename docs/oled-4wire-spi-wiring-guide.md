# OLED Display (SSD1306) - 4-Wire SPI Wiring Guide

## Your Display Configuration

You have an **SSD1306 OLED display (128x64)** in **4-wire SPI mode** with **dual-color zones**.

### Display Color Zones (Hardware-Based)

Your display has **two fixed color zones**:

```
┌─────────────────────────────┐
│                             │ ← YELLOW/AMBER Zone
│    Top 16 pixels (rows 0-15)│   (Hardware fixed)
│         YELLOW              │   Perfect for: Alerts, Status, Warnings
├─────────────────────────────┤
│                             │
│                             │
│   Bottom 48 pixels          │ ← BLUE Zone
│      (rows 16-63)           │   (Hardware fixed)
│         BLUE                │   Perfect for: Main data, Readings
│                             │
│                             │
└─────────────────────────────┘
```

**Important:** The color split is **hardware-based** (different color LEDs soldered on the display). You cannot change which pixels are yellow vs blue, but the code strategically uses colors for better visual hierarchy!

### Smart Color Usage

The firmware uses the dual colors intelligently:

**YELLOW Zone (Attention-grabbing):**
- ✅ System title and status
- ✅ FPS counter
- ✅ **Danger warnings** (distance < 10cm)
- ✅ **Motion alerts** (high acceleration)
- ✅ **Buffer critical** warnings

**BLUE Zone (Main content):**
- ✅ ToF sensor grid (4x4 distances)
- ✅ IMU readings (roll, pitch, yaw)
- ✅ Statistics and measurements
- ✅ Detailed data display

### Display Pins Explained

Your display has **6 pins** (not 7-wire SPI):

```
Pin Label    Full Name           Function                    ESP32-S3 Connection
─────────    ─────────           ────────                    ───────────────────
GND          Ground              Power ground                GND
VCC          Power               3.3V power supply           3.3V
SCK (CLK)    Serial Clock        SPI clock signal            GPIO 7
SDA (MOSI)   Serial Data         SPI data from ESP32         GPIO 1
RES (RST)    Reset               Hardware reset (active LOW) GPIO 21
DC (D/C)     Data/Command        Command=LOW, Data=HIGH      GPIO 35
```

**Important Notes:**
- ✅ **NO CS (Chip Select) pin** - This is normal for 4-wire SPI displays
- ✅ **SDA pin = MOSI** (Master Out, Slave In) - data flows from ESP32 to display
- ✅ **4-wire SPI** is faster than I2C and perfect for 60 FPS updates

## Complete Wiring Diagram

```
┌─────────────────────────────────────────────────────────┐
│                                                         │
│              SSD1306 OLED Display (128x64)              │
│                    4-Wire SPI Mode                      │
│                                                         │
│  ┌───┬───┬───┬───┬───┬───┐                             │
│  │GND│VCC│SCK│SDA│RES│DC │                             │
│  └─┬─┴─┬─┴─┬─┴─┬─┴─┬─┴─┬─┘                             │
│    │   │   │   │   │   │                               │
│    │   │   │   │   │   │                               │
└────┼───┼───┼───┼───┼───┼───────────────────────────────┘
     │   │   │   │   │   │
     │   │   │   │   │   │
     │   │   │   │   │   └──────────┐
     │   │   │   │   │              │
     │   │   │   │   └──────┐       │
     │   │   │   │          │       │
     │   │   │   └──────┐   │       │
     │   │   │          │   │       │
     │   │   └──────┐   │   │       │
     │   │          │   │   │       │
     │   └──────┐   │   │   │       │
     │          │   │   │   │       │
     └──────┐   │   │   │   │       │
            │   │   │   │   │       │
     ┌──────┴───┴───┴───┴───┴───┴───┴───────────────────┐
     │                                                   │
     │              ESP32-S3-DevKitC-1                   │
     │                                                   │
     │  GND   3.3V  GPIO7  GPIO1  GPIO21  GPIO19        │
     │   │     │      │      │       │       │          │
     └───┴─────┴──────┴──────┴───────┴───────┴──────────┘
```

## Step-by-Step Wiring Instructions

### 1. Power Connections (CRITICAL - DO THIS FIRST)

```
Display Pin    →    ESP32-S3 Pin    Notes
───────────         ────────────    ─────
GND            →    GND             Ground connection
VCC            →    3.3V            ⚠️ USE 3.3V, NOT 5V!
```

**⚠️ WARNING:** Most SSD1306 displays are **3.3V only**. Using 5V may damage the display!

### 2. SPI Signal Connections

```
Display Pin    →    ESP32-S3 Pin    Function
───────────         ────────────    ────────
SCK (CLK)      →    GPIO 7          SPI Clock
SDA (MOSI)     →    GPIO 1          SPI Data Out
```

### 3. Control Signal Connections

```
Display Pin    →    ESP32-S3 Pin    Function
───────────         ────────────    ────────
RES (RST)      →    GPIO 21         Reset (active LOW)
DC (D/C)       →    GPIO 35         Data/Command select
```

## Physical Connection Tips

### Using Jumper Wires

1. **Use female-to-female jumper wires** if your display has pins
2. **Use female-to-male jumper wires** if your display has holes
3. **Keep wires short** (< 15cm) to reduce noise
4. **Twist SPI wires together** (SCK + SDA) to reduce interference

### Using a Breadboard

```
ESP32-S3 Side:                    Display Side:
─────────────                     ─────────────

GND  ────────────────────────────  GND
3.3V ────────────────────────────  VCC
GPIO 7  ─────────────────────────  SCK
GPIO 1  ─────────────────────────  SDA
GPIO 21 ─────────────────────────  RES
GPIO 19 ─────────────────────────  DC
```

## Why 4-Wire SPI (No CS Pin)?

### What is CS (Chip Select)?

CS (Chip Select) is used when **multiple SPI devices** share the same bus. The master (ESP32) pulls CS LOW to select which device to talk to.

### Why Your Display Doesn't Have CS

Your display is designed for **dedicated SPI connection**:
- ✅ **Simpler wiring** - fewer pins needed
- ✅ **Always selected** - display is always listening
- ✅ **Lower cost** - fewer components on display PCB
- ✅ **Perfect for single display** - no bus sharing needed

### How It Works Without CS

The display is **permanently selected** (CS is tied LOW internally on the display PCB). This means:
- Display always listens to SPI bus
- No need to toggle CS pin in software
- Slightly faster communication (no CS overhead)

## Verification Checklist

Before powering on, verify:

- [ ] **GND connected** to ESP32 GND
- [ ] **VCC connected** to ESP32 **3.3V** (NOT 5V!)
- [ ] **SCK connected** to GPIO 7
- [ ] **SDA connected** to GPIO 1
- [ ] **RES connected** to GPIO 21
- [ ] **DC connected** to GPIO 35
- [ ] **No loose connections**
- [ ] **No short circuits** between adjacent pins

## Testing the Display

### 1. Build and Flash

```bash
idf.py build flash monitor
```

### 2. Expected Serial Output

```
╔════════════════════════════════════════════╗
║   OLED Display Initialization (SSD1306)   ║
╚════════════════════════════════════════════╝

Display: 128x64 OLED (4-wire SPI mode)

Wiring Configuration:
  Display Pin  →  ESP32-S3 Pin
  ───────────     ────────────
  GND          →  GND
  VCC          →  3.3V
  SCK          →  GPIO 7
  SDA (MOSI)   →  GPIO 1
  RES (Reset)  →  GPIO 21
  DC (D/C)     →  GPIO 35

Note: This is 4-wire SPI (no CS pin needed)

✓ SPI bus initialized
✓ SSD1306 device created
✓ Display initialized
✓ OLED display ready!
```

### 3. What You Should See on Display

1. **Startup screen** (2 seconds):
   ```
   ProxiTof
   Starting...
   ```

2. **Dashboard mode** (real-time):
   ```
   ProxiTof              60FPS
   ─────────────────────────────
   ToF Grid:    IMU:
   45  32  28  35    R: +12.3°
   52  18  15  42    P: -5.2°
   48  12  10  38    Y: +180.0°
   55  25  22  40
   
   Min:10cm          Buf:45%
   ```

## Troubleshooting

### Display Shows Nothing (Blank Screen)

**Check:**
1. ✅ Power connections (GND, VCC to 3.3V)
2. ✅ VCC is connected to **3.3V** (not 5V, not VIN)
3. ✅ Display contrast (some displays ship with low contrast)
4. ✅ Try different display if available

**Test:**
```bash
# Check serial output for initialization errors
idf.py monitor
```

### Display Shows Garbage/Random Pixels

**Causes:**
- ❌ Wrong SPI pins (SCK or SDA swapped)
- ❌ Loose connections
- ❌ Wires too long (> 30cm)
- ❌ Electromagnetic interference

**Solutions:**
1. Double-check SCK → GPIO 7, SDA → GPIO 1
2. Secure all connections
3. Use shorter wires
4. Keep wires away from power cables

### Display Flickers

**Causes:**
- ❌ Insufficient power supply
- ❌ Loose VCC or GND connection
- ❌ USB cable too long or low quality

**Solutions:**
1. Use high-quality USB cable (< 1m)
2. Check all power connections
3. Try different USB port or power supply

### Display Works But Shows Wrong Data

**Check:**
1. ✅ DC pin connected to GPIO 35 (not swapped with RES)
2. ✅ RES pin connected to GPIO 21
3. ✅ Sensors are working (check serial output)

### "Failed to initialize SPI bus" Error

**Cause:** SPI bus already in use by another device (IMU uses different SPI pins)

**Solution:** This is normal - the code handles shared SPI buses automatically.

### "Failed to create SSD1306 device" Error

**Causes:**
- ❌ Display not powered
- ❌ Wrong GPIO pins in code
- ❌ Display hardware failure

**Solutions:**
1. Verify 3.3V on VCC pin with multimeter
2. Check GPIO pin numbers in code match wiring
3. Try different display

## Display Modes

Press button (GPIO 3) to cycle through modes:

### Mode 0: Dashboard (Default)
```
┌─────────────────────────────┐
│ ProxiTof  <!>        60FPS  │ ← YELLOW: Title, alerts, FPS
├─────────────────────────────┤
│ ToF 4x4 grid                │
│ IMU orientation             │ ← BLUE: Main sensor data
│ Min distance                │
│ Buffer status               │
└─────────────────────────────┘
```

### Mode 1: ToF Focus
```
┌─────────────────────────────┐
│ DANGER! TOO CLOSE           │ ← YELLOW: Distance warning
├─────────────────────────────┤
│                             │
│   Large ToF Grid            │ ← BLUE: Large numbers
│   (easier to read)          │
│                             │
└─────────────────────────────┘
```

### Mode 2: IMU Focus
```
┌─────────────────────────────┐
│ HIGH MOTION!                │ ← YELLOW: Motion alert
├─────────────────────────────┤
│ Roll:  +12.3°               │
│ Pitch: -5.2°                │ ← BLUE: Large IMU readings
│ Yaw:   +180.0°              │
│ Accel: 0.98 g               │
└─────────────────────────────┘
```

### Mode 3: System Status
```
┌─────────────────────────────┐
│ BUFFER CRITICAL!            │ ← YELLOW: System alerts
├─────────────────────────────┤
│ Logged: 3600                │
│ Buffer: 95.0%               │ ← BLUE: Statistics
│ Dropped: 12                 │
│ FPS: 60                     │
└─────────────────────────────┘
```

## Configuration

Edit `/sdcard/config.txt`:

```ini
# OLED Display Configuration
OLED_ENABLED=true
OLED_REFRESH_HZ=60        # 1-60 Hz (60 = smooth)
OLED_BRIGHTNESS=128       # 0-255 (128 = medium)
OLED_MODE=0               # 0=Dashboard, 1=ToF, 2=IMU, 3=Status
OLED_FLIP=false           # Rotate 180 degrees
```

## Performance

**With 60 Hz refresh rate:**
- ✅ Smooth real-time updates
- ✅ ~10-15% CPU usage
- ✅ No interference with sensors
- ✅ ~16ms display latency

**Power consumption:**
- Display: ~15-25mA
- Total system: ~170-660mA

## Pin Summary

| ESP32-S3 GPIO | Display Pin | Function |
|---------------|-------------|----------|
| GND | GND | Ground |
| 3.3V | VCC | Power (3.3V) |
| GPIO 7 | SCK | SPI Clock |
| GPIO 1 | SDA | SPI Data (MOSI) |
| GPIO 21 | RES | Reset |
| GPIO 19 | DC | Data/Command |

## Common Questions

### Q: Can I use 5V for VCC?
**A:** NO! Most SSD1306 displays are 3.3V only. Check your display datasheet.

### Q: Why doesn't my display have a CS pin?
**A:** Your display uses 4-wire SPI mode. CS is tied LOW internally. This is normal and correct.

### Q: Can I use different GPIO pins?
**A:** Yes, but you must update the pin definitions in `components/oled_display/oled_display.c` and rebuild.

### Q: Can I use I2C instead of SPI?
**A:** Only if your display supports I2C mode (check for address selection jumpers on back). SPI is faster for 60 FPS.

### Q: Display works but text is upside down?
**A:** Set `OLED_FLIP=true` in config.txt to rotate 180 degrees.

### Q: Can I change which pixels are yellow vs blue?
**A:** No, the color split is hardware-based (different color LEDs). The top 16 pixels are always yellow, bottom 48 are always blue. But the firmware strategically places important alerts in the yellow zone for better visibility!

### Q: Why use dual colors?
**A:** The yellow zone naturally draws attention - perfect for warnings and status. The blue zone is calmer - ideal for detailed data. This creates better visual hierarchy without any extra code!

## Next Steps

1. ✅ **Wire display** according to this guide
2. ✅ **Build and flash** firmware
3. ✅ **Verify display** shows startup message
4. ✅ **Check real-time data** updates at 60 FPS
5. ✅ **Adjust brightness** in config.txt if needed

Your display should now show real-time sensor data! 🎉

---

**Document Version:** 1.0  
**Last Updated:** January 20, 2026  
**Hardware:** ESP32-S3-DevKitC-1 + SSD1306 OLED (4-wire SPI)
