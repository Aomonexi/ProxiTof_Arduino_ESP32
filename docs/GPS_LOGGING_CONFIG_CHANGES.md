# GPS Logging Configuration - Implementation Guide

**Date**: January 29, 2026  
**Purpose**: Make GPS logging optional for ML training vs production use  
**Status**: ⚠️ Partial implementation - needs manual completion

---

## Overview

GPS data (latitude, longitude, speed, heading, satellites) is useful for mapping near-miss locations in production, but not needed for ESP-NN training. This adds a configuration option to enable/disable GPS logging.

---

## Changes Made

### 1. Configuration Files

**File: `data/config.txt`**
```
# GPS Logging Configuration
# GPS data is useful for mapping near-miss locations but not needed for ESP-NN training
# Set to false during data collection for ML training (reduces file size)
# Set to true for production use (enables location tracking)
GPS_LOGGING_ENABLED=false
```

### 2. Config Manager

**File: `components/config_manager/config_manager.h`**
```c
typedef struct {
    // ... existing fields ...
    bool gps_logging_enabled;  // Enable/disable GPS data in logs
    // ... rest of fields ...
} system_config_t;
```

**File: `components/config_manager/config_manager.c`**
- Added `config->gps_logging_enabled = false;` to defaults
- Added parsing for `GPS_LOGGING_ENABLED` key
- Added GPS logging status to config_print()
- Added GPS logging to config_save_to_sd()

### 3. Storage Manager (NEEDS MANUAL UPDATE)

**File: `components/storage_manager/storage_manager.c`**

You need to manually update these sections:

#### A. CSV Header (Line ~505)

**Current:**
```c
// GPS columns
fprintf(log_file, ",latitude,longitude,speed_ms,heading_deg,gps_satellites");
```

**Change to:**
```c
// GPS columns (conditional based on config)
const system_config_t *config = config_manager_get_config();
if (config->gps_logging_enabled) {
    fprintf(log_file, ",latitude,longitude,speed_ms,heading_deg,gps_satellites");
}
```

#### B. Emergency Flush GPS Data (Line ~138)

**Current:**
```c
// GPS data
fprintf(log_file, ",%.8f,%.8f,%.2f,%.2f,%u", 
        entry.latitude, entry.longitude, entry.speed_ms, 
        entry.heading_deg, entry.gps_satellites);
```

**Change to:**
```c
// GPS data (conditional)
const system_config_t *config = config_manager_get_config();
if (config->gps_logging_enabled) {
    fprintf(log_file, ",%.8f,%.8f,%.2f,%.2f,%u", 
            entry.latitude, entry.longitude, entry.speed_ms, 
            entry.heading_deg, entry.gps_satellites);
}
```

#### C. Regular Flush GPS Data (Line ~646)

**Current:**
```c
// GPS data
fprintf(log_file, ",%.8f,%.8f,%.2f,%.2f,%u", 
        entry.latitude, entry.longitude, entry.speed_ms, 
        entry.heading_deg, entry.gps_satellites);
```

**Change to:**
```c
// GPS data (conditional)
const system_config_t *config = config_manager_get_config();
if (config->gps_logging_enabled) {
    fprintf(log_file, ",%.8f,%.8f,%.2f,%.2f,%u", 
            entry.latitude, entry.longitude, entry.speed_ms, 
            entry.heading_deg, entry.gps_satellites);
}
```

---

## CSV Format Changes

### With GPS Logging Enabled (Production)
```csv
timestamp_ms,tof_00,tof_01,...,roll,pitch,yaw,accel_x,accel_y,accel_z,gyro_x,gyro_y,gyro_z,latitude,longitude,speed_ms,heading_deg,gps_satellites,flag_manual,flag_auto
```

### With GPS Logging Disabled (ML Training)
```csv
timestamp_ms,tof_00,tof_01,...,roll,pitch,yaw,accel_x,accel_y,accel_z,gyro_x,gyro_y,gyro_z,flag_manual,flag_auto
```

**File Size Reduction:** ~40 bytes per entry (5 GPS fields × 8 bytes average)

---

## Usage

### For ML Training (Default)
```
GPS_LOGGING_ENABLED=false
```
- Smaller log files
- Faster writes
- Only sensor data needed for training

### For Production
```
GPS_LOGGING_ENABLED=true
```
- Full location tracking
- Near-miss event mapping
- Route analysis

---

## Benefits

✅ **Smaller files** - 40 bytes less per entry  
✅ **Faster logging** - Less data to write  
✅ **Flexible** - Easy to toggle via config.txt  
✅ **No recompilation** - Change on SD card  
✅ **ML-friendly** - Only relevant data for training  

---

## Next Steps

1. **Manual update required:** Update storage_manager.c as shown above
2. **Rebuild:** `idf.py build`
3. **Flash:** `idf.py flash`
4. **Test:** Check CSV files with GPS_LOGGING_ENABLED=false and true

---

**Status**: Config infrastructure complete, storage_manager needs manual update  
**Author**: Kiro AI Assistant
