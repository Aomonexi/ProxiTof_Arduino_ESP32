# ESP-IDF Build Guide

## Why Use ESP-IDF Directly?

**Advantages:**
- ✅ No version conflicts with component dependencies
- ✅ Access to latest ESP-IDF features (v5.0+)
- ✅ Better control over build configuration
- ✅ Easier to use ESP-IDF examples
- ✅ Professional development workflow

## Prerequisites

1. **Install ESP-IDF v5.0+**
   - Follow: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/

2. **Set up environment**
   
   **⚠️ IMPORTANT: Must run EVERY TIME before using idf.py commands**
   
   ```powershell
   # Windows (PowerShell) - Replace [USERNAME] with your actual username
   C:\Users\[USERNAME]\esp\v5.5.1\esp-idf\export.ps1
   
   # Alternative if ESP-IDF is in different location
   $env:IDF_PATH\export.ps1
   ```
   
   ```bash
   # Linux/Mac
   . $HOME/esp/esp-idf/export.sh
   ```
   
   **Expected Output:**
   ```
   Activating ESP-IDF 5.5
   Setting IDF_PATH to 'C:\Users\[USERNAME]\esp\v5.5.1\esp-idf'.
   * Checking python version ... 3.13.11
   * Checking python dependencies ... OK
   * Deactivating the current ESP-IDF environment (if any) ... OK
   * Establishing a new ESP-IDF environment ... OK
   * Identifying shell ... powershell.exe
   * Detecting outdated tools in system ... OK - no outdated tools found
   
   Done! You can now compile ESP-IDF projects.
   Go to the project directory and run:
   
     idf.py build
   ```

## Build Commands

### Full Build
```bash
idf.py build
```

### Flash to Device
```bash
idf.py -p COM3 flash
```

### Monitor Serial Output
```bash
idf.py -p COM3 monitor
```

### Flash + Monitor (Combined)
```bash
idf.py -p COM3 flash monitor
```

### Clean Build
```bash
idf.py fullclean
idf.py build
```

## SD Card Pin Configuration

**Shared SPI Bus with IMU (Recommended):**

The SD card shares the SPI bus with the BNO08x IMU sensor:
- **MOSI: GPIO 11** (shared with IMU)
- **MISO: GPIO 13** (shared with IMU)
- **CLK: GPIO 12** (shared with IMU)
- **CS: GPIO 7** (unique to SD card, IMU uses GPIO 10)

**Why Shared Bus?**
- ✅ Saves GPIO pins
- ✅ Standard practice for SPI devices
- ✅ Each device has unique CS (Chip Select) pin
- ✅ SPI protocol handles multiple devices automatically

**Wiring:**
```
ESP32-S3          SD Card       IMU (BNO08x)
--------          -------       ------------
GPIO 11 (MOSI) -> DI/MOSI   -> MOSI
GPIO 13 (MISO) -> DO/MISO   -> MISO  
GPIO 12 (CLK)  -> CLK       -> SCK
GPIO 7  (CS)   -> CS        -> (not connected)
GPIO 10 (CS)   -> (not)     -> CS
3.3V           -> VCC       -> VCC
GND            -> GND       -> GND
```

**To Change CS Pin Only:**
Edit `components/storage_manager/storage_manager.c`:
```c
slot_config.gpio_cs = GPIO_NUM_XX;  // Your CS pin (must be different from IMU CS)
```

## SD Card Requirements

- **Format:** FAT32
- **Capacity:** 8GB+ recommended
- **Speed Class:** Class 10 or higher
- **Voltage:** 3.3V compatible

## Troubleshooting

### Environment Not Activated
**Error:** `idf.py : The term 'idf.py' is not recognized`

**Solution:** Run the export script first:
```powershell
# Windows - Replace [USERNAME] with your actual username
C:\Users\[USERNAME]\esp\v5.5.1\esp-idf\export.ps1

# Then verify it worked
idf.py --version
```

### SD Card Not Detected
1. Check wiring (MOSI, MISO, CLK, CS, VCC, GND)
2. Verify SD card is formatted as FAT32
3. Check power supply (3.3V, sufficient current)
4. Try lower SPI speed in code

### Build Errors
1. Make sure ESP-IDF environment is activated
2. Run `idf.py fullclean` then `idf.py build`
3. Check component dependencies in `idf_component.yml`

### Component Version Conflicts
- Use ESP-IDF v5.0+ for best compatibility
- Check component versions in `components/*/idf_component.yml`

## Performance Tips

**For Maximum Logging Speed:**
1. Use Class 10 or UHS-I SD cards
2. Increase SPI clock speed (up to 20MHz)
3. Increase buffer size if needed
4. Use DMA for SPI transfers (already enabled)

## Next Steps

After successful build:
1. Insert formatted SD card
2. Flash firmware
3. Monitor serial output
4. Check for "SD card mounted" message
5. Data will be logged to `/sdcard/YYYY-MM-DD/log_*.csv`

## File Structure

```
/sdcard/
├── 2024-12-09/
│   ├── log_20241209_143022.csv
│   └── log_20241209_153045.csv
└── 2024-12-10/
    └── log_20241210_084512.csv
```

## CSV Format

```csv
timestamp_ms,tof_00,tof_01,...,tof_33,roll,pitch,yaw,accel_x,accel_y,accel_z,gyro_x,gyro_y,gyro_z,flag_manual
1234567,450,520,480,...,12.3,-5.6,89.1,0.12,-0.05,9.81,0.01,-0.02,0.03,0
```

## Monitoring Logs

**View in real-time:**
```bash
idf.py -p COM3 monitor
```

**Exit monitor:** `Ctrl+]`

**Filter logs:**
```bash
idf.py -p COM3 monitor | grep "STORAGE_MGR"
```
