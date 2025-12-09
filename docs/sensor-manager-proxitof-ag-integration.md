# Sensor Manager Integration with ProxiTof_AG Configuration

## Overview
Updated sensor_manager component to match the working ProxiTof_AG project configuration exactly. This ensures hardware compatibility and working sensor detection.

## Changes Made

### 1. Component Dependencies

**Created: `components/sensor_manager/idf_component.yml`**
```yaml
dependencies:
  rjrp44/vl53l8cx: "^4.0.0"
  espressif/led_strip: "^2.5.5"
  esp32_BNO08x:
    git: https://github.com/myles-parfeniuk/esp32_BNO08x.git
```

### 2. CMakeLists.txt Updates

**Updated: `components/sensor_manager/CMakeLists.txt`**
- Changed source file from `.c` to `.cpp` for C++ BNO08x support
- Added all required component dependencies

### 3. File Conversion

**Renamed:** `sensor_manager.c` → `sensor_manager.cpp`
- Required for BNO08x C++ library integration
- Added proper extern "C" linkage for C headers

### 4. VL53L8CX ToF Sensor Fixes

Applied critical fixes from working implementation:

✅ **I2C Address Conversion**
```cpp
.device_address = VL53L8CX_DEFAULT_I2C_ADDRESS >> 1  // 0x52 >> 1 = 0x29
```

✅ **Bus Config Storage**
```cpp
tof_dev.platform.bus_config = i2c_bus_config;
```

✅ **Library Reset Function**
```cpp
VL53L8CX_Reset_Sensor(&(tof_dev.platform));
```

✅ **Proper Data Indexing**
```cpp
data->distances[i] = results.distance_mm[VL53L8CX_NB_TARGET_PER_ZONE * i];
```

### 5. BNO08x IMU Implementation

**Replaced placeholder with full implementation:**

- Created BNO08x object with proper SPI configuration
- Initialized with 3MHz SPI clock
- Enabled rotation vector @ 100Hz
- Enabled linear accelerometer @ 100Hz
- Proper error handling and health tracking

**Removed:**
- Manual SPI bus initialization (BNO08x handles internally)

**Added:**
- Full IMU data reading in `sensor_manager_read_imu()`
- Euler angles (roll, pitch, yaw)
- Linear acceleration (x, y, z)

### 6. Main Application Restored

**Reverted:** `main/main.c` from backup
**Updated:** `main/CMakeLists.txt` to use sensor_manager
**Updated:** `main/idf_component.yml` - removed dependencies (handled by sensor_manager)

## Hardware Configuration

### VL53L8CX ToF Sensor (I2C)
- SDA: GPIO 8
- SCL: GPIO 9
- LPN: GPIO 4
- Address: 0x29 (7-bit)
- Frequency: 60Hz
- Resolution: 4x4

### BNO08x IMU (SPI)
- MOSI: GPIO 11
- MISO: GPIO 13
- SCK: GPIO 12
- CS: GPIO 10
- INT: GPIO 5
- RST: GPIO 6
- Clock: 3MHz
- Reports: Rotation Vector + Linear Accel @ 100Hz

### Button
- GPIO 3 (active low with pull-up)

## Build Instructions

```bash
# Clean build recommended
idf.py fullclean

# Build
idf.py build

# Flash and monitor
idf.py flash monitor
```

## Expected Behavior

On successful initialization, you should see:

```
I (xxx) SENSOR_MGR: I2C bus initialized (SDA=8, SCL=9)
I (xxx) SENSOR_MGR:   Internal pullups: ENABLED
I (xxx) SENSOR_MGR: Scanning I2C bus...
I (xxx) SENSOR_MGR:   Device found at: 0x29
I (xxx) SENSOR_MGR:     ^ VL53L8CX ToF Sensor!
I (xxx) SENSOR_MGR: ✓ VL53L8CX is alive!
I (xxx) SENSOR_MGR: ✓ VL53L8CX ready!
I (xxx) SENSOR_MGR: ✓ Configuration complete
I (xxx) SENSOR_MGR: ✓ VL53L8CX initialized successfully (4x4 mode, 60Hz)
I (xxx) SENSOR_MGR: === BNO08x IMU Initialization ===
I (xxx) SENSOR_MGR: ✓ BNO08x initialized successfully!
I (xxx) SENSOR_MGR: ✓ BNO08x ready!
I (xxx) SENSOR_MGR: Sensor Manager initialized (ToF: OK, IMU: OK)
```

## Key Differences from Previous Implementation

1. **Working I2C address** - Properly shifted for 7-bit addressing
2. **Bus config stored** - Platform structure has all needed info
3. **Library reset** - Uses VL53L8CX_Reset_Sensor() function
4. **BNO08x integrated** - Full C++ implementation, not placeholder
5. **No manual SPI init** - BNO08x library handles SPI setup
6. **Proper data reading** - Both ToF and IMU data correctly extracted

## Next Steps

Once sensors are confirmed working:
1. Test data logging functionality
2. Implement GNSS integration
3. Add SD card logging
4. Optimize for PSRAM usage
5. Implement ML inference pipeline

## References

- Working project: https://github.com/Aomonexi/ProxiTof_AG
- VL53L8CX fixes: `docs/vl53l8cx-integration-fixes.md`
- Build guide: `docs/build-and-test-guide.md`
