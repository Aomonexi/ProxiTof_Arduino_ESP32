# Development Journey: ProxiTof Near-Miss Detection System
## Configuration Parser Implementation & ESP32-S3 Setup

**Date:** December 8, 2024  
**Task:** Implementation of Task 2.2 - Configuration File Parser  
**Hardware:** ESP32-S3-DevKitC-1-N16R8 (16MB Flash, 8MB PSRAM)

---

## Overview

This document chronicles the development process, challenges encountered, and solutions implemented during the initial setup and configuration parser implementation for the ProxiTof Near-Miss Detection System.

---

## 1. Initial Setup & Configuration Parser Implementation

### 1.1 Task Objective
Implement a configuration file parser that reads system parameters from an SD card-stored `config.txt` file with key=value format, including:
- Parameter validation with defined ranges
- Default value fallback mechanism
- Support for log_interval_ms, flag_window_ms, ml_threshold, approach_threshold_mm_s, and jerk_threshold_m_s3

### 1.2 Implementation Approach
Created a modular `SystemConfig` class with the following methods:
- `loadFromSD()`: Reads configuration from SD card
- `parseFromString()`: Parses key=value pairs
- `setParameter()`: Sets individual parameters with validation
- `validateParameters()`: Validates all parameters against defined ranges
- `printConfig()`: Outputs current configuration to serial

**Files Modified:**
- `include/DataModels.h`: Added SystemConfig struct and method declarations
- `src/SystemConfig.cpp`: Implemented configuration parsing logic
- `src/main.cpp`: Added test code for configuration parser

---

## 2. Hardware Configuration Challenges

### 2.1 Flash Configuration Issue - Boot Loop

**Problem:** After initial firmware upload, the ESP32-S3 entered an infinite boot loop with the error:
```
E (92) cpu_start: Octal Flash option selected, but EFUSE not configured!
abort() was called at PC 0x403771d1 on core 0
```

**Root Cause:** The `platformio.ini` was configured with `board_build.arduino.memory_type = opi_opi` (Octal flash + Octal PSRAM), but the ESP32-S3-N16R8 board has:
- **Quad SPI Flash** (16MB)
- **Octal SPI PSRAM** (8MB)

**Solution:** Changed memory type configuration in `platformio.ini`:
```ini
board_build.arduino.memory_type = qio_opi  # Changed from opi_opi
board_build.psram_type = opi
```

**Lesson Learned:** Always verify the exact flash and PSRAM configuration of your specific board variant. The N16R8 variant uses QIO flash, not OPI flash.

---

### 2.2 Serial Port Configuration - Dual USB Ports

**Problem:** No serial output was visible after successful firmware upload to COM6.

**Root Cause:** The ESP32-S3-DevKitC-1 has **two separate USB ports**:
1. **USB-C Port (COM6)**: Native USB-Serial/JTAG interface - used for programming
2. **Micro-USB Port (COM5)**: CH343 USB-to-UART bridge - used for Serial.print() output

**Discovery Process:**
1. Initially attempted to monitor COM6 (USB-JTAG) - no output
2. Ran `pio device list` and discovered COM5 with CH343 chip
3. Connected to COM5 and successfully received serial output

**Solution:** Use COM6 for firmware upload, COM5 for serial monitoring:
```bash
# Upload firmware
pio run --target upload --upload-port COM6

# Monitor serial output
pio device monitor --port COM5 --baud 115200
```

**Lesson Learned:** ESP32-S3 DevKitC boards with dual USB ports require understanding which port serves which function. Both can remain connected simultaneously.

---

### 2.3 PSRAM Detection Issue

**Problem:** Initial boot showed PSRAM error:
```
E (89) psram: PSRAM ID read error: 0x00ffffff
[W][esp32-hal-psram.c:71] psramInit(): PSRAM init failed!
```

**Root Cause:** Incorrect memory type configuration - using `qio_qspi` instead of `qio_opi` for PSRAM.

**Solution:** Updated `platformio.ini` with correct PSRAM configuration:
```ini
board_build.arduino.memory_type = qio_opi  # QIO flash + OPI PSRAM
board_build.psram_type = opi
```

**Verification:** After fix, boot message showed:
```
[I][esp32-hal-psram.c:96] psramInit(): PSRAM enabled
```

**Lesson Learned:** The ESP32-S3-N16R8 requires `qio_opi` memory type to properly initialize the 8MB Octal PSRAM, which is critical for TensorFlow Lite ML inference.

---

## 3. Configuration Parser Testing Results

### 3.1 Test Execution
The configuration parser was tested with hardcoded test values in `main.cpp`:

**Test Configuration String:**
```
log_interval_ms=20
flag_window_ms=3000
ml_threshold=0.8
approach_threshold_mm_s=600
jerk_threshold_m_s3=2.5
```

### 3.2 Test Results
✅ **All tests passed successfully:**

**Default Configuration:**
```
Log Interval: 16 ms
Flag Window: 2000 ms
ML Threshold: 0.70
Approach Threshold: 500.00 mm/s
Jerk Threshold: 2.00 m/s³
```

**Parsed Configuration:**
```
Parse result: SUCCESS
Log Interval: 20 ms
Flag Window: 3000 ms
ML Threshold: 0.80
Approach Threshold: 600.00 mm/s
Jerk Threshold: 2.50 m/s³
```

**Validation Result:** VALID

---

## 4. Final Configuration

### 4.1 PlatformIO Configuration (`platformio.ini`)
```ini
[env:esp32-s3-devkitc-1]
platform = espressif32
board = esp32-s3-devkitc-1
framework = arduino

monitor_speed = 115200
monitor_filters = esp32_exception_decoder

build_flags = 
    -DCORE_DEBUG_LEVEL=3
    -DBOARD_HAS_PSRAM
    -DARDUINO_USB_CDC_ON_BOOT=1
    -DARDUINO_USB_MODE=1
    -mfix-esp32-psram-cache-issue

board_build.arduino.memory_type = qio_opi
board_build.flash_size = 16MB
board_build.flash_mode = qio
board_upload.flash_size = 16MB
board_build.psram_type = opi

upload_speed = 921600
upload_port = COM6
```

### 4.2 Memory Usage
- **RAM:** 5.8% (18,892 / 327,680 bytes)
- **Flash:** 22.3% (292,233 / 1,310,720 bytes)
- **PSRAM:** 8MB available for TensorFlow Lite operations

---

## 5. Key Takeaways

### 5.1 Technical Insights
1. **Flash Configuration is Critical:** Mismatched flash/PSRAM configuration causes boot failures
2. **Dual USB Ports:** ESP32-S3 DevKitC has separate ports for programming and serial communication
3. **PSRAM Essential for ML:** Proper PSRAM initialization is required for TensorFlow Lite inference
4. **Modular Design:** Separating configuration parsing into its own class improves maintainability

### 5.2 Development Best Practices
1. Always verify hardware specifications before configuring build settings
2. Use `pio device list` to identify correct COM ports
3. Test configuration changes incrementally
4. Implement validation at the parsing stage to catch errors early
5. Provide default values as fallback for robustness

### 5.3 Troubleshooting Workflow
1. Check boot messages for specific error codes
2. Verify hardware variant matches software configuration
3. Identify correct serial ports for programming vs. monitoring
4. Test PSRAM initialization before proceeding with ML implementation

---

## 6. Next Steps

With the configuration parser successfully implemented and hardware properly configured, the project is ready to proceed with:
1. SD card integration for reading config.txt from storage
2. Sensor initialization (VL53L8CX ToF, BNO08x IMU)
3. TensorFlow Lite model integration
4. Data logging implementation

---

## Appendix: Command Reference

### Upload Firmware
```bash
pio run --target upload --upload-port COM6
```

### Monitor Serial Output
```bash
pio device monitor --port COM5 --baud 115200
```

### List Available Ports
```bash
pio device list
```

### Build Only
```bash
pio run
```

---

**Document Version:** 1.0  
**Last Updated:** December 8, 2024  
**Status:** Configuration Parser Implementation Complete ✅
