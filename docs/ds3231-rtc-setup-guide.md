# DS3231 Real-Time Clock Setup Guide

## Why DS3231 is Better Than WiFi

| Feature | DS3231 RTC | WiFi + NTP |
|---------|------------|------------|
| **Battery Backup** | ✅ Keeps time when powered off | ❌ Resets every boot |
| **Field Deployment** | ✅ No internet needed | ❌ Requires WiFi/internet |
| **Reliability** | ✅ Always works | ❌ Depends on network |
| **Power Consumption** | ✅ Very low | ❌ High during sync |
| **Setup Complexity** | ✅ Set once, works forever | ❌ Configure WiFi every location |
| **Accuracy** | ✅ ±2ppm (very accurate) | ✅ Very accurate when synced |

## DS3231 Module Wiring

### Standard Wiring (Shared I2C Bus):
```
DS3231 Module     ESP32-S3
-------------     --------
VCC            -> 3.3V
GND            -> GND
SDA            -> GPIO 8  (Shared with VL53L8CX ToF)
SCL            -> GPIO 9  (Shared with VL53L8CX ToF)
SQW            -> NOT CONNECTED (optional)
32K            -> NOT CONNECTED (optional)
```

**I2C Bus Sharing:**
- DS3231 RTC: I2C address **0x68**
- VL53L8CX ToF: I2C address **0x29**
- Both devices use the same SDA/SCL pins (GPIO 8/9)
- No conflicts - different I2C addresses

### Pin Configuration in Code:
Edit `main/main.c`:
```c
// DS3231 RTC Configuration (shared I2C bus with ToF sensor)
#define RTC_SDA_PIN GPIO_NUM_8   // Shared with VL53L8CX ToF
#define RTC_SCL_PIN GPIO_NUM_9   // Shared with VL53L8CX ToF
```

## Initial Time Setup

### Method 1: Manual Time Setting (Recommended)

1. **Edit main.c** and uncomment this line:
   ```c
   // UNCOMMENT BELOW TO SET TIME MANUALLY (first time setup)
   rtc_ds3231_set_time_string("2024-12-24 15:30:00");  // <- Remove //
   ```

2. **Set current time** (format: "YYYY-MM-DD HH:MM:SS"):
   ```c
   rtc_ds3231_set_time_string("2024-12-24 15:30:00");
   ```

3. **Build and flash**:
   ```bash
   idf.py build flash monitor
   ```

4. **Comment out the line again** after first run:
   ```c
   // rtc_ds3231_set_time_string("2024-12-24 15:30:00");  // <- Add // back
   ```

5. **Rebuild and flash** - time is now permanently set!

### Method 2: WiFi + NTP Setup (One-time)

If you want to set time via internet once:

1. **Connect DS3231** and **WiFi** temporarily
2. **Use both systems** to sync DS3231 with NTP
3. **Remove WiFi code** after DS3231 is set
4. **DS3231 maintains time** forever

## Expected Output

### Success Case:
```
I (1234) DS3231: Initializing DS3231 RTC...
I (1235) DS3231: I2C pins: SDA=8, SCL=9 (shared with ToF)
I (1236) DS3231: ✓ DS3231 RTC initialized successfully
I (1237) DS3231: ✓ System time synced: 2024-12-24 15:30:45
I (1238) MAIN: ✓ DS3231 RTC initialized successfully
I (1239) MAIN: ✓ System time synchronized with DS3231
I (1240) MAIN: Current time: 2024-12-24 15:30:45
I (2345) STORAGE_MGR: Using synchronized time: 2024-12-24
I (2346) STORAGE_MGR: Created directory: /sdcard/2024-12-24
```

### Failure Case:
```
E (1234) DS3231: DS3231 not responding at address 0x68
E (1235) DS3231: Failed to add DS3231 device: ESP_ERR_NOT_FOUND
W (1236) MAIN: ❌ DS3231 RTC initialization failed: ESP_ERR_NOT_FOUND
W (1237) MAIN: Troubleshooting:
W (1238) MAIN:   1. Check DS3231 wiring: SDA=8, SCL=9 (shared with ToF)
W (1239) MAIN:   2. Verify DS3231 power (3.3V)
W (1240) MAIN:   3. Check I2C pull-up resistors (4.7kΩ)
W (1241) MAIN:   4. Verify ToF sensor is also working (shared I2C bus)
```

## Troubleshooting

### DS3231 Not Detected
- ✅ **Check wiring** - SDA, SCL, VCC, GND
- ✅ **Verify power** - 3.3V or 5V (both work)
- ✅ **I2C address** - Should be 0x68
- ✅ **Pull-up resistors** - 4.7kΩ on SDA/SCL (usually built-in)
- ✅ **Try different pins** - Change RTC_SDA_PIN/RTC_SCL_PIN

### Time Not Keeping
- ✅ **Battery installed** - CR2032 coin cell
- ✅ **Battery voltage** - Should be >2.5V
- ✅ **First time setup** - Set time manually once

### Wrong Time Zone
- ✅ **Set local time** - DS3231 stores local time, not UTC
- ✅ **Manual adjustment** - Add/subtract hours for timezone

## Hardware Requirements

### DS3231 Module Features:
- **Real-time clock** with seconds, minutes, hours, day, date, month, year
- **Battery backup** (CR2032) - keeps time for years
- **Temperature sensor** (bonus feature)
- **High accuracy** - ±2ppm (±1 minute per year)
- **I2C interface** - easy to connect

### Recommended Modules:
- **Generic DS3231 breakout** - $2-5
- **Adafruit DS3231** - Higher quality
- **Waveshare DS3231** - Good documentation

## File Organization

With DS3231 RTC, your files will be properly organized:

```
/sdcard/
├── 2024-12-24/
│   ├── log_20241224_153045.csv
│   └── log_20241224_160122.csv
├── 2024-12-25/
│   └── log_20241225_090315.csv
└── 2024-12-26/
    ├── log_20241226_080000.csv
    └── log_20241226_140000.csv
```

## Power Consumption

- **DS3231 active**: ~100µA
- **DS3231 battery backup**: ~1µA (years of operation)
- **ESP32 impact**: Negligible (I2C communication is brief)

## Advantages for Your Project

1. **Field deployment** - No WiFi setup needed at each location
2. **Reliable logging** - Always correct timestamps
3. **Data analysis** - Properly organized by date
4. **Battery backup** - Time survives power outages
5. **Simple setup** - Set time once, works forever

## Next Steps

1. **Wire DS3231** according to diagram above
2. **Set current time** using manual method
3. **Test logging** - verify correct timestamps
4. **Deploy in field** - no additional setup needed!

The DS3231 is the perfect solution for your near-miss detection system!