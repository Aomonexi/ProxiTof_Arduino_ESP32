# How to Enable SD Card Functionality

## Current Status
SD card is **disabled by default** to prevent 5-10 second timeout delays when hardware is not connected.

## Method 1: Build Flag (Easiest)

Add the build flag when building:

```bash
# Enable SD card and build
idf.py -D ENABLE_SD_CARD=1 build

# Or flash directly with SD card enabled
idf.py -D ENABLE_SD_CARD=1 flash monitor
```

## Method 2: Edit CMakeLists.txt (Permanent)

Add this line to your main `CMakeLists.txt`:

```cmake
# Enable SD card functionality
add_compile_definitions(ENABLE_SD_CARD=1)
```

Then build normally:
```bash
idf.py build
```

## Method 3: Create Kconfig Option (Advanced)

Create `Kconfig.projbuild` in project root:

```kconfig
menu "ProxiTof Configuration"
    config ENABLE_SD_CARD
        bool "Enable SD Card Logging"
        default n
        help
            Enable SD card data logging functionality.
            Requires SDIO hardware to be connected.
endmenu
```

Then use menuconfig:
```bash
idf.py menuconfig
# Navigate to: ProxiTof Configuration → Enable SD Card Logging
```

## What Happens When Enabled

### Success Case (SD card working):
```
I (1234) MAIN: Initializing SD card storage...
I (2345) STORAGE_MGR: Using SDMMC host with conflict-free GPIO assignment
I (2346) STORAGE_MGR: Pins: CLK=14, CMD=15, D0=2, D1=16, D2=17, D3=18
I (3456) STORAGE_MGR: ✓ Filesystem mounted successfully
I (3457) STORAGE_MGR: ✓ SD card mounted at /sdcard
I (3458) MAIN: ✓ SD card initialized successfully
I (3459) MAIN: ✓ Data logging started
```

### Failure Case (SD card not working):
```
I (1234) MAIN: Initializing SD card storage...
W (6789) STORAGE_MGR: ❌ SD card initialization timeout - hardware not connected
W (6790) MAIN: SD card initialization failed: ESP_ERR_TIMEOUT
W (6791) MAIN: Continuing without data logging
W (6792) MAIN: Troubleshooting:
W (6793) MAIN:   1. Check SD card is inserted
W (6794) MAIN:   2. Format SD card as FAT32
W (6795) MAIN:   3. Check wiring: CLK=14, CMD=15, D0=2, D1=16, D2=17, D3=18
W (6796) MAIN:   4. Verify 3.3V power to SD card
```

## Testing Without Hardware

If you want to test the SD card code without hardware connected, expect:
- **5-10 second delay** during initialization
- **Timeout error** message
- **System continues normally** after timeout

## Recommended Testing Sequence

1. **First**: Test without SD card enabled (sensors + LED only)
2. **Then**: Wire SD card hardware according to pinout
3. **Finally**: Enable SD card and test logging

## SD Card Requirements

- **Format**: FAT32
- **Size**: 8GB+ recommended  
- **Speed**: Class 10 or higher
- **Voltage**: 3.3V compatible
- **Interface**: SDIO (not SPI SD modules)

## Pin Configuration (Updated)

```
ESP32-S3          SD Card SDIO
--------          ------------
GPIO 14 (CLK)  -> CLK       (Clock)
GPIO 15 (CMD)  -> CMD       (Command/Response)
GPIO 2  (D0)   -> D0        (Data Line 0)
GPIO 16 (D1)   -> D1        (Data Line 1)
GPIO 17 (D2)   -> D2        (Data Line 2)
GPIO 18 (D3)   -> D3        (Data Line 3)
3.3V           -> VCC
GND            -> GND
```

**Note**: These pins are conflict-free and don't interfere with ToF, IMU, Button, or LED.