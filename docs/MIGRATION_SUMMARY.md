# ESP-IDF Migration - Decision Summary

**Date:** December 9, 2025  
**Decision:** Migrate from Arduino framework to ESP-IDF framework

## Why We're Migrating

### The Problem
- VL53L8CX sensor library (STM32duino) is incompatible with ESP32
- Arduino framework caused I2C initialization hangs
- No mature Arduino libraries exist for VL53L8CX on ESP32

### The Solution
- Use RJRP44's VL53L8CX-Library (ESP-IDF native) - **tested and working**
- Switch entire project to ESP-IDF framework
- Gain better ML performance with ESP-NN

## Benefits of ESP-IDF

1. **Working Sensors** ✅
   - VL53L8CX: RJRP44 library (confirmed working)
   - BNO08x: ESP-IDF components available
   - Better hardware control

2. **Better ML Performance** ✅
   - TensorFlow Lite Micro: Official ESP-IDF component
   - ESP-NN: Native optimized neural network kernels
   - Faster inference on ESP32-S3

3. **Professional Framework** ✅
   - Official Espressif framework
   - Better debugging and profiling
   - Production-ready
   - More suitable for Master's thesis

4. **FreeRTOS Integration** ✅
   - Proper multi-tasking for sensors
   - Better resource management
   - More control over timing

## What Changes

### Project Structure
- Arduino `lib/` → ESP-IDF `components/`
- Arduino APIs → ESP-IDF APIs
- PlatformIO still works (supports ESP-IDF)

### Code Changes
- GPIO: `digitalWrite()` → `gpio_set_level()`
- I2C: `Wire.begin()` → `i2c_driver_install()`
- Timing: `delay()` → `vTaskDelay()`
- Serial: `Serial.println()` → `ESP_LOGI()` or `printf()`

### What Stays the Same
- Overall architecture (SensorManager, DataModels, etc.)
- Data flow and algorithms
- ML inference logic
- Hardware wiring

## Migration Timeline

**Estimated Time:** 2-3 days

1. **Day 1:** Project setup + VL53L8CX sensor working
2. **Day 2:** Port remaining sensors + core systems
3. **Day 3:** TensorFlow Lite integration + testing

## Files Updated

- ✅ `platformio.ini` - Changed framework to espidf
- ✅ `README.md` - Updated framework information
- ✅ `docs/esp-idf-migration.md` - Detailed migration guide
- ⏳ Source code - To be ported

## Next Steps

1. Create ESP-IDF component structure
2. Port VL53L8CX sensor (priority #1)
3. Test basic sensor reading
4. Port remaining components
5. Integrate TensorFlow Lite with ESP-NN

## GitHub Repo Name

Current: `ProxiTof_Arduino_ESP32`  
Future: Can be renamed later (e.g., `ProxiTof_ESP32` or `ProxiTof_ESPIDF`)  
**Action:** Rename after migration is complete

---

**Status:** Migration started ✅  
**Next:** Create ESP-IDF component structure and port VL53L8CX
