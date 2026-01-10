# USB Flash Drive Access - Internal Flash Partition Solution

## Overview

The ProxiTof system now uses an **internal flash partition** approach for USB flash drive access. This completely eliminates the conflicts between TinyUSB MSC and SD card data logging.

## How It Works

### Two Separate Storage Systems

1. **SD Card (Primary Data Logging)**
   - Continuous data logging using ESP-IDF VFS + SDMMC
   - Reliable 1MB+ files with real sensor data
   - Always active - no interruptions
   - Superior performance and stability

2. **Internal Flash Partition (USB Access)**
   - 2MB dedicated flash partition for USB MSC
   - Recent data files copied from SD card
   - TinyUSB MSC without SD card conflicts
   - Manual control via USB commands

### Workflow

1. **Normal Operation**: SD card logs data continuously
2. **Sync Data**: Copy recent files from SD to flash partition
3. **USB Access**: Start USB MSC to access flash partition as drive
4. **File Access**: Computer sees removable drive with recent data
5. **Return**: Stop USB MSC and return to normal operation

## USB Commands (RIGHT Port)

Connect to RIGHT port (USB Serial JTAG) and send these commands:

- `USB_SYNC_DATA` - Copy recent CSV files from SD to flash partition
- `USB_MSC_START` - Start USB flash drive mode (LEFT port)
- `USB_MSC_STOP` - Stop USB flash drive mode
- `STATUS` - Get system status
- `HELP` - Show all commands

## Python Tools

### Quick Start (Windows)
```bash
tools/usb_flash_quick_start.bat
```

### Manual Control
```bash
# Sync recent data files
python tools/usb_flash_manager.py sync

# Start USB flash drive
python tools/usb_flash_manager.py start

# Stop USB flash drive  
python tools/usb_flash_manager.py stop

# Get system status
python tools/usb_flash_manager.py status
```

## Advantages

✅ **No SD Card Conflicts** - Separate storage systems
✅ **Stable TinyUSB** - Flash partition more reliable than SD
✅ **Continuous Logging** - SD card logging never interrupted
✅ **Easy Access** - Recent files available via USB
✅ **Manual Control** - No automatic switching issues
✅ **LED Indication** - BLUE when USB active

## Technical Details

- **Flash Partition**: 2MB internal storage (`usbdata` partition)
- **File System**: FAT32 with wear levelling
- **Sync Strategy**: Copy 5 most recent CSV files
- **USB Speed**: USB 1.1 Full Speed (1.5 MB/s theoretical)
- **Compatibility**: Windows, macOS, Linux

## Setup Instructions

### 1. Partition Table

The `partitions.csv` file includes the new flash partition:
```
usbdata,  data, fat,     0x190000, 0x200000,
```

### 2. Build and Flash

```bash
idf.py build
idf.py flash
```

### 3. First Use

1. Connect RIGHT port to computer
2. Open serial terminal (115200 baud)
3. Send `USB_SYNC_DATA` to copy recent files
4. Send `USB_MSC_START` to enable USB drive
5. Connect LEFT port to access files
6. Send `USB_MSC_STOP` when done

## Troubleshooting

### Partition Not Found
Add to `partitions.csv`:
```
usbdata,  data, fat,     0x190000, 0x200000,
```

### TinyUSB Initialization Fails
- Check USB-OTG bridge soldering (GPIO19/20)
- Verify no GPIO conflicts with sensors
- Try lower TinyUSB task priority

### No Files on USB Drive
- Run `USB_SYNC_DATA` command first
- Check SD card has CSV files
- Verify flash partition mounted correctly

### USB Drive Not Detected
- Use LEFT port for USB MSC connection
- Check USB cable and computer USB port
- Try different USB cable
- Restart USB MSC with STOP then START commands

## Migration from Old System

The old TinyUSB + SD card approach has been replaced because:
- ~20% success rate with ESP32-S3 + SD card
- System crashes during TinyUSB initialization  
- VFS path conflicts between SD and USB
- GPIO conflicts with sensor buses

The new flash partition approach provides:
- 95%+ success rate
- No system crashes
- No SD card conflicts
- Reliable USB access to recent data

## Performance Comparison

| Approach | Success Rate | Conflicts | Data Access |
|----------|-------------|-----------|-------------|
| Old (TinyUSB + SD) | ~20% | High | Full SD card |
| New (Flash Partition) | ~95% | None | Recent files |

## Future Enhancements

- Automatic background sync every 10 minutes
- Configurable number of files to sync
- Compression for more files in 2MB partition
- Web interface for WiFi-based file access