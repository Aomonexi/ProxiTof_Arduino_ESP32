# Performance Optimization and SD Card Implementation Plan

## Problem Analysis

### Current Issue: System Slowdown After 5 Minutes

**Symptoms:**
- Sensors work fine initially
- After ~5 minutes, response becomes slow/laggy
- Display updates become sluggish

**Root Cause:**
Excessive UART logging causing buffer overflow and CPU saturation.

**Current Logging Rate:**
- Display update: 20Hz (every 50ms)
- Each update: ~40 lines of printf
- Data rate: ~800 lines/second = ~80KB/second to UART
- UART speed: 115200 baud = ~11.5KB/second
- **Result:** 7x more data than UART can handle!

### Why This Happens

1. **UART Buffer Overflow**
   - Printf sends data faster than UART can transmit
   - Buffer fills up
   - System blocks waiting for UART
   - Sensors can't be read in time

2. **CPU Saturation**
   - Printf is CPU-intensive (formatting, buffering)
   - At 20Hz with 40 lines each = 800 printf calls/second
   - CPU spends more time printing than reading sensors

3. **Memory Pressure**
   - UART buffers grow
   - Printf buffers accumulate
   - Eventually causes memory fragmentation

## Solution Strategy

### Phase 1: Immediate Fix (Reduce Logging) ✅

**Option A: Reduce Display Rate**
```c
vTaskDelay(pdMS_TO_TICKS(500));  // 2Hz instead of 20Hz
```

**Option B: Conditional Display**
```c
// Only display every Nth frame
if (frame_count % 10 == 0) {
    print_tof_grid(...);
}
```

**Option C: Minimal Logging Mode**
```c
// Single line updates instead of full grid
printf("\r[%lu] Min: %4u mm | IMU: R:%6.1f P:%6.1f Y:%6.1f", 
       frame_count, min_dist, roll, pitch, yaw);
```

### Phase 2: SD Card Data Logging (Next Implementation) 🎯

**Why SD Card?**
- ✅ High-speed data storage (MB/s vs KB/s for UART)
- ✅ No CPU blocking
- ✅ Large capacity for training datasets
- ✅ Offline analysis capability
- ✅ Essential for ML training data collection

**Implementation Plan:**

1. **Hardware Setup**
   - SD card module via SPI
   - Recommended: SPI2 (HSPI) for high speed
   - Pins: MOSI, MISO, SCK, CS

2. **Data Format**
   - CSV format for easy analysis
   - Timestamp, ToF grid (16 values), IMU (6-9 values), GPS (lat/lon)
   - Example: `1234567,450,520,480,...,12.3,-5.6,89.1,0.12,-0.05,9.81`

3. **Logging Strategy**
   - Buffer data in RAM (circular buffer)
   - Write to SD card in batches
   - Minimize SD card writes (wear leveling)
   - Flush on button press or periodic interval

4. **File Management**
   - One file per session
   - Filename: `YYYYMMDD_HHMMSS.csv`
   - Header row with column names
   - Automatic file rotation at size limit

### Phase 3: Optimized Display (Optional)

**For Development/Debug:**
- Minimal console output
- Web-based dashboard (ESP32 as HTTP server)
- Real-time plotting via WebSocket
- Only when needed, not continuous

## Implementation Priority

### High Priority (Do Now) ✅
1. **Reduce logging rate** - Fix immediate slowdown
2. **Add SD card support** - Essential for data collection
3. **Implement CSV logging** - Training dataset format

### Medium Priority
4. **Add GPS module** - Location tracking
5. **Implement buffering** - Efficient data handling
6. **Add file management** - Session handling

### Low Priority
7. **Web dashboard** - Nice to have for debugging
8. **Real-time plotting** - Development tool

## Next Steps

### Step 1: Quick Fix (5 minutes)
Update main.c to reduce display rate:
```c
// Change from 50ms to 500ms
vTaskDelay(pdMS_TO_TICKS(500));  // 2Hz display
```

### Step 2: SD Card Integration (2-3 hours)
1. Add SD card component to CMakeLists.txt
2. Initialize SD card in sensor_manager or separate component
3. Implement CSV writer
4. Add data buffering
5. Test write performance

### Step 3: Data Collection Mode (1 hour)
1. Add "logging mode" vs "display mode"
2. Button press to start/stop logging
3. LED indicator for logging status
4. File management (create, close, rotate)

### Step 4: Testing (1 hour)
1. Verify no slowdown with SD logging
2. Check data integrity
3. Measure write performance
4. Test long-duration logging (30+ minutes)

## Expected Results

### After Quick Fix:
- ✅ No more slowdown
- ✅ System stable for hours
- ⚠️ Less frequent display updates

### After SD Card Implementation:
- ✅ High-speed data logging (60Hz ToF + 100Hz IMU)
- ✅ No performance impact
- ✅ Training dataset collection ready
- ✅ Offline analysis capability

## Hardware Requirements

### SD Card Module
- **Interface:** SPI
- **Voltage:** 3.3V
- **Speed:** Class 10 or higher recommended
- **Capacity:** 8GB+ (plenty for millions of samples)

### Wiring (Example)
```
SD Card    ESP32-S3
--------   ---------
MOSI   ->  GPIO 35
MISO   ->  GPIO 37
SCK    ->  GPIO 36
CS     ->  GPIO 34
VCC    ->  3.3V
GND    ->  GND
```

Note: Use different SPI bus than IMU (SPI2/HSPI vs SPI3/VSPI)

## Code Structure

### New Component: sd_logger
```
components/sd_logger/
├── sd_logger.c
├── include/
│   └── sd_logger.h
├── CMakeLists.txt
└── idf_component.yml
```

### API Design
```c
// Initialize SD card
esp_err_t sd_logger_init(void);

// Start logging session
esp_err_t sd_logger_start_session(void);

// Log sensor data
esp_err_t sd_logger_write_sample(tof_data_t *tof, imu_data_t *imu);

// Stop logging session
esp_err_t sd_logger_stop_session(void);

// Get logging status
bool sd_logger_is_logging(void);
```

## Performance Targets

| Metric | Target | Notes |
|--------|--------|-------|
| ToF Sample Rate | 60Hz | No degradation |
| IMU Sample Rate | 100Hz | No degradation |
| SD Write Rate | 100+ samples/sec | Batch writes |
| CPU Usage | < 50% | Leave room for ML |
| Logging Duration | Hours | Limited by SD card size |
| File Size | ~1MB/minute | Depends on sample rate |

## Conclusion

**Immediate Action:** Reduce display rate to fix slowdown  
**Next Implementation:** SD card logging for data collection  
**Timeline:** Quick fix now, SD card within 1 day  
**Impact:** Essential for ML training dataset collection
