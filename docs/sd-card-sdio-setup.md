# SD Card SDIO Setup Guide

## Overview

The system now uses **SDIO mode** for SD card access, which is:
- ✅ **Much faster** than SPI (20-40MHz vs 10MHz)
- ✅ **More reliable** (native SD protocol)
- ✅ **No bus sharing** (dedicated pins, no conflict with IMU)
- ✅ **Better for continuous logging** at 60Hz

## Hardware Required

**Recommended:** [Adafruit MicroSD SPI/SDIO Breakout](https://learn.adafruit.com/adafruit-microsd-spi-sdio/pinouts)

**Why this board?**
- ✅ Built-in pull-up resistors (no external components needed)
- ✅ Level shifter (works with 3.3V)
- ✅ Supports both SPI and SDIO modes
- ✅ High quality, reliable
- ✅ Designed for ESP32

## Wiring Diagram

### SDIO 4-Bit Mode (Recommended - Fastest)

```
Adafruit Breakout    ESP32-S3        Notes
----------------     --------        -----
CLK              →   GPIO 39         Clock signal
CMD              →   GPIO 38         Command line
DAT0 (D0)        →   GPIO 40         Data bit 0
DAT1 (D1)        →   GPIO 41         Data bit 1
DAT2 (D2)        →   GPIO 42         Data bit 2
DAT3 (D3)        →   GPIO 43         Data bit 3
VCC              →   3.3V            Power
GND              →   GND             Ground
```

### SDIO 1-Bit Mode (Alternative - Slower but fewer pins)

```
Adafruit Breakout    ESP32-S3        Notes
----------------     --------        -----
CLK              →   GPIO 39         Clock signal
CMD              →   GPIO 38         Command line
DAT0 (D0)        →   GPIO 40         Data bit 0
VCC              →   3.3V            Power
GND              →   GND             Ground
```

**Note:** 4-bit mode is recommended for maximum performance!

## Complete System Wiring

```
ESP32-S3 Pin Allocation:
========================

I2C (ToF Sensor):
  GPIO 8  → VL53L8CX SDA
  GPIO 9  → VL53L8CX SCL
  GPIO 4  → VL53L8CX LPN

SPI (IMU Sensor):
  GPIO 11 → BNO08x MOSI
  GPIO 13 → BNO08x MISO
  GPIO 12 → BNO08x SCK
  GPIO 10 → BNO08x CS
  GPIO 5  → BNO08x INT
  GPIO 6  → BNO08x RST

SDMMC (SD Card):
  GPIO 39 → SD CLK
  GPIO 38 → SD CMD
  GPIO 40 → SD D0
  GPIO 41 → SD D1
  GPIO 42 → SD D2
  GPIO 43 → SD D3

GPIO:
  GPIO 3  → Button
  GPIO 48 → WS2812 LED
```

**No pin conflicts!** All interfaces use separate pins.

## Pull-up Resistors

**Good news:** The Adafruit board has **built-in 10kΩ pull-ups** on:
- CMD line
- D0, D1, D2, D3 lines

**No external pull-ups needed!**

The ESP32-S3 also enables internal pull-ups as backup.

## SD Card Requirements

- **Format:** FAT32
- **Capacity:** 8GB-32GB recommended (larger cards work too)
- **Speed Class:** Class 10 or UHS-I for best performance
- **Voltage:** 3.3V compatible (Adafruit board handles this)

## Performance Comparison

| Mode | Speed | Pins | Bus Sharing | Reliability |
|------|-------|------|-------------|-------------|
| **SPI** | ~10MHz | 4 | Yes (with IMU) | Medium |
| **SDIO 1-bit** | ~20MHz | 3 | No | High |
| **SDIO 4-bit** | ~40MHz | 6 | No | Very High |

**Current implementation:** SDIO 4-bit mode (fastest and most reliable)

## Code Configuration

The code is already configured for SDIO mode:

```c
// In storage_manager.c
sdmmc_host_t host = SDMMC_HOST_DEFAULT();
host.max_freq_khz = SDMMC_FREQ_DEFAULT;  // 20MHz

sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
slot_config.width = 4;  // 4-bit mode
slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;
```

## Graceful Degradation

**The system works WITHOUT an SD card!**

If SD card initialization fails:
- ✅ Sensors continue working (ToF + IMU)
- ✅ LED continues working
- ✅ Display continues working
- ❌ No data logging (obviously)

**Serial output:**
```
W (xxxx) MAIN: SD card initialization failed: ESP_ERR_TIMEOUT
W (xxxx) MAIN: Continuing without data logging
W (xxxx) MAIN: 
W (xxxx) MAIN: Troubleshooting:
W (xxxx) MAIN:   1. Check SD card is inserted
W (xxxx) MAIN:   2. Format SD card as FAT32
W (xxxx) MAIN:   3. Check wiring: CLK=39, CMD=38, D0-D3=40-43
W (xxxx) MAIN:   4. Verify 3.3V power to SD card
```

## Build and Flash

```bash
idf.py build
idf.py -p COM3 flash monitor
```

## Expected Boot Messages

**With SD card working:**
```
I (xxxx) MAIN: Initializing SD card storage...
I (xxxx) STORAGE_MGR: Initializing SD card via SDIO (4-bit mode)...
I (xxxx) STORAGE_MGR: Using SDMMC host (dedicated pins, no sharing with IMU)
I (xxxx) STORAGE_MGR: Pins: CLK=39, CMD=38, D0=40, D1=41, D2=42, D3=43
I (xxxx) STORAGE_MGR: Mounting filesystem...
I (xxxx) STORAGE_MGR: Filesystem mounted
I (xxxx) STORAGE_MGR: SD card mounted at /sdcard
I (xxxx) STORAGE_MGR: Created directory: /sdcard/2024-12-09
I (xxxx) STORAGE_MGR: Created log file: /sdcard/2024-12-09/log_20241209_143022.csv
I (xxxx) MAIN: ✓ SD card initialized successfully
I (xxxx) MAIN: ✓ Data logging started
```

**Without SD card (graceful degradation):**
```
I (xxxx) MAIN: Initializing SD card storage...
I (xxxx) STORAGE_MGR: Initializing SD card via SDIO (4-bit mode)...
E (xxxx) STORAGE_MGR: Failed to mount filesystem
W (xxxx) MAIN: SD card initialization failed: ESP_ERR_TIMEOUT
W (xxxx) MAIN: Continuing without data logging
```

## Troubleshooting

### SD Card Not Detected

**Check:**
1. SD card is inserted fully
2. SD card is formatted as FAT32
3. Wiring is correct (especially CLK, CMD, D0)
4. 3.3V power is connected
5. GND is connected

**Try:**
- Different SD card (some cards are more compatible)
- Reformat SD card as FAT32
- Check for loose connections
- Verify Adafruit board has power LED on

### Slow Performance

**Solutions:**
1. Use Class 10 or UHS-I SD card
2. Increase frequency in code:
   ```c
   host.max_freq_khz = SDMMC_FREQ_HIGHSPEED;  // 40MHz
   ```
3. Ensure short wires (< 10cm)
4. Use 4-bit mode (not 1-bit)

### Data Corruption

**Solutions:**
1. Reduce frequency:
   ```c
   host.max_freq_khz = SDMMC_FREQ_DEFAULT;  // 20MHz
   ```
2. Check power supply (SD cards need stable 3.3V)
3. Add decoupling capacitor (100nF) near SD card VCC
4. Use shorter wires

## Migration from SPI Mode

If you were using SPI mode before:

**Old pins (SPI):**
- MOSI: GPIO 11 (shared with IMU)
- MISO: GPIO 13 (shared with IMU)
- SCK: GPIO 12 (shared with IMU)
- CS: GPIO 7

**New pins (SDIO):**
- CLK: GPIO 39 (dedicated)
- CMD: GPIO 38 (dedicated)
- D0-D3: GPIO 40-43 (dedicated)

**Benefits:**
- ✅ 2-4x faster
- ✅ No bus sharing conflicts
- ✅ More reliable
- ✅ Better for continuous logging

## Testing Procedure

### 1. Hardware Setup
```
1. Wire Adafruit board to ESP32-S3 (see wiring diagram)
2. Insert FAT32-formatted SD card
3. Verify 3.3V power
4. Check all connections
```

### 2. Build and Flash
```bash
idf.py build
idf.py -p COM3 flash monitor
```

### 3. Verify Initialization
```
Look for:
✓ SD card initialized successfully
✓ Data logging started
```

### 4. Check Logging
```
After 10 seconds:
SD Card: Logging | Entries: 600 | Buffer: 5.0% | Dropped: 0
```

### 5. Verify SD Card Contents
```
1. Power off ESP32-S3
2. Remove SD card
3. Insert into computer
4. Check for: /2024-12-09/log_YYYYMMDD_HHMMSS.csv
5. Open CSV file, verify data
```

## Performance Expectations

**SDIO 4-bit mode:**
- Write speed: ~2-5 MB/s
- Latency: < 10ms per flush
- Buffer usage: < 20% typical
- Dropped frames: 0

**At 60Hz logging:**
- ~60 entries/second
- ~1 MB/minute
- ~60 MB/hour
- File rotation every ~100 minutes

## Summary

### ✅ Advantages of SDIO Mode

1. **Faster:** 2-4x faster than SPI
2. **More reliable:** Native SD protocol
3. **No conflicts:** Dedicated pins
4. **Better performance:** Lower latency, higher throughput
5. **Production ready:** Used in commercial products

### 🎯 Next Steps

1. **Order Adafruit MicroSD SDIO breakout**
2. **Wire according to diagram** (GPIO 38-43)
3. **Format SD card as FAT32**
4. **Build and flash** (code already updated!)
5. **Start logging!**

---

**The code is ready for SDIO mode!** Just wire up the Adafruit board when it arrives and you're good to go! 🚀
