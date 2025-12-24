# WiFi Time Synchronization Guide

## Why Time Sync is Important

For accurate data logging, the ESP32 needs to know the current date and time. Without this:
- Log files are created with wrong dates (1970-01-01)
- Timestamps in CSV files are incorrect
- Data analysis becomes difficult

## How It Works

1. **ESP32 connects to WiFi** at startup
2. **Downloads current time** from NTP servers (pool.ntp.org)
3. **Sets internal RTC** with accurate time
4. **Disconnects WiFi** to save power
5. **Internal RTC maintains time** even without WiFi

## Setup Instructions

### Step 1: Edit WiFi Credentials

Open `main/main.c` and update these lines:

```c
// WiFi Configuration - Edit these for your network
#define WIFI_SSID     "YourWiFiNetwork"      // <- Your WiFi name
#define WIFI_PASSWORD "YourWiFiPassword"     // <- Your WiFi password
#define TIMEZONE      "UTC0"                 // <- Your timezone
```

### Step 2: Set Your Timezone

Common timezone strings:

| Location | Timezone String |
|----------|----------------|
| **UTC** | `UTC0` |
| **US Eastern** | `EST5EDT,M3.2.0,M11.1.0` |
| **US Pacific** | `PST8PDT,M3.2.0,M11.1.0` |
| **Europe/London** | `GMT0BST,M3.5.0/1,M10.5.0` |
| **Europe/Berlin** | `CET-1CEST,M3.5.0,M10.5.0/3` |
| **Asia/Tokyo** | `JST-9` |
| **Asia/Shanghai** | `CST-8` |
| **Australia/Sydney** | `AEST-10AEDT,M10.1.0,M4.1.0/3` |

### Step 3: Build and Flash

```bash
idf.py build flash monitor
```

## Expected Output

### Success Case:
```
I (1234) TIME_SYNC: Connecting to WiFi SSID: YourNetwork
I (2345) TIME_SYNC: ✓ Connected to WiFi
I (3456) TIME_SYNC: Waiting for NTP time sync...
I (4567) TIME_SYNC: ✓ Time synchronized: 2024-12-24 15:30:45
I (4568) MAIN: ✓ Time synchronized successfully
I (4569) MAIN: Current time: 2024-12-24 15:30:45
I (4570) TIME_SYNC: ✓ WiFi disconnected
I (5678) STORAGE_MGR: Using synchronized time: 2024-12-24
I (5679) STORAGE_MGR: Created directory: /sdcard/2024-12-24
```

### Failure Case:
```
E (1234) TIME_SYNC: ❌ Failed to connect to WiFi
W (1235) MAIN: ❌ Time sync failed: ESP_FAIL
W (1236) MAIN: Continuing with default time (logs will have incorrect timestamps)
W (1237) STORAGE_MGR: System time not synchronized, using default date
```

## Troubleshooting

### WiFi Connection Failed
- ✅ Check SSID spelling (case-sensitive)
- ✅ Check password (case-sensitive)
- ✅ Ensure 2.4GHz WiFi (ESP32 doesn't support 5GHz)
- ✅ Check WiFi signal strength

### Time Sync Failed
- ✅ Check internet connectivity
- ✅ Try different NTP servers
- ✅ Check firewall settings (port 123 UDP)

### Wrong Timezone
- ✅ Verify timezone string format
- ✅ Test with UTC0 first
- ✅ Check daylight saving rules

## Security Notes

- WiFi credentials are stored in flash memory
- Consider using WPA2-Enterprise for corporate networks
- WiFi is disconnected after time sync for security/power

## Alternative: Manual Time Setting

For testing without WiFi, you can set time manually in code:

```c
// Set time manually (for testing only)
struct tm timeinfo = {
    .tm_year = 2024 - 1900,  // Year since 1900
    .tm_mon = 12 - 1,        // Month (0-11)
    .tm_mday = 24,           // Day
    .tm_hour = 15,           // Hour
    .tm_min = 30,            // Minute
    .tm_sec = 0              // Second
};
time_t t = mktime(&timeinfo);
struct timeval tv = { .tv_sec = t };
settimeofday(&tv, NULL);
```

## Power Consumption

- **WiFi connection**: ~200mA for 5-10 seconds
- **After disconnect**: Normal ESP32 consumption
- **Time keeping**: Minimal power (internal RTC)

## File Organization

With correct time sync, files are organized as:
```
/sdcard/
├── 2024-12-24/
│   ├── log_20241224_153045.csv
│   └── log_20241224_160122.csv
├── 2024-12-25/
│   └── log_20241225_090315.csv
```

This makes data analysis much easier!