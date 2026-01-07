# Field Configuration Guide

## 🎯 Overview

The ProxiTof system supports **field configuration** without recompiling code. Simply edit a text file on the SD card to change WiFi settings, time sync intervals, and hardware pins.

## 📁 Configuration File Location

**File**: `/sdcard/config.txt`  
**Format**: Simple key=value pairs  
**Editing**: Remove SD card, edit on computer, reinsert

## 📝 Configuration File Format

```ini
# ProxiTof System Configuration
# Edit this file to change settings without recompiling
# Remove SD card, edit on computer, insert back

# WiFi Configuration
WIFI_ENABLED=true
WIFI_SSID=YourWiFiNetwork
WIFI_PASSWORD=YourWiFiPassword

# Time Configuration
TIMEZONE=UTC0
SYNC_INTERVAL_HOURS=24

# DS3231 RTC Configuration
DS3231_ENABLED=true
DS3231_SDA_PIN=21
DS3231_SCL_PIN=22

# Common Timezone Examples:
# UTC: UTC0
# US Eastern: EST5EDT,M3.2.0,M11.1.0
# US Pacific: PST8PDT,M3.2.0,M11.1.0
# Europe/Berlin: CET-1CEST,M3.5.0,M10.5.0/3
# Asia/Tokyo: JST-9
```

## 🔧 Configuration Options

### WiFi Settings
| Parameter | Description | Example |
|-----------|-------------|---------|
| `WIFI_ENABLED` | Enable/disable WiFi | `true` or `false` |
| `WIFI_SSID` | WiFi network name | `LabWiFi` |
| `WIFI_PASSWORD` | WiFi password | `SecurePassword123` |

### Time Settings
| Parameter | Description | Example |
|-----------|-------------|---------|
| `TIMEZONE` | Timezone string | `EST5EDT,M3.2.0,M11.1.0` |
| `SYNC_INTERVAL_HOURS` | Hours between NTP sync | `24` (daily), `168` (weekly) |

### Hardware Settings
| Parameter | Description | Example |
|-----------|-------------|---------|
| `DS3231_ENABLED` | Enable/disable DS3231 RTC | `true` or `false` |
| `DS3231_SDA_PIN` | DS3231 SDA GPIO pin | `21` |
| `DS3231_SCL_PIN` | DS3231 SCL GPIO pin | `22` |

### Display Settings
| Parameter | Description | Example |
|-----------|-------------|---------|
| `DISPLAY_REFRESH_HZ` | Display update rate (1-60 Hz) | `2` (low CPU), `10` (smooth) |
| `DISPLAY_COMPACT_MODE` | Compact display layout | `true` or `false` |

**Display Performance Impact**:
- `1-2 Hz`: ~5% CPU usage (recommended for battery)
- `5-10 Hz`: ~15% CPU usage (good balance)
- `30+ Hz`: ~30% CPU usage (very smooth, high power)

## 🚀 Field Deployment Scenarios

### Scenario 1: **Lab Setup → Field Deployment**

**Lab Configuration** (`config.txt`):
```ini
WIFI_ENABLED=true
WIFI_SSID=LabWiFi
WIFI_PASSWORD=LabPassword
SYNC_INTERVAL_HOURS=1
DS3231_ENABLED=true
```

**Field Configuration** (edit `config.txt`):
```ini
WIFI_ENABLED=false
SYNC_INTERVAL_HOURS=168
DS3231_ENABLED=true
```

### Scenario 2: **Multiple Field Locations**

**Location A** (`config.txt`):
```ini
WIFI_ENABLED=true
WIFI_SSID=LocationA_WiFi
WIFI_PASSWORD=LocationA_Pass
TIMEZONE=EST5EDT,M3.2.0,M11.1.0
```

**Location B** (edit same SD card):
```ini
WIFI_ENABLED=true
WIFI_SSID=LocationB_WiFi
WIFI_PASSWORD=LocationB_Pass
TIMEZONE=PST8PDT,M3.2.0,M11.1.0
```

### Scenario 3: **Performance Optimization**

**High Performance** (minimal WiFi usage):
```ini
WIFI_ENABLED=true
SYNC_INTERVAL_HOURS=168    # Weekly sync
DS3231_ENABLED=true
```

**Maximum Accuracy** (frequent sync):
```ini
WIFI_ENABLED=true
SYNC_INTERVAL_HOURS=6      # Every 6 hours
DS3231_ENABLED=true
```

## ⚡ Performance Impact Analysis

### WiFi Sync Performance Impact

| Sync Interval | WiFi Time/Day | Performance Impact | Use Case |
|---------------|---------------|-------------------|----------|
| **1 hour** | 4 minutes | 0.28% | Lab testing |
| **6 hours** | 40 seconds | 0.046% | High accuracy field |
| **24 hours** | 10 seconds | 0.012% | **Recommended** |
| **168 hours** | 1.4 seconds | 0.002% | Long-term deployment |

**Recommendation**: 24 hours provides excellent accuracy with minimal impact.

### ESP32 Internal RTC Reliability

| Duration | Drift (±40ppm) | Reliability | Recommendation |
|----------|----------------|-------------|----------------|
| **1 day** | ±3.5 seconds | ✅ Excellent | Safe without DS3231 |
| **1 week** | ±24 seconds | ✅ Good | Acceptable for most uses |
| **1 month** | ±1.7 minutes | ⚠️ Marginal | Need DS3231 or frequent sync |
| **1 year** | ±21 minutes | ❌ Poor | Must have DS3231 |

## 🔄 Configuration Update Process

### Method 1: **SD Card Removal** (Recommended)
1. **Power off** ESP32
2. **Remove SD card**
3. **Edit config.txt** on computer
4. **Insert SD card** back
5. **Power on** ESP32
6. **Check logs** for new configuration

### Method 2: **Hot Configuration** (Advanced)
- System monitors config file for changes
- Automatically reloads when file is modified
- Useful for remote configuration updates

## 📊 Configuration Validation

The system validates all configuration parameters:

### Valid Ranges:
- **GPIO pins**: 0-48 (ESP32-S3 range)
- **Sync interval**: 1-8760 hours (1 hour to 1 year)
- **WiFi SSID**: 1-63 characters
- **WiFi password**: 0-63 characters

### Error Handling:
- **Invalid values**: Uses defaults with warning
- **Missing file**: Creates default config.txt
- **Corrupted file**: Falls back to defaults

## 🛡️ Security Considerations

### WiFi Password Protection:
- **File permissions**: SD card should be secured
- **Password visibility**: Visible in plain text
- **Alternative**: Use WPA2-Enterprise for corporate networks

### Field Security:
- **Physical access**: Secure SD card slot
- **Configuration lock**: Consider read-only config for production

## 🔍 Troubleshooting

### Configuration Not Loading:
```
W (1234) CONFIG: Config file not found: /sdcard/config.txt
I (1235) CONFIG: Using default configuration
```
**Solution**: Check SD card is inserted and config.txt exists

### Invalid Configuration:
```
W (1234) CONFIG: Invalid line 5: WIFI_SSID YourNetwork
```
**Solution**: Use format `WIFI_SSID=YourNetwork` (with equals sign)

### WiFi Connection Failed:
```
W (1234) SMART_TIME: NTP sync failed: ESP_FAIL
```
**Solution**: Check WIFI_SSID and WIFI_PASSWORD in config.txt

## 📋 Configuration Templates

### **Template 1: Lab Testing**
```ini
WIFI_ENABLED=true
WIFI_SSID=LabWiFi
WIFI_PASSWORD=LabPassword
TIMEZONE=UTC0
SYNC_INTERVAL_HOURS=1
DS3231_ENABLED=true
DS3231_SDA_PIN=21
DS3231_SCL_PIN=22
```

### **Template 2: Field Deployment**
```ini
WIFI_ENABLED=false
TIMEZONE=EST5EDT,M3.2.0,M11.1.0
SYNC_INTERVAL_HOURS=168
DS3231_ENABLED=true
DS3231_SDA_PIN=21
DS3231_SCL_PIN=22
```

### **Template 3: WiFi-Only (No DS3231)**
```ini
WIFI_ENABLED=true
WIFI_SSID=YourWiFi
WIFI_PASSWORD=YourPassword
TIMEZONE=UTC0
SYNC_INTERVAL_HOURS=24
DS3231_ENABLED=false
```

## 🎯 Best Practices

### For Field Deployment:
1. **Test configuration** in lab first
2. **Use DS3231** for reliability
3. **Set reasonable sync intervals** (24-168 hours)
4. **Secure SD card** physically

### For Multiple Locations:
1. **Prepare multiple SD cards** with location-specific configs
2. **Document configurations** for each site
3. **Use consistent naming** for easy identification

### For Long-term Deployment:
1. **Use weekly sync** (168 hours) to minimize WiFi usage
2. **Enable DS3231** for continuous time keeping
3. **Monitor logs** for configuration issues

The field configuration system makes your ProxiTof deployment **flexible and maintainable** without requiring programming knowledge!