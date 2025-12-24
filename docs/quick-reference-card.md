# ProxiTof Quick Reference Card

## 🚀 Quick Start Checklist

### ✅ Hardware Setup
- [ ] ESP32-S3 powered via USB-C
- [ ] VL53L8CX wired: SDA=8, SCL=9, LPN=4
- [ ] BNO08x wired: MOSI=11, MISO=13, SCK=12, CS=10, INT=5, RST=6
- [ ] DS3231 wired: SDA=21, SCL=22
- [ ] SD card wired: CLK=14, CMD=15, D0=2, D1=16, D2=17, D3=18
- [ ] Button wired: GPIO 3 to GND
- [ ] SD card formatted as FAT32

### ✅ Software Setup
- [ ] ESP-IDF v5.5+ installed
- [ ] Run `export.ps1` before building
- [ ] Build: `idf.py build`
- [ ] Flash: `idf.py flash monitor`

---

## 🔧 Common Commands

### ESP-IDF Build Commands
```bash
# Activate ESP-IDF environment (REQUIRED every time)
C:\Users\[USERNAME]\esp\v5.5.1\esp-idf\export.ps1

# Build and flash
idf.py build flash monitor

# Clean build
idf.py fullclean build

# Monitor only
idf.py monitor
```

### USB Interface Commands
```bash
# Python tool (recommended)
python tools/proxitof_usb_tool.py --status
python tools/proxitof_usb_tool.py --wifi "SSID" "Password"
python tools/proxitof_usb_tool.py --sync-time

# Serial terminal commands
help                    # Show available commands
status                  # System status
config wifi SSID PASS  # Set WiFi
time 2024-12-24 15:30:00 # Set time
sync                    # NTP sync
```

---

## 📁 File Locations

### Configuration
- **SD Card Config**: `/sdcard/config.txt`
- **Python Tool**: `tools/proxitof_usb_tool.py`
- **Documentation**: `docs/README.md`

### Data Files
- **Log Files**: `/sdcard/YYYY-MM-DD/log_YYYYMMDD_HHMMSS.csv`
- **Config Backup**: `/sdcard/config.txt`

---

## ⚡ Troubleshooting

### Build Issues
| Problem | Solution |
|---------|----------|
| `idf.py not recognized` | Run `export.ps1` first |
| Build errors | Try `idf.py fullclean build` |
| Component errors | Check `components/` folder |

### Hardware Issues
| Problem | Solution |
|---------|----------|
| ToF not detected | Check I2C wiring (SDA=8, SCL=9, LPN=4) |
| IMU not responding | Verify SPI wiring and P0=P1=3.3V |
| SD card failed | Format as FAT32, check SDMMC wiring |
| Time wrong | Set via USB: `time 2024-12-24 15:30:00` |

### Configuration Issues
| Problem | Solution |
|---------|----------|
| WiFi not connecting | Edit `/sdcard/config.txt` |
| USB not working | Install Python + pyserial |
| Config not loading | Check config.txt syntax |

---

## 📊 Status Indicators

### LED Status
- 🔵 **Blue**: System initializing
- 🟢 **Green**: Normal operation (>50cm)
- 🟡 **Yellow**: Medium distance (10-50cm)
- 🔴 **Red**: Close proximity (<10cm)

### Serial Monitor Messages
- `✓ Sensor manager initialized` - Sensors working
- `✓ SD card initialized` - Storage ready
- `✓ Time synchronized` - Clock accurate
- `⚠️ DS3231 not available` - RTC issue
- `❌ SD card failed` - Storage problem

---

## 🔄 Field Deployment Workflow

### Pre-Deployment (Lab)
1. **Hardware Test**: Verify all sensors working
2. **Time Setup**: Set accurate time via USB or WiFi
3. **Config Prepare**: Edit `/sdcard/config.txt` for field
4. **Data Test**: Verify logging to SD card

### Deployment (Field)
1. **Power On**: Connect power, wait for initialization
2. **Status Check**: Green LED = ready for data collection
3. **Time Verify**: Check timestamps in log files
4. **Operation**: System runs autonomously

### Maintenance (Field)
1. **USB Connect**: Connect laptop via USB-C cable
2. **Status Check**: `python proxitof_usb_tool.py --status`
3. **Time Sync**: `python proxitof_usb_tool.py --sync-time`
4. **Config Update**: Edit settings via USB interface

---

## 📋 Configuration Template

### `/sdcard/config.txt` Example
```ini
# WiFi Configuration
WIFI_ENABLED=true
WIFI_SSID=YourWiFiNetwork
WIFI_PASSWORD=YourWiFiPassword

# Time Configuration  
TIMEZONE=UTC0
SYNC_INTERVAL_HOURS=24

# Hardware Configuration
DS3231_ENABLED=true
DS3231_SDA_PIN=21
DS3231_SCL_PIN=22
```

### Common Timezones
- **UTC**: `UTC0`
- **US Eastern**: `EST5EDT,M3.2.0,M11.1.0`
- **US Pacific**: `PST8PDT,M3.2.0,M11.1.0`
- **Europe/Berlin**: `CET-1CEST,M3.5.0,M10.5.0/3`

---

## 🎯 Performance Targets

### Normal Operation
- **ToF Rate**: 60 Hz (16.7ms intervals)
- **IMU Rate**: 100 Hz (10ms intervals)
- **Display**: 2 Hz updates
- **SD Write**: Every 500ms (batched)
- **Time Sync**: Every 24 hours (configurable)

### Data Quality
- **Time Accuracy**: ±2ppm with DS3231
- **Distance Range**: 0-4000mm (ToF)
- **IMU Range**: ±16g accel, ±2000°/s gyro
- **File Size**: ~100MB per file (auto-rotate)

---

## 🆘 Emergency Procedures

### System Not Responding
1. **Power Cycle**: Disconnect and reconnect power
2. **USB Check**: Try different USB cable/port
3. **SD Card**: Remove and reinsert SD card
4. **Factory Reset**: Reflash firmware

### Data Recovery
1. **SD Card**: Remove and read on computer
2. **File Check**: Look for `/YYYY-MM-DD/` folders
3. **CSV Validation**: Open files in Excel/Python
4. **Backup**: Copy important data immediately

### Time Issues
1. **USB Sync**: `python proxitof_usb_tool.py --sync-time`
2. **Manual Set**: `time 2024-12-24 15:30:00`
3. **DS3231 Check**: Verify RTC wiring and battery
4. **WiFi Sync**: Configure WiFi for NTP

---

## 📞 Support Resources

### Documentation
- **Complete Guide**: `docs/README.md`
- **System Overview**: `docs/system-overview.md`
- **USB Interface**: `docs/usb-interface-guide.md`
- **Time Management**: `docs/smart-time-system-guide.md`

### Tools
- **Python Tool**: `tools/proxitof_usb_tool.py`
- **Build System**: ESP-IDF v5.5+
- **Serial Monitor**: Built into ESP-IDF

### Hardware Specs
- **ESP32-S3**: DevKitC-1 (16MB Flash, 8MB PSRAM)
- **ToF Sensor**: VL53L8CX (4x4 array, I2C 0x29)
- **IMU Sensor**: BNO08x (9-DOF, SPI)
- **RTC Module**: DS3231 (I2C 0x68, battery-backed)

---

**Keep this reference card handy for quick troubleshooting and field operations!** 📋