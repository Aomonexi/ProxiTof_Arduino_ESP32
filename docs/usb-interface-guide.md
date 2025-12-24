# USB Interface Guide

## 🔌 Overview

The ProxiTof system provides **USB interface** for easy configuration and time synchronization without removing the SD card.

## 🖥️ Computer Access Methods

### Method 1: **Python Tool** (Recommended)
```bash
# Install Python tool
pip install pyserial

# Auto-detect and show status
python tools/proxitof_usb_tool.py --status

# Set WiFi credentials
python tools/proxitof_usb_tool.py --wifi "MyWiFi" "MyPassword"

# Sync time with computer
python tools/proxitof_usb_tool.py --sync-time

# Interactive mode
python tools/proxitof_usb_tool.py --interactive
```

### Method 2: **Serial Terminal**
```bash
# Windows
# Use PuTTY, Tera Term, or Arduino Serial Monitor

# Linux/Mac
screen /dev/ttyACM0 115200
# or
minicom -D /dev/ttyACM0 -b 115200
```

## 📝 Available Commands

| Command | Description | Example |
|---------|-------------|---------|
| `help` | Show available commands | `help` |
| `status` | Show system status | `status` |
| `config` | Show/set configuration | `config wifi MyWiFi MyPass` |
| `time` | Show/set time | `time 2024-12-24 15:30:00` |
| `sync` | Sync time via NTP | `sync` |
| `files` | List log files info | `files` |

## 🚀 Usage Examples

### Quick Status Check:
```
ProxiTof> status

=== System Status ===
Time: 2024-12-24 15:30:45
Time Source: DS3231 RTC
DS3231: Available
Time Synced: Yes
Sync Count: 3
USB Connected: Yes
SD Card: Available
```

### WiFi Configuration:
```
ProxiTof> config wifi LabWiFi SecurePassword123
✓ WiFi configuration saved to SD card
💡 Restart system to apply changes
```

### Time Synchronization:
```
ProxiTof> time 2024-12-24 15:30:00
✓ Time set successfully

ProxiTof> sync
Syncing time from computer...
✓ Time synchronized via NTP
```

## 🔧 SD Card Access

### Current Method: **Physical Removal**
1. **Power off** ESP32
2. **Remove SD card**
3. **Insert into computer**
4. **Edit config.txt**
5. **View log files**
6. **Insert back into ESP32**

### Future Enhancement: **USB Mass Storage**
- ESP32 appears as USB drive
- Direct file access from computer
- No need to remove SD card
- (Requires additional implementation)

## ⚡ Performance Impact

### USB Interface Impact:
- **Processing time**: <1ms per command
- **Memory usage**: ~2KB RAM
- **Performance impact**: Negligible
- **Real-time operation**: Continues normally

### Time Sync via USB:
- **Duration**: Instant (no WiFi needed)
- **Accuracy**: Computer clock accuracy
- **Use case**: Field deployment without WiFi

## 🛠️ Python Tool Features

### Installation:
```bash
pip install pyserial
```

### Command Line Usage:
```bash
# Show device status
python proxitof_usb_tool.py --status

# Configure WiFi
python proxitof_usb_tool.py --wifi "NetworkName" "Password"

# Set timezone
python proxitof_usb_tool.py --timezone "EST5EDT,M3.2.0,M11.1.0"

# Sync time with computer
python proxitof_usb_tool.py --sync-time

# Interactive mode
python proxitof_usb_tool.py --interactive
```

### Auto-Detection:
- Automatically finds ProxiTof device
- Works on Windows, Linux, Mac
- No need to specify COM port

## 🔍 Troubleshooting

### Device Not Found:
```
❌ No ProxiTof device found
💡 Make sure device is connected via USB cable
```
**Solutions:**
- Check USB cable connection
- Try different USB port
- Install ESP32 USB drivers
- Check Device Manager (Windows)

### Connection Failed:
```
❌ Device found but not responding correctly
```
**Solutions:**
- Close other serial programs
- Try different baud rate
- Restart ESP32
- Check USB cable quality

### Commands Not Working:
```
Unknown command. Type 'help' for available commands.
```
**Solutions:**
- Type `help` to see available commands
- Check command syntax
- Ensure proper spacing

## 🎯 Best Practices

### For Field Configuration:
1. **Use Python tool** for quick changes
2. **Sync time** before deployment
3. **Verify settings** with status command
4. **Test WiFi** connection before leaving

### For Development:
1. **Use interactive mode** for testing
2. **Monitor status** during development
3. **Quick time sync** for accurate logging
4. **Easy config changes** without recompiling

### For Production:
1. **Secure USB access** if needed
2. **Document configurations** used
3. **Regular time sync** for accuracy
4. **Monitor device status** remotely

## 🔮 Future Enhancements

### Planned Features:
- **USB Mass Storage**: Direct SD card access
- **Web interface**: Browser-based configuration
- **Remote monitoring**: Status via network
- **Firmware updates**: Over USB interface

The USB interface makes ProxiTof **easy to configure and maintain** in the field!