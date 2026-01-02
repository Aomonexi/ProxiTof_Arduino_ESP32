# USB Mass Storage Guide

## Overview

The ProxiTof system includes USB Mass Storage Class (MSC) functionality that allows the SD card to appear as a USB drive when connected to a computer. This provides easy access to log files and configuration without removing the SD card.

## Features

- **Automatic SD Card Access**: SD card appears as USB drive when connected
- **Smart Switching**: Automatically pauses data logging when USB MSC is active
- **Manual Control**: USB commands to start/stop MSC mode
- **High Performance**: 4KB buffer size for fast file transfers
- **Safe Operation**: Proper mount/unmount handling prevents data corruption

## Hardware Requirements

- ESP32-S3 with USB-C connector
- SD card properly wired and initialized
- USB-C cable for computer connection

## Usage

### Automatic Mode

1. **Connect USB Cable**: Plug USB-C cable into ESP32-S3
2. **Access SD Card**: SD card appears as USB drive on computer
3. **Data Logging**: Automatically pauses while USB MSC is active
4. **Disconnect**: Remove USB cable to resume data logging

### Manual Control via USB Commands

Connect to ESP32-S3 via serial terminal and use these commands:

```bash
# Start USB Mass Storage (SD card as USB drive)
msc start

# Stop USB Mass Storage (resume data logging)
msc stop

# Check USB Mass Storage status
msc status
```

### Command Examples

```bash
# Check current status
> msc status
=== USB Mass Storage Status ===
MSC Active: ✗ NO
USB Connected: ✓ YES
Data Logging: ▶️  ACTIVE

# Start USB Mass Storage
> msc start
✓ USB Mass Storage started
💾 SD card is now accessible as USB drive
⚠️  Data logging paused while MSC active
💡 Use 'msc stop' to resume data logging

# Stop USB Mass Storage
> msc stop
✓ USB Mass Storage stopped
📊 Data logging resumed
💾 SD card returned to application control
```

## File Access

When USB MSC is active, you can access:

### Log Files
- **Location**: `/YYYY-MM-DD/` folders (organized by date)
- **Format**: CSV files with ToF + IMU + timestamp data
- **Naming**: `proxitof_YYYYMMDD_HHMMSS.csv`

### Configuration
- **File**: `/config.txt` (system configuration)
- **Edit**: Modify WiFi, timezone, hardware settings
- **Apply**: Changes take effect on next boot

### Example File Structure
```
/
├── config.txt                    # System configuration
├── 2024-12-26/                  # Today's logs
│   ├── proxitof_20241226_090000.csv
│   ├── proxitof_20241226_120000.csv
│   └── proxitof_20241226_150000.csv
├── 2024-12-25/                  # Yesterday's logs
│   └── proxitof_20241225_140000.csv
└── wifi_config.txt              # WiFi backup config
```

## Technical Details

### USB Configuration
- **USB Host Control Transfer**: 1024 bytes (increased from 256 bytes)
- **MSC Buffer Size**: 4096 bytes (4KB) for optimal performance
- **Transfer Speed**: ~1-2 MB/s depending on SD card class
- **Memory Usage**: ~8KB RAM for USB MSC stack

### USB Peripheral Sharing
- **USB Serial JTAG**: Used for programming and serial commands
- **USB MSC**: Used for SD card access as USB drive
- **Coexistence**: Both can operate simultaneously on ESP32-S3
- **Port Sharing**: Same USB-C port serves both functions

### Important USB Notes
⚠️ **USB Peripheral Compatibility**
- ESP32-S3 USB Serial JTAG and TinyUSB MSC can coexist
- Programming and debugging work alongside USB MSC functionality
- USB Serial JTAG remains available for commands even when MSC is active
- Both use the same physical USB-C port but different USB endpoints

### Safety Features
- **Exclusive Access**: Only USB or application can access SD card at once
- **Automatic Switching**: Smart detection of USB connection state
- **Data Protection**: Proper filesystem unmounting prevents corruption
- **Error Recovery**: Graceful handling of USB disconnection

### Mount Points
- **USB Mode**: SD card mounted to USB host (computer access)
- **App Mode**: SD card mounted to ESP32 application (data logging)
- **Switching**: Automatic or manual via USB commands

## Troubleshooting

### SD Card Not Appearing
1. **Check USB Cable**: Ensure data-capable USB-C cable
2. **Verify SD Card**: Confirm SD card is initialized and working
3. **Try Manual Start**: Use `msc start` command via serial
4. **Check Logs**: Monitor ESP32 serial output for errors

### Data Logging Issues
1. **Check MSC Status**: Use `msc status` to verify state
2. **Stop MSC**: Use `msc stop` to return SD card to application
3. **Verify SD Card**: Ensure SD card is not corrupted
4. **Restart System**: Power cycle ESP32 if needed

### Performance Issues
1. **SD Card Class**: Use Class 10 or better SD cards
2. **USB Port**: Try different USB ports on computer
3. **File Size**: Large files (>100MB) may transfer slowly
4. **Fragmentation**: Format SD card if heavily fragmented

### USB Conflicts and Compatibility
1. **Programming Issues**: If flashing fails, disconnect and reconnect USB
2. **Serial Commands**: USB Serial JTAG works even when MSC is active
3. **Device Recognition**: Computer may show both serial port and USB drive
4. **Driver Issues**: Windows may need USB driver updates for dual functionality

### Common Error Messages

```bash
# USB initialization conflict
❌ USB Mass Storage initialization failed: ESP_ERR_INVALID_STATE
💡 Check USB Serial JTAG is not blocking TinyUSB

# USB peripheral busy
❌ Failed to install TinyUSB driver: ESP_ERR_INVALID_STATE  
💡 Restart ESP32 to reset USB peripheral state

# SD card not initialized
❌ Failed to start USB Mass Storage: ESP_ERR_INVALID_STATE
💡 Check SD card is initialized

# Already active
❌ USB Mass Storage already active
💡 Use 'msc stop' first, then 'msc start'

# Hardware issue
❌ Failed to create MSC storage: ESP_FAIL
💡 Check SD card wiring and power
```

## Integration with Other Systems

### Data Analysis Workflow
1. **Collect Data**: Let system log data normally
2. **Connect USB**: Plug in USB cable when ready to analyze
3. **Access Files**: SD card appears as USB drive
4. **Copy Data**: Copy CSV files to computer for analysis
5. **Disconnect**: Remove USB cable to resume logging

### Field Deployment
1. **Configure**: Edit `config.txt` via USB MSC
2. **Deploy**: Install system in field location
3. **Monitor**: Use USB commands for status checks
4. **Retrieve**: Connect USB to download data

### Multi-Device Management
- Each device creates date-organized folders
- Unique device IDs in filenames (future enhancement)
- Batch processing scripts can handle multiple devices

## Best Practices

### Data Management
- **Regular Downloads**: Download data weekly to prevent SD card full
- **Backup Configuration**: Keep backup of `config.txt`
- **File Organization**: Use date-based folder structure
- **Compression**: Compress old log files to save space

### System Operation
- **Graceful Shutdown**: Use `msc stop` before powering off
- **USB Safety**: Safely eject USB drive before disconnecting
- **Monitor Status**: Check `msc status` regularly
- **Error Handling**: Monitor serial logs for issues

### Performance Optimization
- **Fast SD Cards**: Use Class 10 or UHS-I cards
- **Regular Formatting**: Format SD card monthly
- **File Size Limits**: Keep individual files under 100MB
- **Buffer Management**: System handles buffering automatically

## Future Enhancements

- **Auto-detection**: Automatic USB MSC start on cable connection
- **Device ID**: Unique device identifiers in filenames
- **Compression**: Real-time log file compression
- **Web Interface**: Browser-based file access via WiFi
- **Cloud Sync**: Automatic cloud backup integration

## Related Documentation

- [Storage Integration Guide](storage-integration-guide.md)
- [USB Interface Guide](usb-interface-guide.md)
- [Field Configuration Guide](field-configuration-guide.md)
- [System Overview](system-overview.md)