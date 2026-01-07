# USB Quick Start Guide

## 🚀 Quick Setup

### 1. Connect USB Cable
- Connect ESP32-S3 to computer via USB-C cable
- Device should appear as COM port (e.g., COM4)

### 2. Access USB Commands

**Method A: Python Tool (Recommended)**
```bash
# Navigate to project directory
cd ProxiTof_Arduino_ESP32

# Install Python dependencies
pip install pyserial

# Use interactive mode
python tools/proxitof_usb_tool.py --interactive
```

**Method B: Serial Terminal**
- Open PuTTY, Arduino Serial Monitor, or VS Code terminal
- Port: COM4 (or your ESP32 port)
- Baud rate: 115200
- Type commands directly

## 📱 Available Commands

### System Commands
```
help                     # Show all commands
status                   # System status and health
config                   # Show current configuration
time                     # Show current time
sync                     # Sync time with computer
files                    # List recent log files
```

### Configuration Commands
```
config wifi SSID PASSWORD           # Set WiFi credentials
config timezone EST5EDT,M3.2.0,M11.1.0  # Set timezone
time 2024-12-24 15:30:00            # Set time manually
```

### USB Mass Storage Commands
```
msc start               # Start USB flash drive mode
msc stop                # Stop USB flash drive mode
msc status              # Show USB drive status
```

### Display Commands
```
display refresh 10      # Set display refresh rate (1-60 Hz)
display compact on      # Enable compact display mode
display compact off     # Disable compact display mode
display status          # Show display configuration
```

## 💾 USB Flash Drive Mode

### Activate Flash Drive
1. **Connect via USB commands** (see above)
2. **Start USB Mass Storage**:
   ```
   ProxiTof> msc start
   ```
3. **Check Windows File Explorer** - SD card should appear as removable drive
4. **Edit files** directly on the drive (config.txt, view log files)
5. **Stop when done**:
   ```
   ProxiTof> msc stop
   ```

### Important Notes
- **Data logging pauses** while USB drive is active
- **Always stop MSC** before disconnecting USB
- **SD card must be FAT32** formatted
- **Files are immediately accessible** - no need to eject

## 🔧 Common Usage Examples

### Example 1: Change WiFi Settings
```bash
# Method A: Via USB commands
python tools/proxitof_usb_tool.py --interactive
ProxiTof> config wifi MyWiFi MyPassword

# Method B: Via config file
ProxiTof> msc start
# Edit /config.txt in File Explorer
# Change WIFI_SSID=MyWiFi and WIFI_PASSWORD=MyPassword
ProxiTof> msc stop
```

### Example 2: Change Display Speed
```bash
# Via USB commands (immediate effect)
ProxiTof> display refresh 30

# Via config file (takes effect after restart)
ProxiTof> msc start
# Edit /config.txt: DISPLAY_REFRESH_HZ=30
ProxiTof> msc stop
```

### Example 3: Download Log Files
```bash
# Activate USB drive
ProxiTof> msc start

# Copy files from drive in File Explorer
# Files are in folders like: /2024-12-24/log_20241224_143022.csv

# Stop when done
ProxiTof> msc stop
```

### Example 4: Sync Time
```bash
# Method A: Using dedicated time sync tool (recommended)
python tools/usb_time_sync.py

# Method B: Using main USB tool
python tools/proxitof_usb_tool.py --sync-time

# Method C: Via interactive mode
ProxiTof> sync

# Method D: Set specific time
python tools/usb_time_sync.py --time "2024-12-24 15:30:00"
```

### Example 5: USB Time Sync (No Internet Required)
```bash
# Windows - use batch file (auto-installs dependencies)
tools\sync_time.bat

# Manual Python usage
python tools/usb_time_sync.py

# Test connection first
python tools/usb_time_sync.py --test

# List available ports
python tools/usb_time_sync.py --list-ports

# Specify port manually
python tools/usb_time_sync.py --port COM3
```

## 🛠️ Troubleshooting

### USB Drive Not Appearing
1. **Check USB connection** - try different cable/port
2. **Start MSC manually**:
   ```
   ProxiTof> msc start
   ```
3. **Check SD card** - must be inserted and FAT32 formatted
4. **Check status**:
   ```
   ProxiTof> msc status
   ```

### Commands Not Working
1. **Check connection**:
   ```bash
   python tools/proxitof_usb_tool.py --status
   ```
2. **Try different COM port** - check Device Manager
3. **Check baud rate** - must be 115200
4. **Restart device** - power cycle ESP32

### SD Card Issues
1. **Format as FAT32** on computer first
2. **Check card size** - use 32GB or smaller for best compatibility
3. **Try different SD card** - some cards have compatibility issues

## 📋 Command Reference

### Python Tool Usage
```bash
# Show device status
python tools/proxitof_usb_tool.py --status

# Set WiFi credentials
python tools/proxitof_usb_tool.py --wifi "MyWiFi" "MyPassword"

# Set timezone
python tools/proxitof_usb_tool.py --timezone "EST5EDT,M3.2.0,M11.1.0"

# Sync time with computer
python tools/proxitof_usb_tool.py --sync-time

# Interactive mode (recommended)
python tools/proxitof_usb_tool.py --interactive
```

### USB Time Sync Tool (Dedicated)
```bash
# Windows - simple batch file
tools\sync_time.bat

# Cross-platform Python tool
python tools/usb_time_sync.py

# Advanced usage
python tools/usb_time_sync.py --port COM3 --time "2024-12-24 15:30:00"
python tools/usb_time_sync.py --test                    # Test connection
python tools/usb_time_sync.py --list-ports             # Show available ports
```

### Direct Serial Commands
```
help                                    # Show all commands
status                                  # System status
config                                  # Show configuration
config wifi MyWiFi MyPassword          # Set WiFi
config timezone EST5EDT,M3.2.0,M11.1.0 # Set timezone
time                                    # Show time
time 2024-12-24 15:30:00               # Set time
sync                                    # Sync time
files                                   # List log files
msc start                              # Start USB drive
msc stop                               # Stop USB drive
msc status                             # USB drive status
display refresh 30                     # Set display rate
display compact on                     # Compact mode
display status                         # Display info
```

### USB Time Sync Commands (Direct Serial)
```
TIME:2024-12-24 15:30:00               # Set specific time
STATUS                                  # Get system status
HELP                                    # Show USB commands
```

## 🎯 Quick Workflow

### Daily Usage
1. **Connect USB** → **Check status** → **Start logging**
2. **Change settings** via USB commands or config file
3. **Download data** via USB drive mode
4. **Monitor system** via status commands

### Field Deployment
1. **Pre-configure** via config.txt on SD card
2. **Deploy device** with SD card
3. **Remote access** via USB when needed
4. **Data collection** runs automatically

---

**The USB interface provides complete control over your ProxiTof device!** 🚀