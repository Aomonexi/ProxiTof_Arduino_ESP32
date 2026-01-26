# Time Synchronization Solution - DS3231 + SNTP

**Date:** January 26, 2026  
**Hardware:** DS3231 RTC Module (H2-084) with 32.768kHz crystal  
**Issue:** Timezone configuration incorrect, need one-time WiFi sync

---

## Problem Analysis

### Current Issues:

1. **Timezone String Wrong**
   - Current: `TIMEZONE=CST-8`
   - This means: 8 hours WEST of UTC (US Central Standard Time)
   - You probably want: UTC+8 (China/Singapore/Philippines)
   - Correct format: `CST-8` actually means UTC-8, not UTC+8!

2. **WiFi Always On**
   - WiFi stays connected after sync
   - Wastes power
   - Not needed after time is set

3. **DS3231 32kHz Pin Not Utilized**
   - DS3231 has 32kHz output pin
   - Can be used as external clock source for ESP32
   - Currently not configured

---

## Solution Architecture

```
┌─────────────────────────────────────────────────────────┐
│                    Time Sync Flow                        │
└─────────────────────────────────────────────────────────┘

1. Power On
   ↓
2. Read DS3231 RTC (battery-backed, survives power loss)
   ↓
3. If DS3231 time valid → Use it
   ↓
4. If DS3231 time invalid OR user requests sync:
   ↓
5. Connect WiFi → Sync SNTP → Update DS3231 → Disconnect WiFi
   ↓
6. Use DS3231 for all timekeeping (no WiFi needed)
   ↓
7. Optional: Auto-sync every 24 hours (configurable)
```

---

## Timezone Fix

### Understanding POSIX Timezone Format

**Format:** `STD offset [DST [offset],start[/time],end[/time]]`

**Examples:**

| Location | Correct String | Wrong String | Explanation |
|----------|---------------|--------------|-------------|
| **China (UTC+8)** | `CST-8` or `<+08>-8` | `CST+8` | Negative offset for EAST of UTC |
| **Singapore (UTC+8)** | `<+08>-8` | `SGT+8` | Same as China |
| **Philippines (UTC+8)** | `PHT-8` | `PHT+8` | Philippine Time |
| **Indonesia (UTC+7)** | `WIB-7` | `WIB+7` | Western Indonesian Time |
| **US Eastern** | `EST5EDT,M3.2.0,M11.1.0` | `EST-5` | Has DST |
| **US Pacific** | `PST8PDT,M3.2.0,M11.1.0` | `PST-8` | Has DST |
| **UTC** | `UTC0` | `UTC+0` | Zero offset |

**Key Rule:** 
- **EAST of UTC** → Use **NEGATIVE** offset (e.g., `-8` for UTC+8)
- **WEST of UTC** → Use **POSITIVE** offset (e.g., `+5` for UTC-5)

### Recommended Timezone Strings

```ini
# Asia (UTC+8)
TIMEZONE=<+08>-8

# Or with name:
TIMEZONE=CST-8  # China Standard Time (but confusing with US CST!)

# Better - use generic format:
TIMEZONE=<UTC+8>-8

# Indonesia (UTC+7)
TIMEZONE=WIB-7

# Japan (UTC+9)
TIMEZONE=JST-9

# India (UTC+5:30)
TIMEZONE=IST-5:30

# Australia Eastern (UTC+10, has DST)
TIMEZONE=AEST-10AEDT,M10.1.0,M4.1.0/3
```

---

## DS3231 32kHz Crystal Usage

### What is the 32kHz Pin?

The DS3231 has a **32.768kHz square wave output** pin that can be used as:
1. **External clock source** for ESP32 (not recommended - ESP32 has good internal oscillator)
2. **Interrupt source** for periodic wakeup (useful for deep sleep)
3. **Clock signal** for other peripherals

### Current Setup (Recommended)

**Your DS3231 is already working correctly!**

- ✅ 32.768kHz crystal is **internal** to the DS3231 module
- ✅ Provides accurate timekeeping (±2ppm = ±1 minute/year)
- ✅ Battery-backed (CR2032) - keeps time when powered off
- ✅ Temperature-compensated (TCXO) - accurate across temperature range

**You don't need to connect the 32kHz output pin to ESP32!**

The ESP32 has its own internal oscillator that's good enough for system time. The DS3231's job is to:
1. Keep accurate time when powered off (battery backup)
2. Provide accurate time reference when powered on
3. Sync with NTP occasionally to correct drift

---

## Implementation Plan

### Phase 1: Fix Timezone (Immediate)

**Update `data/config.txt`:**

```ini
# Time Configuration
TIMEZONE=<+08>-8  # UTC+8 (China/Singapore/Philippines)
SYNC_INTERVAL_HOURS=24
```

**Or if you're in a different timezone:**

```ini
# Indonesia (UTC+7)
TIMEZONE=WIB-7

# Japan (UTC+9)
TIMEZONE=JST-9

# India (UTC+5:30)
TIMEZONE=IST-5:30
```

### Phase 2: Improve Time Sync Logic (Code Changes)

**Current Flow:**
```
1. Init → Try DS3231
2. If DS3231 fails → Try NTP
3. WiFi stays connected
```

**Improved Flow:**
```
1. Init → Read DS3231
2. If DS3231 time valid (year > 2024) → Use it, skip WiFi
3. If DS3231 time invalid → Connect WiFi → NTP sync → Update DS3231 → Disconnect WiFi
4. Periodic check: If 24 hours passed → Optional auto-sync
```

### Phase 3: Add Manual Sync Command (USB Interface)

**New Commands:**
```
time_sync          # Force NTP sync now
time_status        # Show current time and source
time_set YYYY-MM-DD HH:MM:SS  # Set time manually
```

---

## Code Changes

### 1. Update `smart_time.c` - Improved Init Logic

**Change:** Only sync NTP if DS3231 time is invalid

```c
esp_err_t smart_time_init(...) {
    // ... existing DS3231 init ...
    
    // Try to get time from DS3231
    struct tm timeinfo;
    esp_err_t ret = smart_time_get(&timeinfo);
    
    if (ret == ESP_OK) {
        time_t t = mktime(&timeinfo);
        // Check if time is reasonable (after 2024)
        if (t > 1704067200) {  // Jan 1, 2024
            ESP_LOGI(TAG, "✓ DS3231 time valid, skipping NTP sync");
            ESP_LOGI(TAG, "Current time: %04d-%02d-%02d %02d:%02d:%02d",
                     timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
                     timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
            return ESP_OK;  // Don't sync NTP if DS3231 is good
        }
    }
    
    // DS3231 time invalid - try NTP sync
    if (wifi_enabled) {
        ESP_LOGI(TAG, "DS3231 time invalid, attempting NTP sync...");
        sync_from_ntp();
    } else {
        ESP_LOGW(TAG, "No valid time source available");
    }
    
    return ESP_OK;
}
```

### 2. Update `time_sync.c` - Better Timezone Handling

**Add timezone validation:**

```c
static const char* validate_timezone(const char* tz) {
    if (!tz || strlen(tz) == 0) {
        return "UTC0";
    }
    
    // Common mistakes - auto-correct
    if (strcmp(tz, "CST+8") == 0) {
        ESP_LOGW(TAG, "Timezone 'CST+8' is wrong for UTC+8, using 'CST-8'");
        return "CST-8";
    }
    if (strcmp(tz, "UTC+8") == 0) {
        ESP_LOGW(TAG, "Timezone 'UTC+8' format wrong, using '<+08>-8'");
        return "<+08>-8";
    }
    
    return tz;
}

esp_err_t time_sync_init(const char* wifi_ssid, const char* wifi_password, const char* timezone) {
    // ... existing code ...
    
    // Validate and set timezone
    const char* validated_tz = validate_timezone(timezone);
    ESP_LOGI(TAG, "Setting timezone: %s", validated_tz);
    setenv("TZ", validated_tz, 1);
    tzset();
    
    // ... rest of code ...
}
```

### 3. Add USB Commands for Time Management

**New file: `components/usb_interface/time_commands.c`**

```c
void handle_time_sync_command(void) {
    ESP_LOGI(TAG, "Manual time sync requested...");
    esp_err_t ret = smart_time_sync_now();
    if (ret == ESP_OK) {
        printf("✓ Time synchronized successfully\n");
    } else {
        printf("❌ Time sync failed: %s\n", esp_err_to_name(ret));
    }
}

void handle_time_status_command(void) {
    time_status_t status;
    smart_time_get_status(&status);
    
    struct tm timeinfo;
    smart_time_get(&timeinfo);
    
    printf("\n╔════════════════════════════════════════╗\n");
    printf("║         Time Status                    ║\n");
    printf("╠════════════════════════════════════════╣\n");
    printf("║ Current Time: %04d-%02d-%02d %02d:%02d:%02d ║\n",
           timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
           timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    printf("║ Source: %-26s ║\n", smart_time_source_name(status.current_source));
    printf("║ DS3231: %-26s ║\n", status.ds3231_available ? "Available" : "Not available");
    printf("║ Synced: %-26s ║\n", status.time_synced ? "Yes" : "No");
    printf("║ Sync Count: %-22lu ║\n", (unsigned long)status.sync_count);
    printf("╚════════════════════════════════════════╝\n\n");
}
```

---

## Testing Procedure

### Test 1: Timezone Verification

```bash
# 1. Update config.txt with correct timezone
TIMEZONE=<+08>-8

# 2. Flash and monitor
idf.py flash monitor

# 3. Check log output - should show correct local time
# If current UTC time is 10:00, local time should be 18:00 (UTC+8)
```

### Test 2: One-Time WiFi Sync

```bash
# 1. Remove DS3231 battery (to clear time)
# 2. Power on device
# 3. Should see:
#    - "DS3231 time invalid"
#    - "Connecting to WiFi..."
#    - "Time synchronized"
#    - "WiFi disconnected"
# 4. Check time is correct
# 5. Power off and on again
# 6. Should see:
#    - "DS3231 time valid, skipping NTP sync"
#    - No WiFi connection
```

### Test 3: Manual Sync via USB

```bash
# In USB serial terminal:
time_status        # Check current time
time_sync          # Force sync
time_status        # Verify updated
```

---

## Power Consumption Analysis

### Current System (WiFi Always On):
```
ESP32-S3: ~160mA (WiFi active)
DS3231: ~200µA
Total: ~160mA
Battery life (2000mAh): ~12 hours
```

### Improved System (WiFi Off):
```
ESP32-S3: ~40mA (WiFi off, sensors active)
DS3231: ~200µA
Total: ~40mA
Battery life (2000mAh): ~50 hours
```

### Deep Sleep Mode (Future):
```
ESP32-S3: ~10µA (deep sleep)
DS3231: ~200µA
Total: ~210µA
Battery life (2000mAh): ~9500 hours (~400 days!)
```

---

## Recommended Configuration

### For Most Users (UTC+8):

**`data/config.txt`:**
```ini
# WiFi Configuration (only for initial sync)
WIFI_ENABLED=true
WIFI_SSID=YourWiFiNetwork
WIFI_PASSWORD=YourWiFiPassword

# Time Configuration
TIMEZONE=<+08>-8  # UTC+8 (China/Singapore/Philippines)
SYNC_INTERVAL_HOURS=168  # Weekly sync (7 days × 24 hours)

# DS3231 RTC Configuration
DS3231_ENABLED=true
DS3231_SDA_PIN=8
DS3231_SCL_PIN=9
```

### For Field Use (No WiFi):

```ini
# WiFi Configuration
WIFI_ENABLED=false  # Disable WiFi completely

# Time Configuration
TIMEZONE=<+08>-8
SYNC_INTERVAL_HOURS=0  # Disable auto-sync

# DS3231 RTC Configuration
DS3231_ENABLED=true
DS3231_SDA_PIN=8
DS3231_SCL_PIN=9
```

**Sync time before field deployment:**
1. Enable WiFi temporarily
2. Power on device
3. Wait for NTP sync
4. Disable WiFi in config
5. Reflash or update config via SD card

---

## Summary

### What You Have:
- ✅ DS3231 RTC with 32kHz crystal (internal, working correctly)
- ✅ Battery backup (CR2032)
- ✅ Temperature compensation (±2ppm accuracy)
- ✅ I2C interface (shared with ToF sensor)

### What You Need to Fix:
1. **Timezone string** - Change `CST-8` to `<+08>-8` (or your actual timezone)
2. **WiFi disconnect** - Add code to disconnect after sync
3. **Skip unnecessary syncs** - Only sync if DS3231 time is invalid

### What You Get:
- ✅ Accurate time (±1 minute/year)
- ✅ Survives power loss (battery backup)
- ✅ One-time WiFi sync (then disconnect)
- ✅ Low power consumption (~40mA vs ~160mA)
- ✅ Optional periodic sync (configurable)
- ✅ Manual sync via USB (for field updates)

---

**Next Steps:**
1. Update timezone in config.txt
2. Implement improved init logic
3. Add USB time commands
4. Test with battery backup
5. Measure power consumption

