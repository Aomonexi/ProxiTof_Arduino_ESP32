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

## 6. Task Completion Summary

### 6.1 Completed Tasks
The following tasks from the implementation plan have been successfully completed:

#### ✅ Task 1: Set up project structure and Git repository
- Created PlatformIO project with ESP32-S3 configuration
- Initialized directory structure: src/, lib/, include/, data/, docs/
- Configured platformio.ini with correct board settings and library dependencies
- Created README.md with hardware setup information

#### ✅ Task 2.1: Create data model header files
- Defined ToFData, IMUData, SyncedData, FeatureVector, LogEntry structs in `include/DataModels.h`
- Defined SystemConfig struct with default values
- Defined SystemState enum and LEDState enum

#### ✅ Task 2.2: Implement configuration file parser
- Implemented `SystemConfig.cpp` with complete parsing functionality
- Created `parseFromString()` method for key=value format parsing
- Implemented `setParameter()` with parameter validation and range checking:
  - log_interval_ms: 10-1000ms
  - flag_window_ms: 500-10000ms
  - ml_threshold: 0.1-1.0
  - approach_threshold_mm_s: 50-2000 mm/s
  - jerk_threshold_m_s3: 0.5-20 m/s³
- Implemented default value fallback mechanism for missing/invalid configuration
- Added `validateParameters()` method for comprehensive validation
- Added `printConfig()` method for debugging and verification
- Successfully tested with hardcoded configuration strings

### 6.2 Additional Accomplishments
Beyond the planned tasks, several critical issues were identified and resolved:

1. **Flash Configuration Fix:** Corrected memory type from `opi_opi` to `qio_opi` to match hardware
2. **PSRAM Initialization:** Successfully enabled 8MB PSRAM for TensorFlow Lite operations
3. **Serial Port Identification:** Documented dual USB port configuration (COM6 for upload, COM5 for monitoring)
4. **Documentation:** Created comprehensive development journey documentation

### 6.3 Verification Results
All implemented functionality has been verified:
- ✅ Configuration parsing works correctly
- ✅ Parameter validation enforces defined ranges
- ✅ Default values are applied when parameters are missing
- ✅ PSRAM is properly initialized and available
- ✅ Serial communication works on correct port
- ✅ System boots without errors

---

## 7. Next Steps

With the configuration parser successfully implemented and hardware properly configured, the project is ready to proceed with:
1. **Task 3.1:** Sensor Manager initialization (VL53L8CX ToF, BNO08x IMU)
2. SD card integration for reading config.txt from storage
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

---

## 8. Property-Based Testing Implementation

### 8.1 Task Objective
Implement property-based tests for configuration parsing to validate:
- **Property 50:** Configuration parameter application (Requirements 12.2)
- **Property 51:** Configuration fallback behavior (Requirements 12.3)

### 8.2 Why Property-Based Testing?

Property-based testing (PBT) is a powerful testing methodology that validates software behavior across a wide range of inputs rather than testing specific examples. Unlike traditional unit tests that check specific cases, PBT:

1. **Tests Universal Properties:** Validates that certain properties hold true for ALL valid inputs, not just hand-picked examples
2. **Discovers Edge Cases:** Automatically generates hundreds of test cases, including edge cases developers might not think of
3. **Provides Higher Confidence:** 100 iterations with random inputs provide much stronger evidence of correctness than 3-4 manual test cases
4. **Catches Specification Bugs:** Often reveals ambiguities or errors in the specification itself

**For Configuration Parsing Specifically:**
- We need to ensure the parser handles ANY valid configuration correctly
- We need to ensure the parser NEVER crashes on invalid input
- Manual testing of all possible parameter combinations would be impractical
- Random generation tests the parser's robustness across the entire input space

### 8.3 Implementation Details

**Test File:** `test/test_config_properties.cpp`

**Framework:** Unity Test Framework (Arduino-compatible)

**Test Structure:**
```cpp
// Property 50: Configuration parameter application
// Tests that valid parameters are correctly applied
for (int i = 0; i < 100; i++) {
    // Generate random valid configuration
    uint16_t log_interval = randomInt(10, 1000);
    float ml_threshold = randomFloat(0.1f, 1.0f);
    // ... generate other parameters
    
    // Parse and verify all parameters match
    SystemConfig config;
    config.parseFromString(configString);
    
    // Assert all values were applied correctly
    TEST_ASSERT_EQUAL(expected, config.log_interval_ms);
    // ... verify other parameters
}
```

**Test Coverage:**
1. **Property 50 - Parameter Application (100 iterations)**
   - Generates random valid values for all 5 configuration parameters
   - Verifies each parameter is correctly parsed and applied
   - Tests integer parameters (log_interval_ms, flag_window_ms)
   - Tests float parameters (ml_threshold, approach_threshold_mm_s, jerk_threshold_m_s3)

2. **Property 51 - Fallback Behavior (100 iterations)**
   - Tests empty configuration (missing file scenario)
   - Tests invalid parameter values (out of range)
   - Tests malformed syntax (missing equals, invalid keys)
   - Verifies system always has valid defaults after parsing

3. **Edge Case Tests**
   - Comments and whitespace handling
   - Partial configuration (some parameters missing)
   - Leading/trailing whitespace in keys and values

### 8.4 PlatformIO Test Configuration Challenge

**Problem:** Initial test runs failed with duplicate `setup()` and `loop()` function definitions:
```
multiple definition of `setup()'; test file first defined here
multiple definition of `loop()'; main.cpp first defined here
```

**Root Cause:** PlatformIO's test framework was building both `src/main.cpp` and the test file, causing symbol conflicts.

**Solution:** Created a separate test environment in `platformio.ini`:
```ini
[env:test]
platform = espressif32
board = esp32-s3-devkitc-1
framework = arduino
test_build_src = yes
build_src_filter = +<*> -<main.cpp>  # Exclude main.cpp from test builds
```

**Lesson Learned:** When writing tests for Arduino projects, create a dedicated test environment that excludes the main application entry point to avoid symbol conflicts.

### 8.5 Test Execution Results

**Command:** `pio test -e test`

**Results:** ✅ **ALL TESTS PASSED**

```
test/test_config_properties.cpp:495: property_50_config_parameter_application [PASSED]
test/test_config_properties.cpp:496: property_51_config_fallback              [PASSED]
test/test_config_properties.cpp:499: test_config_comments_and_whitespace      [PASSED]
test/test_config_properties.cpp:500: test_partial_config                      [PASSED]

4 test cases: 4 succeeded
```

**Test Statistics:**
- **Total Iterations:** 200+ (100 for Property 50, 100 for Property 51, plus edge cases)
- **Test Duration:** ~113 seconds (includes firmware upload to ESP32-S3)
- **Coverage:** All 5 configuration parameters tested across valid and invalid ranges
- **Edge Cases:** Comments, whitespace, partial configs, malformed syntax

### 8.6 What the Tests Validate

**Property 50 validates that:**
- ✅ Valid integer parameters are parsed correctly (log_interval_ms, flag_window_ms)
- ✅ Valid float parameters are parsed correctly (ml_threshold, approach_threshold_mm_s, jerk_threshold_m_s3)
- ✅ All parameters within valid ranges are accepted
- ✅ Parsing succeeds for any valid configuration format
- ✅ No data loss or corruption during parsing

**Property 51 validates that:**
- ✅ Empty configuration falls back to safe defaults
- ✅ Out-of-range values are rejected and defaults are used
- ✅ Malformed syntax doesn't crash the parser
- ✅ Unknown parameters are ignored gracefully
- ✅ System always ends up in a valid state after parsing

**Edge case tests validate that:**
- ✅ Comments (lines starting with #) are ignored
- ✅ Leading/trailing whitespace is handled correctly
- ✅ Partial configurations work (missing parameters use defaults)
- ✅ Whitespace around equals signs is tolerated

### 8.7 Benefits Achieved

1. **High Confidence:** 200+ test iterations provide strong evidence the parser is robust
2. **Regression Prevention:** Future changes to the parser will be validated against these properties
3. **Documentation:** Tests serve as executable specification of parser behavior
4. **Early Bug Detection:** Would catch issues like:
   - Integer overflow in parameter parsing
   - Float precision loss
   - Buffer overflows in string handling
   - Missing validation checks

### 8.8 Task Completion

#### ✅ Task 2.3: Write property test for configuration parsing
- Implemented custom property-based testing framework for Arduino
- Created 4 comprehensive test functions
- Validated Properties 50 and 51 from design document
- All tests passing with 100+ iterations each
- Successfully integrated with PlatformIO test framework

---

## Document History

| Version | Date | Changes |
|---------|------|---------|
| 1.0 | December 8, 2024 | Initial documentation - Configuration parser implementation |
| 1.1 | December 8, 2024 | Added Task Completion Summary section |
| 1.2 | December 8, 2024 | Added Property-Based Testing implementation and results |

**Current Status:** Tasks 1, 2.1, 2.2, and 2.3 Complete ✅  
**Next Task:** Task 3.1 - Sensor Manager initialization
