# Smart Time Management System Guide

## 🎯 Overview

The Smart Time System provides **robust, multi-tier time management** that automatically handles:

- **DS3231 RTC** (Primary) - Battery-backed external clock
- **ESP32 Internal RTC** (Secondary) - Keeps time when DS3231 fails  
- **WiFi + NTP** (Sync source) - Updates both RTCs automatically
- **Automatic fallback** - Seamless switching between sources
- **Health monitoring** - Detects and recovers from failures

## 🏗️ Architecture

```
┌─────────────────────────────────────────────────────────┐
│                Smart Time Manager                       │
├─────────────────────────────────────────────────────────┤
│  Priority 1: DS3231 RTC (Battery-backed)               │
│  Priority 2: ESP32 Internal RTC (Volatile)             │
│  Priority 3: Default time (Fallback)                   │
├─────────────────────────────────────────────────────────┤
│  Sync Source: WiFi + NTP (Updates all sources)         │
│  Maintenance: Health checks, auto-sync, recovery       │
└─────────────────────────────────────────────────────────┘
```

## ✅ Smart Features

### 1. **Automatic Source Selection**
- Uses DS3231 when available (most accurate)
- Falls back to Internal RTC if DS3231 fails
- Seamless switching without user intervention

### 2. **Health Monitoring**
- Detects DS3231 disconnection/failure
- Automatically recovers when DS3231 reconnects
- Tracks error counts and sync statistics

### 3. **Auto-Sync**
- Syncs with NTP every 24 hours (configurable)
- Updates both DS3231 and Internal RTC
- Disconnects WiFi after sync to save power

### 4. **Graceful Degradation**
- System continues working even if all time sources fail
- Uses reasonable default dates for logging
- Provides clear status information

## 🔧 Configuration

### Hardware Setup

#### Option 1: DS3231 + WiFi (Recommended)
```
DS3231 Module     ESP32-S3
-------------     --------
VCC            -> 3.3V
GND            -> GND
SDA            -> GPIO 21
SCL            -> GPIO 22
```

#### Option 2: WiFi Only
```c
// Disable DS3231, use WiFi + Internal RTC
#define DS3231_SDA_PIN -1        // Disable DS3231
#define DS3231_SCL_PIN -1        // Disable DS3231
#define WIFI_SSID      "YourWiFi"
#define WIFI_PASSWORD  "YourPassword"
```

#### Option 3: DS3231 Only
```c
// Disable WiFi, use DS3231 + manual time setting
#define WIFI_SSID      NULL      // Disable WiFi
#define WIFI_PASSWORD  NULL      // Disable WiFi
```

### Software Configuration

Edit `main/main.c`:

```c
// Smart Time Configuration
#define DS3231_SDA_PIN GPIO_NUM_21   // DS3231 SDA pin (-1 to disable)
#define DS3231_SCL_PIN GPIO_NUM_22   // DS3231 SCL pin (-1 to disable)
#define WIFI_SSID      "YourWiFiNetwork"  // WiFi SSID (NULL to disable)
#define WIFI_PASSWORD  "YourWiFiPassword" // WiFi password (NULL to disable)
#define TIMEZONE       "UTC0"        // Timezone string
```

## 🚀 Usage Scenarios

### Scenario 1: **Field Deployment with DS3231**
1. **Setup**: Wire DS3231, set WiFi credentials
2. **First boot**: Connects to WiFi, syncs DS3231 with NTP
3. **Deployment**: Move to field location (no WiFi)
4. **Operation**: DS3231 provides accurate time
5. **Maintenance**: Occasionally connect to WiFi for re-sync

### Scenario 2: **Lab Testing with WiFi**
1. **Setup**: Configure WiFi, disable DS3231
2. **Operation**: Auto-syncs with NTP every 24 hours
3. **Reliability**: Internal RTC maintains time between syncs

### Scenario 3: **Offline Operation**
1. **Setup**: Wire DS3231, disable WiFi
2. **Time setting**: Set time manually once
3. **Operation**: DS3231 maintains accurate time indefinitely

## 📊 Status Monitoring

The system provides detailed status information:

```c
time_status_t status;
smart_time_get_status(&status);

// Check what's available
bool ds3231_ok = status.ds3231_available;
bool time_synced = status.time_synced;
time_source_t source = status.current_source;
uint32_t sync_count = status.sync_count;
```

### Status Display Example:
```
I (1234) SMART_TIME: Time Status:
I (1235) SMART_TIME:   DS3231: ✓ Available
I (1236) SMART_TIME:   Current source: DS3231 RTC
I (1237) SMART_TIME:   Sync count: 3
I (1238) SMART_TIME: ✓ Time is valid and reliable
```

## 🔄 Automatic Behaviors

### Boot Sequence:
1. **Initialize DS3231** (if configured)
2. **Try to get time** from DS3231
3. **Sync Internal RTC** with DS3231
4. **Attempt NTP sync** (if WiFi configured)
5. **Update all sources** with NTP time

### Runtime Maintenance:
- **Every 60 seconds**: Check DS3231 health
- **Every 24 hours**: Auto-sync with NTP (if WiFi available)
- **On DS3231 failure**: Switch to Internal RTC
- **On DS3231 recovery**: Switch back to DS3231

### Failure Recovery:
- **DS3231 disconnected**: Continues with Internal RTC
- **DS3231 reconnected**: Automatically detects and switches back
- **WiFi unavailable**: Continues with existing time sources
- **All sources failed**: Uses reasonable default time

## 🛠️ Manual Operations

### Set Time Manually:
```c
// In your code
smart_time_set_manual("2024-12-24 15:30:00");
```

### Force NTP Sync:
```c
// In your code  
smart_time_sync_now();
```

### Check Time Validity:
```c
if (smart_time_is_valid()) {
    // Time is reliable
} else {
    // Time may be inaccurate
}
```

## 📈 Expected Performance

### Accuracy:
- **DS3231**: ±2ppm (±1 minute per year)
- **NTP Sync**: ±50ms (when synced)
- **Internal RTC**: ±40ppm (±21 minutes per year)

### Power Consumption:
- **DS3231**: ~100µA active, ~1µA backup
- **WiFi Sync**: ~200mA for 10 seconds (once per day)
- **Maintenance**: Negligible impact

### Reliability:
- **DS3231 failure detection**: <3 minutes
- **Auto-recovery**: <1 minute after reconnection
- **Sync frequency**: Every 24 hours (configurable)

## 🔍 Troubleshooting

### DS3231 Not Detected:
```
W (1234) SMART_TIME: DS3231 initialization failed: ESP_ERR_NOT_FOUND
```
**Solutions:**
- Check wiring (SDA, SCL, VCC, GND)
- Verify I2C address (0x68)
- Test with different GPIO pins

### WiFi Sync Failed:
```
W (1234) SMART_TIME: NTP sync failed: ESP_FAIL
```
**Solutions:**
- Check WiFi credentials
- Verify internet connectivity
- Try different NTP servers

### Time Not Valid:
```
W (1234) SMART_TIME: ⚠️  Time may not be accurate
```
**Solutions:**
- Set time manually: `smart_time_set_manual("2024-12-24 15:30:00")`
- Connect to WiFi for NTP sync
- Check DS3231 battery

## 🎯 Best Practices

### For Field Deployment:
1. **Use DS3231 + WiFi** for maximum reliability
2. **Sync before deployment** in lab with WiFi
3. **Periodic maintenance** - re-sync monthly
4. **Monitor status** - check logs for time source

### For Lab Testing:
1. **WiFi + Internal RTC** is sufficient
2. **Auto-sync** handles time maintenance
3. **Manual sync** for immediate updates

### For Offline Operation:
1. **DS3231 only** with manual time setting
2. **Set accurate time** before deployment
3. **Replace battery** annually

## 📋 Configuration Examples

### Maximum Reliability (Field):
```c
#define DS3231_SDA_PIN GPIO_NUM_21
#define DS3231_SCL_PIN GPIO_NUM_22  
#define WIFI_SSID      "LabWiFi"
#define WIFI_PASSWORD  "LabPassword"
#define TIMEZONE       "EST5EDT,M3.2.0,M11.1.0"
```

### Lab Testing:
```c
#define DS3231_SDA_PIN -1           // Disable DS3231
#define DS3231_SCL_PIN -1           // Disable DS3231
#define WIFI_SSID      "LabWiFi"
#define WIFI_PASSWORD  "LabPassword"
#define TIMEZONE       "UTC0"
```

### Offline Deployment:
```c
#define DS3231_SDA_PIN GPIO_NUM_21
#define DS3231_SCL_PIN GPIO_NUM_22
#define WIFI_SSID      NULL         // Disable WiFi
#define WIFI_PASSWORD  NULL         // Disable WiFi
#define TIMEZONE       "UTC0"
```

The Smart Time System gives you the **best of all worlds** - reliability, accuracy, and flexibility!