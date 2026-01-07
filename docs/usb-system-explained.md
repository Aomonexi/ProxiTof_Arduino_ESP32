# USB System Explained - ProxiTof Device

## 🔌 USB Port Configuration

Your ESP32-S3 DevKit-C has **TWO USB ports** with different functions:

### RIGHT Port (COM/UART)
- **Purpose**: Programming, monitoring, debugging
- **Hardware**: CH343 USB-to-Serial chip
- **Always available**: Works without USB-OTG bridge
- **Use for**: 
  - Flashing firmware (`idf.py flash`)
  - Serial monitoring (`idf.py monitor`)
  - USB time sync commands
  - Configuration via serial terminal

### LEFT Port (USB/Native)
- **Purpose**: USB Mass Storage (flash drive mode)
- **Hardware**: ESP32-S3 native USB (requires USB-OTG bridge)
- **Requires**: USB-OTG bridge must be soldered
- **Use for**:
  - Automatic flash drive access to SD card
  - File transfer and data download
  - Direct SD card access from computer

## 🎯 How It Works

### Normal Operation Mode
- **LED Color**: Green/Yellow/Red (based on distance)
- **Function**: Sensor data collection and logging
- **USB**: RIGHT port can be used for commands/monitoring
- **Data**: Continuously logs to SD card

### USB Mass Storage Mode (Flash Drive)
- **LED Color**: **BLUE** (indicates USB mode)
- **Function**: SD card appears as removable drive
- **USB**: LEFT port connected to computer
- **Data**: Logging **PAUSED** to prevent corruption
- **Sensors**: **PAUSED** to reduce CPU usage

## 🔄 Automatic Mode Switching

The system **automatically detects** when you connect the LEFT USB port:

1. **Connect LEFT USB** → System detects computer connection
2. **Wait 1 second** → Connection stabilizes (debounce)
3. **LED turns BLUE** → USB Mass Storage starts
4. **SD card appears** → Access files in Windows Explorer
5. **Disconnect LEFT USB** → System returns to normal mode
6. **LED returns to distance colors** → Logging resumes

## 💡 LED Color Meanings

| LED Color | Mode | Description |
|-----------|------|-------------|
| 🔴 **RED** | Normal | Object < 10cm (close) |
| 🟡 **YELLOW** | Normal | Object 10-50cm (medium) |
| 🟢 **GREEN** | Normal | Object > 50cm (far) |
| 🔵 **BLUE** | USB MSC | Flash drive mode active |

## 📋 Usage Scenarios

### Scenario 1: Programming and Monitoring
```
RIGHT USB connected → Programming/monitoring
LEFT USB disconnected → Normal sensor operation
LED: Distance colors (Red/Yellow/Green)
```

### Scenario 2: File Access
```
RIGHT USB disconnected → No programming
LEFT USB connected → Flash drive mode
LED: Blue (USB mode indicator)
```

### Scenario 3: Both Connected
```
RIGHT USB connected → Programming available
LEFT USB connected → Flash drive mode active
LED: Blue (USB mode takes priority)
```

### Scenario 4: Field Deployment
```
Both USB disconnected → Standalone operation
LED: Distance colors
Data: Continuous logging to SD card
```

## ⚠️ Important Notes

### Data Safety
- **Logging automatically pauses** when LEFT USB is connected
- **No data corruption** risk during file access
- **Logging resumes** when LEFT USB is disconnected

### Connection Requirements
- **USB-OTG bridge must be soldered** for LEFT port to work
- **RIGHT port works immediately** (no soldering required)
- **Use quality USB cables** for reliable connection

### File Access
- **SD card appears as removable drive** when LEFT USB connected
- **Access log files directly** in Windows Explorer
- **Edit config.txt** directly on the drive
- **No need to eject** - just disconnect USB when done

## 🛠️ Troubleshooting

### Flash Drive Not Appearing
1. **Check USB-OTG bridge** - must be soldered
2. **Try different USB cable** - some are power-only
3. **Check LED color** - should be BLUE when connected
4. **Wait 2-3 seconds** - detection takes time
5. **Check SD card** - must be inserted and FAT32 formatted

### Flash Drive Disappears Quickly
- **This is normal behavior** if connection is unstable
- **Use quality USB cable** with good data connection
- **Check USB port** - some ports are power-only
- **LED should stay BLUE** when properly connected

### Time Sync Not Working
1. **Use RIGHT USB port** for time sync commands
2. **Check serial connection** with `python tools/usb_time_sync.py --test`
3. **Try different COM port** if auto-detection fails
4. **Use manual port** with `--port COM3` option

## 🎉 Quick Test Procedure

### Test USB Time Sync (RIGHT Port)
```bash
# Connect RIGHT USB port
python tools/usb_time_sync.py --test --port COM4

# If working, sync time
python tools/usb_time_sync.py --port COM4
```

### Test USB Mass Storage (LEFT Port)
```bash
# 1. Connect LEFT USB port
# 2. Wait for LED to turn BLUE
# 3. Check Windows Explorer for new drive
# 4. Access files directly
# 5. Disconnect when done - LED returns to distance colors
```

---

**The dual USB system gives you the best of both worlds: easy programming/monitoring AND convenient file access!** 🚀