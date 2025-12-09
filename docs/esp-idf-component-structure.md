# ESP-IDF Component Structure

**Date:** December 9, 2025  
**Status:** In Progress

## Component Architecture

The ProxiTof system is now organized as ESP-IDF components:

```
components/
├── sensor_manager/          # Sensor hardware abstraction
│   ├── CMakeLists.txt
│   ├── sensor_manager.c
│   └── include/
│       └── sensor_manager.h
├── data_processor/          # (TODO) Data synchronization and features
├── storage_manager/         # (TODO) SD card and buffering
├── ml_inference/            # (TODO) TensorFlow Lite inference
└── status_led/              # (TODO) WS2812 LED control
```

## Completed Components

### ✅ sensor_manager

**Status:** Initial implementation complete, ToF sensor working

**Features:**
- I2C bus initialization for VL53L8CX ToF sensor
- SPI bus initialization for BNO08x IMU sensor
- GPIO initialization for button input
- VL53L8CX ToF sensor driver integration (RJRP44 library)
- Button debouncing logic
- Timestamp generation using `esp_timer_get_time()`
- Health monitoring and error tracking

**API:**
```c
esp_err_t sensor_manager_init(void);
esp_err_t sensor_manager_read_tof(tof_data_t *data);
esp_err_t sensor_manager_read_imu(imu_data_t *data);
bool sensor_manager_is_button_pressed(void);
uint32_t sensor_manager_get_timestamp(void);
void sensor_manager_get_health(sensor_health_t *health);
```

**TODO:**
- [ ] Implement BNO08x IMU driver (SPI communication)
- [ ] Add IMU data reading functionality
- [ ] Test IMU sensor integration
- [ ] Add sensor calibration routines

## Building the Project

### Using PlatformIO (Recommended)

```bash
# Build
pio run

# Flash
pio run -t upload

# Monitor
pio device monitor
```

### Using Native ESP-IDF

```bash
# Configure
idf.py set-target esp32s3

# Build
idf.py build

# Flash and monitor
idf.py flash monitor
```

## Component Dependencies

### sensor_manager

**ESP-IDF Components:**
- `driver` - I2C, SPI, GPIO drivers
- `esp_timer` - High-resolution timing
- `freertos` - Task management

**External Libraries:**
- `VL53L8CX-Library` (RJRP44) - ToF sensor driver

## Next Steps

1. **Complete BNO08x IMU driver** - Implement SPI communication protocol
2. **Create data_processor component** - Sensor synchronization and feature extraction
3. **Create storage_manager component** - SD card operations with circular buffering
4. **Create status_led component** - WS2812 LED control using RMT driver
5. **Create ml_inference component** - TensorFlow Lite integration

## Migration Notes

### Arduino → ESP-IDF API Changes

| Arduino | ESP-IDF |
|---------|---------|
| `Wire.begin()` | `i2c_driver_install()` |
| `SPI.begin()` | `spi_bus_initialize()` |
| `pinMode()` | `gpio_config()` |
| `digitalWrite()` | `gpio_set_level()` |
| `digitalRead()` | `gpio_get_level()` |
| `millis()` | `esp_timer_get_time() / 1000` |
| `delay()` | `vTaskDelay(pdMS_TO_TICKS())` |
| `Serial.println()` | `ESP_LOGI()` / `printf()` |

### Benefits Gained

1. **Native VL53L8CX support** - RJRP44 library works perfectly with ESP-IDF
2. **Better performance** - Direct hardware access, no Arduino overhead
3. **FreeRTOS tasks** - Proper multi-tasking for concurrent sensor operations
4. **ESP-NN ready** - TensorFlow Lite with ESP-NN optimization support
5. **Production-grade** - Official Espressif framework for commercial products

## Testing

### Current Test Status

- [x] I2C bus initialization
- [x] VL53L8CX ToF sensor detection
- [x] VL53L8CX 4x4 mode configuration
- [x] VL53L8CX 60Hz ranging
- [x] Button GPIO configuration
- [x] Button debouncing logic
- [ ] SPI bus initialization (done, but IMU not tested)
- [ ] BNO08x IMU communication
- [ ] IMU data reading

### Test Commands

```bash
# Build and flash
pio run -t upload

# Monitor output
pio device monitor

# Expected output:
# - "VL53L8CX initialized successfully (4x4 mode, 60Hz)"
# - ToF distance readings at ~60Hz
# - Button press detection
```

## Known Issues

1. **BNO08x IMU not implemented** - SPI communication protocol needs to be implemented
2. **No IMU data reading** - Returns `ESP_ERR_NOT_SUPPORTED` until driver is complete

## References

- [ESP-IDF Programming Guide](https://docs.espressif.com/projects/esp-idf/en/latest/)
- [VL53L8CX Library (RJRP44)](https://github.com/RJRP44/VL53L8CX-Library)
- [ESP32-S3 Technical Reference](https://www.espressif.com/sites/default/files/documentation/esp32-s3_technical_reference_manual_en.pdf)
