# USB Flash Drive - Direct SD Card Access

## Overview

The ProxiTof system now exposes the **SD card directly** as a USB flash drive. When you connect the LEFT USB port to your computer, the SD card appears as a removable drive with all your data files.

## How It Works

### Direct SD Card Exposure
- **SD Card**: Contains all your sensor data (CSV files)
- **USB Connection**: LEFT port exposes SD card as USB flash drive
- **No Copying**: Direct access to SD card - no file copying needed
- **Full Access**: See all folders and files on the SD card

### Safe Data Management
1. **Normal Operation**: SD card logs data continuously
2. **USB Connection**: System detects USB host connection
3. **Safe Pause**: Data logging safely paused, buffers flushed
4. **USB Access**: SD card exposed as removable drive
5. **LED Indication**: LED turns BLUE during USB mode
6. **Disconnection**: Data logging automatically resumes

## Usage Instructions

### Automatic Mode (Recommended)
1. **Connect LEFT port** to your computer with USB cable
2. **Wait for BLUE LED** - indicates USB flash drive is active
3. **Access files** - SD card appears as removable drive on computer
4. **Copy/view files** as needed
5. **Safely eject** the drive from your computer
6. **Disconnect USB cable** - LED returns to distance colors, logging resumes

### Manual Mode (via Commands)
1. **Connect RIGHT port** to computer (programming port)
2. **Open serial terminal** (115200 baud)
3. **Send `USB_MSC_START`** to manually start USB flash drive
4. **Connect LEFT port** to access files
5. **Send `USB_MSC_STOP`** when done

## USB Commands (RIGHT Port)

- `USB_MSC_START` - Manually start USB flash drive mode
- `USB_MSC_STOP` - Manually stop USB flash drive mode
- `STATUS` - Get system status
- `HELP` - Show all available commands

## File Structure on USB Drive

When you access the USB drive, you'll see:
```
/
├── 20260107/          # Date folders (YYYYMMDD)
│   ├── log1430.csv    # Data files (logHHMM.csv)
│   ├── log1445.csv
│   └── log1500.csv
├── 20260108/
│   └── log0900.csv
└── README.txt         # Info file (if present)
```

## LED Indication

- **Distance Colors** (Normal): RED (close), YELLOW (medium), GREEN (far)
- **BLUE** (USB Mode): USB flash drive is active
- **Flashing**: Transitioning between modes

## Advantages

✅ **Direct Access** - No file copying, see all SD card contents
✅ **Full Capacity** - Access entire SD card storage
✅ **Real-time** - Files appear immediately after logging
✅ **Standard Interface** - Works like any USB flash drive
✅ **Safe Operation** - Data logging paused during USB access
✅ **Automatic** - Plug and play operation

## Technical Details

- **USB Speed**: USB 1.1 Full Speed (~1 MB/s)
- **SD Card Interface**: SDMMC 4-bit mode (high performance)
- **File System**: FAT32 (universal compatibility)
- **Sector Access**: Direct sector-level read/write
- **Safety**: Emergency flush before USB activation

## Troubleshooting

### USB Drive Not Detected
- Check LEFT port connection (not RIGHT port)
- Try different USB cable
- Wait for BLUE LED before accessing
- Use manual commands if automatic detection fails

### Files Not Visible
- Ensure SD card is properly inserted and formatted
- Check that data logging has created files
- Try refreshing/remounting the drive

### Data Corruption
- Always wait for BLUE LED before accessing files
- Use "Safely Remove Hardware" before disconnecting
- Don't disconnect during file operations
- System automatically flushes data before USB mode

### Performance Issues
- Use shorter USB cables for better signal integrity
- Ensure stable power supply
- Close unnecessary programs on computer
- Consider SDMMC 4-bit mode for better SD performance

## Comparison with Previous Approaches

| Feature | Direct SD Card | Flash Partition | Old TinyUSB |
|---------|---------------|-----------------|-------------|
| Success Rate | ~90% | ~95% | ~20% |
| File Access | Full SD card | Recent files only | Full SD card |
| Setup Complexity | Simple | Medium | Complex |
| Storage Capacity | Full SD card | 2MB limit | Full SD card |
| Conflicts | Managed | None | High |

## Safety Features

- **Safe Shutdown**: Data logging paused before USB access
- **Buffer Flush**: All pending data written before USB mode
- **Automatic Resume**: Data logging resumes when USB disconnected
- **Error Recovery**: System continues if USB operations fail
- **LED Feedback**: Clear indication of current mode

## Future Improvements

- Faster USB 2.0 support (hardware dependent)
- Background file compression
- Selective file exposure
- Web-based file access over WiFi