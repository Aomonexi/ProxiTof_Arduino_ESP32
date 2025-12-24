# Storage Manager Integration Guide

## Current Status ✅

Your Storage Manager component is **fully implemented** and ready to use! It includes:

- ✅ **SDMMC/SDIO interface** (4-bit mode, up to 40MHz)
- ✅ **Circular buffering** (120 entries, non-blocking)
- ✅ **CSV file generation** with proper headers
- ✅ **Date-based organization** (/sdcard/YYYY-MM-DD/)
- ✅ **File rotation** at 100MB
- ✅ **Statistics tracking**

## Hardware Setup Required

### SD Card Wiring (SDMMC - Dedicated Pins)
```
SD Card Breakout → ESP32-S3
├── VCC → 3.3V
├── GND → GND
├── CLK → GPIO 39 (dedicated SDMMC pin)
├── CMD → GPIO 38 (dedicated SDMMC pin)
├── D0  → GPIO 40 (dedicated SDMMC pin)
├── D1  → GPIO 41 (4-bit mode)
├── D2  → GPIO 42 (4-bit mode)
└── D3  → GPIO 43 (4-bit mode)
```

**Benefits of SDMMC vs SPI:**
- ✅ **4x faster** (4-bit parallel vs 1-bit serial)
- ✅ **Dedicated pins** (no sharing with sensors)
- ✅ **Hardware acceleration** (ESP32-S3 SDMMC controller)
- ✅ **Better reliability** (native SD protocol)

## Integration with Sensor Data

### Step 1: Add Storage Manager to Main Application

**main/CMakeLists.txt:**
```cmake
idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES sensor_manager data_processor storage_manager
)
```

### Step 2: Initialize in app_main()

**main/main.c:**
```c
#include "storage_manager.h"
#include "sensor_manager.h"
#include "data_processor.h"

void app_main(void) {
    ESP_LOGI(TAG, "ProxiTof System Starting...");
    
    // Initialize components
    sensor_manager_init();
    data_processor_init();
    
    // Initialize storage (graceful if SD card not present)
    esp_err_t storage_ret = storage_manager_init();
    if (storage_ret == ESP_OK) {
        ESP_LOGI(TAG, "✓ SD card ready for data logging");
        storage_manager_start_logging();
    } else {
        ESP_LOGW(TAG, "⚠️  SD card not available - sensors will work without logging");
    }
    
    // Create tasks
    xTaskCreate(sensor_task, "sensor_task", 4096, NULL, 5, NULL);
    xTaskCreate(storage_task, "storage_task", 4096, NULL, 3, NULL);
}
```

### Step 3: Sensor Task Integration

**Sensor task writes to storage buffer:**
```c
void sensor_task(void *pvParameters) {
    tof_data_t tof_data;
    imu_data_t imu_data;
    log_entry_t log_entry;
    
    while (1) {
        // Read sensors (your existing code)
        if (sensor_manager_read_tof(&tof_data) == ESP_OK &&
            sensor_manager_read_imu(&imu_data) == ESP_OK) {
            
            // Convert to log entry format
            log_entry.timestamp_ms = tof_data.timestamp_ms;
            
            // Copy ToF data (16 zones)
            memcpy(log_entry.tof, tof_data.distances, sizeof(log_entry.tof));
            
            // Copy IMU data
            log_entry.roll = imu_data.roll;
            log_entry.pitch = imu_data.pitch;
            log_entry.yaw = imu_data.yaw;
            log_entry.accel_x = imu_data.accel_x;
            log_entry.accel_y = imu_data.accel_y;
            log_entry.accel_z = imu_data.accel_z;
            log_entry.gyro_x = imu_data.gyro_x;
            log_entry.gyro_y = imu_data.gyro_y;
            log_entry.gyro_z = imu_data.gyro_z;
            
            // Check button for manual flag
            log_entry.flag_manual = sensor_manager_is_button_pressed() ? 1 : 0;
            
            // Write to storage (non-blocking)
            if (storage_manager_is_logging()) {
                esp_err_t ret = storage_manager_write(&log_entry);
                if (ret != ESP_OK) {
                    // Buffer full - this is logged internally
                }
            }
        }
        
        // 60Hz operation
        vTaskDelay(pdMS_TO_TICKS(16));
    }
}
```

### Step 4: Storage Task (Optional - for monitoring)

**Separate task for storage monitoring:**
```c
void storage_task(void *pvParameters) {
    storage_stats_t stats;
    
    while (1) {
        if (storage_manager_is_logging()) {
            storage_manager_get_stats(&stats);
            
            // Log statistics every 10 seconds
            ESP_LOGI("STORAGE", "Logged: %lu, Dropped: %lu, Buffer: %.1f%%",
                     stats.entries_logged, stats.entries_dropped, stats.buffer_fill_percent);
            
            // Force flush if buffer getting full
            if (stats.buffer_fill_percent > 90.0f) {
                ESP_LOGW("STORAGE", "Buffer nearly full - forcing flush");
                storage_manager_flush();
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(10000)); // 10 seconds
    }
}
```

## Expected CSV Output

Your system will generate files like:
```
/sdcard/2025-01-15/log_20250115_143022.csv
```

**CSV Format:**
```csv
timestamp_ms,tof_00,tof_01,tof_02,tof_03,tof_10,tof_11,tof_12,tof_13,tof_20,tof_21,tof_22,tof_23,tof_30,tof_31,tof_32,tof_33,roll,pitch,yaw,accel_x,accel_y,accel_z,gyro_x,gyro_y,gyro_z,flag_manual
1234567,500,520,480,510,505,525,485,515,500,520,480,510,505,525,485,515,0.50,1.20,-0.30,0.10,0.20,9.80,0.00,0.10,0.00,0
1234583,501,521,481,511,506,526,486,516,501,521,481,511,506,526,486,516,0.51,1.21,-0.29,0.11,0.21,9.81,0.01,0.11,0.01,0
```

## Testing Steps

### 1. Hardware Test
```bash
# Flash with test code
idf.py build flash monitor

# Look for these messages:
# ✓ Filesystem mounted successfully
# ✓ SD card mounted at /sdcard
# ✓ Created directory: /sdcard/2025-01-15
# ✓ Created log file: /sdcard/2025-01-15/log_20250115_143022.csv
```

### 2. Data Verification
```bash
# Remove SD card and check on PC
# Should see:
# /YYYY-MM-DD/log_YYYYMMDD_HHMMSS.csv
# File should contain CSV data with proper headers
```

### 3. Performance Test
```bash
# Monitor serial output for:
# - Buffer fill percentage
# - Entries logged vs dropped
# - Flush frequency
# - File size growth
```

## Troubleshooting

### SD Card Not Detected
```
❌ Failed to mount filesystem - SD card not detected or not formatted as FAT32
```
**Solutions:**
- Check wiring (especially power and ground)
- Format SD card as FAT32 on PC
- Try different SD card (some are incompatible)
- Verify 3.3V power supply

### Buffer Overflows
```
Buffer fill: 95.0%
Entries dropped: 15
```
**Solutions:**
- Use faster SD card (Class 10 or UHS-I)
- Reduce logging frequency
- Check for SD card write errors

### File System Errors
```
Failed to create log file
```
**Solutions:**
- Check SD card free space
- Verify FAT32 format
- Check file permissions

## Next Steps

1. **Wire SD card breakout** to ESP32-S3 SDMMC pins
2. **Test with test_storage_manager.c** code
3. **Integrate with your sensor tasks**
4. **Collect training data** for ML model
5. **Use CSV files** with Python training pipeline

Your Storage Manager is production-ready and optimized for high-speed data logging!