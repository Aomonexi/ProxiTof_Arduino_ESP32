# ESP-IDF Migration Guide

**Date Started:** December 9, 2025  
**Reason:** VL53L8CX sensor requires ESP-IDF framework for proper ESP32 support

## Why ESP-IDF?

1. ✅ **VL53L8CX Support** - RJRP44 library works natively with ESP-IDF
2. ✅ **TensorFlow Lite** - Official ESP-IDF component with ESP-NN optimization
3. ✅ **Better Performance** - Direct hardware access, optimized ML inference
4. ✅ **Professional Framework** - Official Espressif framework for production

## Migration Status

### Phase 1: Project Setup ✅
- [x] Update platformio.ini to ESP-IDF framework
- [x] Remove Arduino-specific libraries
- [ ] Create ESP-IDF project structure (components)
- [ ] Test basic build

### Phase 2: Sensor Drivers
- [ ] Port VL53L8CX using RJRP44 library
- [ ] Port BNO08x IMU to ESP-IDF
- [ ] Port button input to ESP-IDF GPIO
- [ ] Port WS2812 LED to ESP-IDF RMT

### Phase 3: Core Systems
- [ ] Port DataModels (should work as-is)
- [ ] Port SensorManager to ESP-IDF APIs
- [ ] Port ConfigParser to ESP-IDF VFS
- [ ] Port SD card logging to ESP-IDF VFS

### Phase 4: Machine Learning
- [ ] Integrate TensorFlow Lite Micro component
- [ ] Enable ESP-NN optimizations
- [ ] Port ML inference pipeline
- [ ] Test ML performance

### Phase 5: Testing & Validation
- [ ] Test all sensors
- [ ] Test data logging
- [ ] Test ML inference
- [ ] Performance benchmarking

## Key API Changes

### GPIO
```cpp
// Arduino
pinMode(pin, OUTPUT);
digitalWrite(pin, HIGH);

// ESP-IDF
gpio_set_direction(pin, GPIO_MODE_OUTPUT);
gpio_set_level(pin, 1);
```

### I2C
```cpp
// Arduino
Wire.begin(SDA, SCL);
Wire.setClock(400000);

// ESP-IDF
i2c_config_t conf = {
    .mode = I2C_MODE_MASTER,
    .sda_io_num = SDA,
    .scl_io_num = SCL,
    .sda_pullup_en = GPIO_PULLUP_ENABLE,
    .scl_pullup_en = GPIO_PULLUP_ENABLE,
    .master.clk_speed = 400000,
};
i2c_param_config(I2C_NUM_0, &conf);
i2c_driver_install(I2C_NUM_0, conf.mode, 0, 0, 0);
```

### Timing
```cpp
// Arduino
delay(100);
millis();

// ESP-IDF
vTaskDelay(pdMS_TO_TICKS(100));
esp_timer_get_time() / 1000;  // microseconds to milliseconds
```

### Serial Output
```cpp
// Arduino
Serial.println("Hello");

// ESP-IDF
printf("Hello\n");
// or
ESP_LOGI(TAG, "Hello");
```

## Benefits We'll Gain

1. **Faster ML Inference** - ESP-NN optimizations for ESP32-S3
2. **Better Debugging** - ESP-IDF has superior debugging tools
3. **FreeRTOS Tasks** - Proper multi-tasking for sensor management
4. **Production Ready** - Official framework for commercial products
5. **Better Documentation** - Espressif's comprehensive docs

## Next Steps

1. Create ESP-IDF component structure
2. Port VL53L8CX sensor first (we know it works!)
3. Test basic sensor reading
4. Port remaining sensors
5. Integrate TensorFlow Lite

---

**Note:** This migration will take 2-3 days but will result in a much more robust and performant system for the thesis.
