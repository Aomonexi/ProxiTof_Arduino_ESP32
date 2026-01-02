# ProxiTof Quick Reference Card

## 🚀 **Essential Commands** 🆕

### **System Status**
```bash
status                          # Complete system overview
sensors                         # Hardware health check
performance                     # CPU and memory usage
```

### **Time Management**
```bash
time                           # Show current time
sync                           # Sync with NTP/computer
config timezone CST-8          # Set China timezone
```

### **WiFi Configuration**
```bash
config wifi "MyWiFi" "MyPass"  # Set WiFi credentials
config                         # Show current config
```

### **USB Mass Storage**
```bash
msc start                      # SD card as USB drive
msc stop                       # Resume data logging
msc status                     # Check MSC status
```

### **Display Control** 🆕
```bash
display refresh 30             # Set 30Hz refresh rate
display compact on             # Enable compact mode
display status                 # Show display stats
```

### **Event Flagging**
```bash
flag                          # Manual event flag
button_status                 # Button press history
```

### **File Management**
```bash
files                         # List log files
msc start                     # Access via USB drive
```

---

## 🔌 **Automatic USB Detection** 🆕

| Connection Type | System Behavior | LED Indicator |
|----------------|------------------|---------------|
| **Computer** | Auto USB MSC mode | Blue flash |
| **Power Bank** | Continue logging | Green breathing |
| **Battery** | Normal operation | Green breathing |

---

## 📱 **Display Refresh Rates** 🆕

| Rate | CPU Usage | Best For |
|------|-----------|----------|
| **10 Hz** | 2-5% | Battery saving |
| **30 Hz** | 5-10% | Balanced performance |
| **60 Hz** | 15-20% | Smooth monitoring |

---

## 💾 **SD Card File Structure**

```
/sdcard/
├── config.txt                 # System configuration
├── 20241231/                 # Today's logs
│   ├── log_090000.csv        # Morning data
│   └── log_150000.csv        # Afternoon data
└── 20241230/                 # Yesterday's logs
```

---

## 🎛️ **Configuration File** (`/sdcard/config.txt`)

```ini
# WiFi Settings
WIFI_ENABLED=true
WIFI_SSID=YourWiFiName
WIFI_PASSWORD=YourPassword

# Time Settings
TIMEZONE=CST-8
SYNC_INTERVAL_HOURS=24

# Display Settings 🆕
DISPLAY_REFRESH_HZ=30
DISPLAY_COMPACT_MODE=false

# Hardware Settings
DS3231_ENABLED=true
DS3231_SDA_PIN=8
DS3231_SCL_PIN=9
```

---

## 🔧 **Hardware Connections**

### **Sensors**
- **ToF VL53L8CX**: SDA=8, SCL=9, LPN=4
- **IMU BNO08x**: MOSI=11, MISO=13, SCK=12, CS=10
- **Button**: GPIO=3 (with pull-up)
- **LED**: GPIO=48 (built-in WS2812)

### **Storage & Time**
- **SD Card**: CLK=14, CMD=15, D0=2, D1=16, D2=17, D3=18
- **DS3231 RTC**: SDA=8, SCL=9 (shared with ToF)

---

## 🚨 **Troubleshooting**

### **Common Issues**
| Problem | Quick Fix | Command |
|---------|-----------|---------|
| Wrong time | Sync time | `sync` |
| No WiFi | Set credentials | `config wifi SSID PASS` |
| SD card full | Check files | `files` then `msc start` |
| Slow display | Reduce rate | `display refresh 15` |
| USB not working | Check connection | `msc status` |

### **Emergency Commands**
```bash
restart                        # Software reset
factory_reset                  # Return to defaults
msc stop                      # Force resume logging
```

---

## 📊 **LED Status Indicators**

| Color/Pattern | Meaning | Action |
|--------------|---------|--------|
| **Blue solid** | Initializing | Wait |
| **Green breathing** | Logging active | Normal |
| **Blue flash** | USB MSC active | File access mode |
| **Yellow flash** | Button pressed | Event flagged |
| **Red solid** | Close proximity | Check placement |
| **Red blinking** | SD card error | Check card |

---

## 🎯 **Quick Start Workflow**

### **Initial Setup**
```bash
1. status                      # Check system
2. config wifi "Net" "Pass"    # Set WiFi
3. sync                        # Sync time
4. sensors                     # Verify hardware
```

### **Data Collection**
```bash
1. Connect USB → Auto USB drive mode
2. Copy files from USB drive
3. Disconnect USB → Auto resume logging
4. Use button for event flagging
```

### **Performance Tuning**
```bash
1. display refresh 60          # Smooth monitoring
2. display status              # Check CPU usage
3. performance                 # System metrics
4. display compact on          # If CPU high
```

---

## 💡 **Pro Tips**

- **🔌 Automatic USB**: Just plug in - no commands needed!
- **📱 Display Tuning**: Start with 30Hz, adjust based on CPU usage
- **💾 File Access**: USB drive mode pauses logging (by design)
- **⏰ Time Sync**: Happens automatically with WiFi
- **🔘 Event Flagging**: Hold button during entire event
- **📊 Monitoring**: Use `status` and `performance` regularly

---

## 📞 **Getting Help**

```bash
help                          # Complete command list
status                        # System overview
display status                # Display performance
sensors                       # Hardware health
performance                   # System metrics
```

**Serial Settings**: 115200 baud, 8N1, no flow control  
**USB Cable**: Must support data (not power-only)

---

**🆕 New Features**: Automatic USB detection, configurable display refresh rates, enhanced command system  
**Last Updated**: December 31, 2024