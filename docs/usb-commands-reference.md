# ProxiTof USB Commands Reference

## 📋 Complete Command List

This document contains **all available USB serial commands** for the ProxiTof system. Connect via USB cable and use serial terminal (115200 baud) to access these commands.

---

## 🔧 **System Commands**

### **help**
- **Description**: Show all available commands with examples
- **Usage**: `help`
- **Output**: Complete command list and usage examples

### **status**
- **Description**: Display comprehensive system status
- **Usage**: `status`
- **Output**: 
  ```
  === System Status ===
  Time: 2024-12-31 13:31:45
  Time Source: DS3231 RTC
  USB Connection: Computer Connected
  Mode: Data Logging Active
  SD Card: 15.2 GB free
  Sensors: ToF=HEALTHY, IMU=HEALTHY
  ```

---

## ⏰ **Time Management Commands**

### **time** (Display Current Time)
- **Description**: Show current system time and source
- **Usage**: `time`
- **Output**: Current time with timezone and time source information

### **time** (Set Manual Time)
- **Description**: Set system time manually
- **Usage**: `time YYYY-MM-DD HH:MM:SS`
- **Examples**: 
  - `time 2024-12-31 13:30:00`
  - `time 2024-01-15 09:45:30`
- **Note**: Use when WiFi/NTP is not available

### **sync**
- **Description**: Synchronize time with computer or NTP server
- **Usage**: `sync`
- **Action**: Attempts WiFi NTP sync or uses computer time
- **Output**: Confirmation of time synchronization

---

## 🌐 **Configuration Commands**

### **config** (Show Configuration)
- **Description**: Display current system configuration
- **Usage**: `config`
- **Output**: WiFi settings, timezone, hardware pins, etc.

### **config wifi**
- **Description**: Set WiFi credentials for time synchronization
- **Usage**: `config wifi SSID PASSWORD`
- **Examples**:
  - `config wifi MyHomeWiFi MyPassword123`
  - `config wifi "Office Network" "Complex Password!"`
- **Note**: Use quotes for SSIDs/passwords with spaces

### **config timezone**
- **Description**: Set system timezone
- **Usage**: `config timezone TIMEZONE_STRING`
- **Examples**:
  - `config timezone CST-8` (China Standard Time)
  - `config timezone EST5EDT,M3.2.0,M11.1.0` (US Eastern with DST)
  - `config timezone UTC0` (UTC time)
  - `config timezone JST-9` (Japan Standard Time)

---

## 💾 **USB Mass Storage Commands**

### **msc start**
- **Description**: Start USB Mass Storage mode (SD card as USB drive)
- **Usage**: `msc start`
- **Action**: 
  - Pauses data logging
  - Exposes SD card as USB drive to computer
  - Allows direct file access and editing
- **Note**: Data logging automatically resumes when stopped

### **msc stop**
- **Description**: Stop USB Mass Storage mode and resume data logging
- **Usage**: `msc stop`
- **Action**:
  - Returns SD card control to ESP32
  - Resumes sensor data logging
  - Closes USB drive access

### **msc status**
- **Description**: Show USB Mass Storage status
- **Usage**: `msc status`
- **Output**:
  ```
  === USB Mass Storage Status ===
  MSC Active: ✓ YES / ✗ NO
  USB Connected: ✓ YES / ✗ NO
  Data Logging: ▶️ ACTIVE / ⏸️ PAUSED
  ```

### **msc auto**
- **Description**: Re-enable automatic USB detection (if disabled)
- **Usage**: `msc auto`
- **Action**: Restores automatic USB MSC switching based on connection type

---

## 📱 **Display Control Commands**

### **display refresh**
- **Description**: Set display refresh rate for screen output
- **Usage**: `display refresh HZ`
- **Range**: 1-60 Hz
- **Examples**:
  - `display refresh 30` (30 FPS - balanced performance)
  - `display refresh 60` (60 FPS - smooth but higher CPU usage)
  - `display refresh 10` (10 FPS - low CPU usage)
- **Note**: Higher refresh rates use more CPU

### **display compact**
- **Description**: Enable/disable compact display mode
- **Usage**: `display compact on/off`
- **Examples**:
  - `display compact on` (Single-line updates, minimal CPU)
  - `display compact off` (Full graphics with borders and colors)

### **display status**
- **Description**: Show display performance statistics
- **Usage**: `display status`
- **Output**:
  ```
  === Display Status ===
  Refresh Rate: 30 Hz (actual: 29 FPS)
  CPU Usage: 8%
  Frames Rendered: 15420
  Performance: GOOD
  ```

---

## 📁 **File Management Commands**

### **files**
- **Description**: List recent log files and SD card information
- **Usage**: `files`
- **Output**: Recent CSV files, file sizes, and SD card usage
- **Note**: Use `msc start` to access files directly via USB drive

---

## 🔄 **Advanced Commands**

### **restart**
- **Description**: Restart the ESP32 system
- **Usage**: `restart`
- **Action**: Performs software reset (equivalent to power cycle)

### **factory_reset**
- **Description**: Reset system to factory defaults
- **Usage**: `factory_reset`
- **Action**: Clears all configuration and restarts with defaults
- **Warning**: This will erase all custom settings

### **sensors**
- **Description**: Show detailed sensor health and statistics
- **Usage**: `sensors`
- **Output**: ToF and IMU sensor status, error counts, performance metrics

### **memory**
- **Description**: Display memory usage statistics
- **Usage**: `memory`
- **Output**: RAM usage, heap status, stack usage

### **performance**
- **Description**: Show system performance metrics
- **Usage**: `performance`
- **Output**: CPU usage breakdown, task statistics, timing information

---

## 📊 **Diagnostic Commands**

### **sensors**
- **Description**: Show detailed sensor health and statistics
- **Usage**: `sensors`
- **Output**: ToF and IMU sensor status, error counts, performance metrics

### **memory**
- **Description**: Display memory usage statistics
- **Usage**: `memory`
- **Output**: RAM usage, heap status, stack usage

### **performance**
- **Description**: Show system performance metrics
- **Usage**: `performance`
- **Output**: CPU usage breakdown, task statistics, timing information

---

## 🎛️ **Button and Flagging Commands**

### **flag**
- **Description**: Manually flag current timestamp for event marking
- **Usage**: `flag`
- **Action**: Marks current time as manual event (same as button press)
- **Note**: Used for remote event flagging via USB

### **button_status**
- **Description**: Show button status and recent press history
- **Usage**: `button_status`
- **Output**: Button state, press count, last press timestamp

---

## 📝 **Command Usage Examples**

### **Quick Setup Sequence**:
```bash
# 1. Check system status
status

# 2. Set WiFi for time sync
config wifi "MyWiFi" "MyPassword"

# 3. Set timezone
config timezone CST-8

# 4. Sync time
sync

# 5. Start USB drive mode for file access
msc start
```

### **Display Optimization**:
```bash
# For high-performance display
display refresh 60
display compact off

# For low CPU usage
display refresh 15
display compact on

# Check performance impact
display status
performance
```

### **Data Collection Workflow**:
```bash
# Check sensors before deployment
sensors
status

# Access data files
msc start
# (copy files via USB drive)
msc stop

# Resume logging
status
```

---

## 🔧 **Command Categories Summary**

| Category | Commands | Purpose |
|----------|----------|---------|
| **System** | `help`, `status`, `restart` | Basic system control |
| **Time** | `time`, `sync`, `config timezone` | Time management |
| **WiFi** | `config wifi` | Network configuration |
| **USB MSC** | `msc start/stop/status/auto` | File access control |
| **Display** | `display refresh/compact/status` | Screen output control |
| **Files** | `files`, `msc start` | Data file management |
| **Sensors** | `sensors`, `button_status` | Hardware monitoring |
| **Flagging** | `flag` | Event marking |
| **Diagnostics** | `memory`, `performance` | System analysis |

---

## 💡 **Tips and Best Practices**

### **For Field Deployment**:
1. Use `config wifi` to set local network credentials
2. Use `sync` to ensure accurate time before deployment
3. Use `sensors` to verify all hardware is working
4. Use `msc start` periodically to download data

### **For Lab Development**:
1. Use `display refresh 60` for smooth real-time monitoring
2. Use `performance` to monitor system load
3. Use `msc start` for quick file access
4. Use `flag` for remote event marking during tests

### **For Troubleshooting**:
1. Use `status` to check overall system health
2. Use `sensors` to identify hardware issues
3. Use `memory` to check for memory leaks
4. Use `restart` to recover from errors

---

## 🚨 **Important Notes**

- **USB Connection Required**: All commands require USB cable connection to computer
- **Serial Settings**: 115200 baud, 8N1, no flow control
- **Case Sensitive**: Commands are case-sensitive (use lowercase)
- **Automatic Features**: Many functions work automatically (USB detection, time sync)
- **Manual Override**: Commands can override automatic behavior when needed
- **Data Safety**: System prevents data loss during USB MSC operations

---

**This reference is updated with each new feature addition. Last updated: December 31, 2024**