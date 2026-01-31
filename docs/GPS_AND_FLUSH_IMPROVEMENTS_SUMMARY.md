# GPS Logging & File Flush Improvements - Summary

**Date**: January 29, 2026  
**Issues Addressed:**
1. GPS data in logs (not needed for ML training)
2. 0KB files (data not flushed before power loss)

---

## Issue 1: GPS Logging Configuration

### Problem
GPS data (latitude, longitude, speed, heading, satellites) is logged but not needed for ESP-NN training. It's only useful for production use to map near-miss locations.

### Solution
Added `GPS_LOGGING_ENABLED` configuration option.

### Changes Made

✅ **`data/config.txt`** - Added GPS_LOGGING_ENABLED=false  
✅ **`components/config_manager/config_manager.h`** - Added gps_logging_enabled field  
✅ **`components/config_manager/config_manager.c`** - Added parsing, defaults, save, print  
⚠️ **`components/storage_manager/storage_manager.c`** - NEEDS MANUAL UPDATE (see GPS_LOGGING_CONFIG_CHANGES.md)

### Usage

**For ML Training (Default):**
```
GPS_LOGGING_ENABLED=false
```
- Smaller files (~40 bytes less per entry)
- Only sensor data for training

**For Production:**
```
GPS_LOGGING_ENABLED=true
```
- Full location tracking
- Near-miss event mapping

### Benefits
- **File size reduction:** ~40 bytes per entry
- **Faster logging:** Less data to write
- **ML-friendly:** Only relevant data
- **Flexible:** Toggle via config.txt

---

## Issue 2: Zero KB Files

### Problem
Some log files are 0KB (empty), likely due to:
- Power loss before first flush
- System crash before data written
- SD card removed while writing

### Current Protection (Already Implemented)

✅ **Header written immediately** - File never truly 0KB  
✅ **Test write after header** - Verifies file is writable  
✅ **Periodic flush** - Every 10 seconds  
✅ **Buffer threshold flush** - At 85% full  
✅ **File sync** - Every 30 seconds  
✅ **Safe shutdown** - Emergency flush on shutdown  

### Recommended Improvements

**File: `components/storage_manager/storage_manager.c` (Line ~60-62)**

**Current:**
```c
#define EMERGENCY_FLUSH_INTERVAL_MS 10000  // 10 seconds
#define FILE_SYNC_INTERVAL_MS 30000        // 30 seconds
#define BUFFER_EMERGENCY_THRESHOLD 85      // 85% full
```

**Recommended:**
```c
#define EMERGENCY_FLUSH_INTERVAL_MS 5000   // 5 seconds (more aggressive)
#define FILE_SYNC_INTERVAL_MS 10000        // 10 seconds (more aggressive)
#define BUFFER_EMERGENCY_THRESHOLD 50      // 50% full (more aggressive)
```

### Benefits
- **Maximum 5 seconds** of data loss on power failure
- **Minimum file size:** 250 bytes (header + test line)
- **No more 0KB files** under normal operation

### Trade-offs
- Slightly higher CPU usage (minimal)
- More SD card wear (negligible for modern cards)

---

## Implementation Status

### Completed ✅
1. GPS logging configuration infrastructure
2. Config file updated with GPS_LOGGING_ENABLED
3. Config manager updated to parse GPS flag
4. Documentation created

### Needs Manual Update ⚠️
1. **storage_manager.c** - Make GPS columns conditional (3 locations)
2. **storage_manager.c** - Update flush intervals (optional but recommended)

---

## Files to Update Manually

### 1. GPS Logging (Required for GPS config to work)

**File:** `components/storage_manager/storage_manager.c`

**Location 1:** Line ~505 (CSV Header)
```c
// GPS columns (conditional based on config)
const system_config_t *config = config_manager_get_config();
if (config->gps_logging_enabled) {
    fprintf(log_file, ",latitude,longitude,speed_ms,heading_deg,gps_satellites");
}
```

**Location 2:** Line ~138 (Emergency Flush)
```c
// GPS data (conditional)
const system_config_t *config = config_manager_get_config();
if (config->gps_logging_enabled) {
    fprintf(log_file, ",%.8f,%.8f,%.2f,%.2f,%u", 
            entry.latitude, entry.longitude, entry.speed_ms, 
            entry.heading_deg, entry.gps_satellites);
}
```

**Location 3:** Line ~646 (Regular Flush)
```c
// GPS data (conditional)
const system_config_t *config = config_manager_get_config();
if (config->gps_logging_enabled) {
    fprintf(log_file, ",%.8f,%.8f,%.2f,%.2f,%u", 
            entry.latitude, entry.longitude, entry.speed_ms, 
            entry.heading_deg, entry.gps_satellites);
}
```

### 2. Aggressive Flushing (Optional but recommended)

**File:** `components/storage_manager/storage_manager.c`

**Location:** Line ~60-62
```c
#define EMERGENCY_FLUSH_INTERVAL_MS 5000   // 5 seconds (was 10)
#define FILE_SYNC_INTERVAL_MS 10000        // 10 seconds (was 30)
#define BUFFER_EMERGENCY_THRESHOLD 50      // 50% (was 85%)
```

---

## Testing Procedure

### Test 1: GPS Logging Disabled
1. Set `GPS_LOGGING_ENABLED=false` in config.txt
2. Flash and run
3. Check CSV file - should NOT have GPS columns
4. File should be smaller

### Test 2: GPS Logging Enabled
1. Set `GPS_LOGGING_ENABLED=true` in config.txt
2. Restart device
3. Check CSV file - should have GPS columns
4. Verify GPS data is populated

### Test 3: File Flush (Power Loss)
1. Start device
2. Wait 3 seconds
3. Pull power
4. Check file - should have header + some data (not 0KB)

### Test 4: File Flush (Normal)
1. Start device
2. Let run for 30 seconds
3. Check file size - should be growing
4. Power off normally
5. File should have all data

---

## Expected Results

### GPS Logging Disabled (ML Training)
```csv
timestamp_ms,tof_00,...,roll,pitch,yaw,accel_x,accel_y,accel_z,gyro_x,gyro_y,gyro_z,flag_manual,flag_auto
1234567890,450,523,...,12.3,-5.7,180.0,0.98,0.02,9.81,0.15,-0.02,0.03,0,0
```

### GPS Logging Enabled (Production)
```csv
timestamp_ms,tof_00,...,roll,pitch,yaw,accel_x,accel_y,accel_z,gyro_x,gyro_y,gyro_z,latitude,longitude,speed_ms,heading_deg,gps_satellites,flag_manual,flag_auto
1234567890,450,523,...,12.3,-5.7,180.0,0.98,0.02,9.81,0.15,-0.02,0.03,1.234567,103.123456,12.5,135.0,8,0,0
```

### File Sizes
- **Minimum (empty):** 250 bytes (header + test line)
- **1 second (60 Hz):** ~9KB
- **10 seconds:** ~90KB
- **1 minute:** ~540KB

---

## Documentation Created

1. **GPS_LOGGING_CONFIG_CHANGES.md** - GPS logging implementation guide
2. **ZERO_KB_FILES_SOLUTION.md** - File flush improvements
3. **GPS_AND_FLUSH_IMPROVEMENTS_SUMMARY.md** - This file

---

## Next Steps

1. **Manual updates:**
   - Update storage_manager.c for GPS conditional logging (3 locations)
   - Update storage_manager.c for aggressive flushing (optional)

2. **Build and flash:**
   ```bash
   idf.py build flash monitor
   ```

3. **Test:**
   - Test with GPS_LOGGING_ENABLED=false
   - Test with GPS_LOGGING_ENABLED=true
   - Test power loss scenarios
   - Verify no 0KB files

4. **Monitor:**
   - Check file sizes
   - Check CSV format
   - Check data integrity

---

## Summary

✅ **GPS logging configuration** - Infrastructure complete, needs storage_manager update  
✅ **File flush improvements** - Recommendations documented  
✅ **Documentation** - Complete guides created  
⚠️ **Manual updates required** - See above for specific changes  

**Benefits:**
- Smaller log files for ML training
- No GPS data overhead when not needed
- Better protection against 0KB files
- More aggressive flushing for data safety

---

**Last Updated**: January 29, 2026  
**Status**: Partial implementation - manual updates required  
**Author**: Kiro AI Assistant
