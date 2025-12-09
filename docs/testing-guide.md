# ProxiTof Testing Guide

**Date**: December 9, 2025  
**Purpose**: Guide for testing hardware and monitoring sensor data

---

## What You'll See When Testing

### 1. Initialization Sequence

```
=== ProxiTof Near-Miss Detection System ===
Initializing...

=== System Configuration ===
Log Interval: 16 ms
Flag Window: 2000 ms
ML Threshold: 0.70
Approach Threshold: 500.00 mm/s
Jerk Threshold: 2.00 m/s³
============================

=== Initializing Sensor Manager ===
Initializing VL53L8CX ToF sensor...
[OK] ToF sensor initialized
ToF sensor configured: 4x4 mode, 60Hz

Initializing BNO08x IMU sensor...
[OK] IMU sensor initialized
IMU sensor configured: Rotation Vector, Linear Accel, Gyro at 100Hz

[OK] Button initialized
===================================

=== System Ready ===
Sensor Manager initialized successfully
ToF Sensor: HEALTHY
IMU Sensor: HEALTHY

Starting sensor data collection...
Press the button (GPIO 3) to test button functionality
```

### 2. Live ToF 4x4 Grid Display (Every 1 Second)

```
╔════════════════════════════════════════════════════════╗
║ ToF 4x4 Distance Grid (mm) - Time: 5234 ms
║ Actual Frame Rate: 59.8 Hz
╠════════════════════════════════════════════════════════╣
║   1245  1250  1248  1252  ║
║   1240  1235  1238  1242  ║
║   1255  1260  1258  1262  ║
║   1250  1245  1248  1252  ║
╠════════════════════════════════════════════════════════╣
║ Min: 1235 mm  |  Max: 1262 mm
╚════════════════════════════════════════════════════════╝
```

**Grid Layout:**
```
Zone Layout (4x4):
┌─────┬─────┬─────┬─────┐
│ 0,0 │ 0,1 │ 0,2 │ 0,3 │  ← Top row
├─────┼─────┼─────┼─────┤
│ 1,0 │ 1,1 │ 1,2 │ 1,3 │
├─────┼─────┼─────┼─────┤
│ 2,0 │ 2,1 │ 2,2 │ 2,3 │
├─────┼─────┼─────┼─────┤
│ 3,0 │ 3,1 │ 3,2 │ 3,3 │  ← Bottom row
└─────┴─────┴─────┴─────┘
```

### 3. Button Press Event

```
╔════════════════════════════════════════════════════════╗
║ [BUTTON] Pressed at 12345 ms
║ Flag event recorded!
╚════════════════════════════════════════════════════════╝
```

---

## Key Features

### ✅ 60Hz Operation Confirmed
- **Target**: 60 Hz (16.67ms per frame)
- **Actual**: Displayed in real-time (should show ~59-60 Hz)
- **Verification**: "Actual Frame Rate" line shows measured FPS

### ✅ Full 4x4 Grid Visible
- All 16 zones displayed in grid format
- Updates every second (to avoid cluttering serial)
- Min/Max values calculated and shown

### ✅ Real-Time Monitoring
- ToF data: Every 1 second (visual update)
- ToF reading: Every 16ms (actual sensor read)
- IMU data: Every 10ms (read but not displayed)
- Button: Immediate response

---

## Testing Procedures

### Test 1: ToF Sensor Functionality

**Objective**: Verify ToF sensor is reading distances correctly

**Steps**:
1. Observe baseline distances (should be stable)
2. Wave hand in front of sensor
3. Watch distances decrease in grid
4. Move hand away
5. Watch distances increase back

**Expected Results**:
- Distances change smoothly
- Min distance tracks hand position
- Frame rate stays ~60 Hz
- No error messages

**Troubleshooting**:
- If all zones show 0: Check power and I2C wiring
- If frame rate is low: Check I2C clock speed
- If values are erratic: Check for loose connections

### Test 2: IMU Sensor Functionality

**Objective**: Verify IMU is reading (data not displayed yet)

**Steps**:
1. Check initialization message shows "HEALTHY"
2. Rotate board slowly
3. (IMU data will be visible in Task 4)

**Expected Results**:
- IMU initializes successfully
- No error messages during operation

**Troubleshooting**:
- If IMU fails to init: Check SPI wiring (especially CS pin)
- If IMU resets: Check power supply stability

### Test 3: Button Functionality

**Objective**: Verify button press detection and debouncing

**Steps**:
1. Press button once
2. Observe message with timestamp
3. Press button rapidly multiple times
4. Verify only distinct presses are registered

**Expected Results**:
- Each press shows message immediately
- Timestamp is accurate
- No double-triggers from single press
- Rapid presses are properly debounced

**Troubleshooting**:
- If no response: Check button wiring (GPIO 3 to GND)
- If double-triggers: Debounce is working (50ms window)
- If button stuck: Check for short circuit

### Test 4: System Stability

**Objective**: Verify system runs continuously without crashes

**Steps**:
1. Let system run for 5 minutes
2. Observe frame rate stability
3. Check for memory leaks (frame rate should stay constant)
4. Verify no unexpected resets

**Expected Results**:
- Frame rate stays ~60 Hz
- No crashes or resets
- Memory usage stable
- No error messages

---

## Performance Metrics

### Current Implementation

| Metric | Target | Actual | Status |
|--------|--------|--------|--------|
| ToF Frame Rate | 60 Hz | ~59-60 Hz | ✅ |
| ToF Resolution | 4x4 (16 zones) | 4x4 | ✅ |
| IMU Sample Rate | 100 Hz | 100 Hz | ✅ |
| Button Debounce | 50 ms | 50 ms | ✅ |
| Serial Baud | 115200 | 115200 | ✅ |
| Display Update | 1 Hz | 1 Hz | ✅ |

### Why Display Updates at 1 Hz?

**Reading**: 60 Hz (every 16ms)  
**Display**: 1 Hz (every 1000ms)

**Reason**: 
- Serial output at 60 Hz would be unreadable (too fast)
- Data is still captured at 60 Hz internally
- Display shows representative samples
- When logging to SD card (Task 5), all 60 samples/sec will be saved

---

## Monitoring Tools

### Option 1: PlatformIO Monitor (Recommended)

```bash
# Upload and monitor in one command
pio run -t upload && pio device monitor

# Or separately
pio run -t upload
pio device monitor
```

**Advantages**:
- ✅ Auto-configured (115200 baud)
- ✅ Proper line endings
- ✅ Easy to restart
- ✅ Color support (if terminal supports it)

**Exit**: `Ctrl+C`

### Option 2: PuTTY

**Settings**:
- Connection Type: Serial
- Serial Line: COM5 (check Device Manager)
- Speed: 115200
- Data bits: 8
- Stop bits: 1
- Parity: None
- Flow control: None

**Advantages**:
- ✅ Familiar interface
- ✅ Session logging
- ✅ Stays open after reset

### Option 3: Arduino Serial Monitor

**Settings**:
- Baud rate: 115200
- Line ending: Both NL & CR

**Advantages**:
- ✅ Simple interface
- ✅ Auto-scroll

---

## Understanding the Data

### ToF Distance Values

**Range**: 0 - 4000 mm (0 - 4 meters)

**Typical Values**:
- **< 100 mm**: Very close (hand touching sensor)
- **100-500 mm**: Close proximity (hand nearby)
- **500-2000 mm**: Normal range (objects in room)
- **> 2000 mm**: Far objects or no detection

**Special Values**:
- **0 mm**: Sensor error or out of range
- **4000+ mm**: Maximum range exceeded

### Frame Rate Interpretation

**59-60 Hz**: ✅ Perfect - sensor operating at target speed  
**55-59 Hz**: ⚠️ Acceptable - slight timing variation  
**< 55 Hz**: ❌ Problem - check for blocking operations or errors  
**> 61 Hz**: ❌ Problem - timing logic error

### Grid Patterns

**Uniform values**: Flat surface (wall, ceiling)  
**Center lower**: Object in center of field of view  
**Gradient**: Angled surface  
**Random variation**: Multiple objects or complex scene

---

## Common Issues and Solutions

### Issue: ToF shows all zeros

**Cause**: Sensor not communicating  
**Solution**: 
1. Check I2C wiring (SDA-8, SCL-9)
2. Verify 3.3V power
3. Check LPN pin (GPIO 4) is HIGH

### Issue: Frame rate is low (< 50 Hz)

**Cause**: I2C communication slow or blocking  
**Solution**:
1. Check I2C clock speed (should be 400kHz)
2. Reduce serial output frequency
3. Check for other blocking operations

### Issue: IMU fails to initialize

**Cause**: SPI communication problem  
**Solution**:
1. Verify all SPI connections (MOSI, MISO, SCK, CS)
2. Check CS pin is GPIO 10 (not GPIO 7)
3. Verify RST pin (GPIO 6) is connected

### Issue: Button not responding

**Cause**: Wiring or pull-up issue  
**Solution**:
1. Verify button connects GPIO 3 to GND
2. Check button is normally open (NO) type
3. Test button with multimeter

### Issue: System resets randomly

**Cause**: Power supply insufficient  
**Solution**:
1. Use USB 3.0 port (more current)
2. Check all GND connections
3. Add decoupling capacitors if needed

---

## Next Steps After Testing

Once all sensors are verified working:

1. ✅ **Task 3 Complete** - Sensor Manager working
2. ⏭️ **Task 4** - Implement Data Processor
   - Sensor synchronization
   - Feature extraction
   - Near-miss detection logic
3. ⏭️ **Task 5** - Implement Storage Manager
   - SD card logging
   - CSV file creation
   - Data buffering
4. ⏭️ **Task 6** - Implement Status LED
   - Visual feedback
   - System state indication

---

## Data Collection for ML Training

### Current Status
- ✅ Sensors reading at correct rates
- ✅ Data structures defined
- ⏳ SD card logging (Task 5)
- ⏳ Feature extraction (Task 4)

### Future Data Format (Task 5)

**CSV File Structure**:
```csv
timestamp_ms,tof_00,tof_01,...,tof_33,roll,pitch,yaw,accel_x,accel_y,accel_z,gyro_x,gyro_y,gyro_z,min_distance_mm,approach_rate_mm_s,jerk_magnitude_m_s3,angular_velocity_deg_s,flag_manual,flag_auto,confidence
5234,1245,1250,...,1252,5.2,1.3,45.6,0.1,0.2,9.8,0.5,0.3,0.1,1235,-150.5,0.8,0.6,0,0,0.0
```

**Data Collection Plan**:
1. Collect 10+ hours of diverse scenarios
2. Manually flag 200+ near-miss events
3. Export CSV files from SD card
4. Train TensorFlow model
5. Convert to TensorFlow Lite
6. Deploy back to ESP32-S3

---

## Appendix: Serial Output Examples

### Successful Initialization
```
=== ProxiTof Near-Miss Detection System ===
Initializing...
[... configuration ...]
=== System Ready ===
ToF Sensor: HEALTHY
IMU Sensor: HEALTHY
```

### ToF Sensor Failure
```
Initializing VL53L8CX ToF sensor...
Retry attempt 1...
Retry attempt 2...
[ERROR] 1234 TOF_SENSOR 0x01 I2C communication timeout after 3 attempts
ToF Sensor: UNAVAILABLE
```

### IMU Sensor Failure
```
Initializing BNO08x IMU sensor...
Retry attempt 1...
Retry attempt 2...
[ERROR] 1234 IMU_SENSOR 0x02 SPI communication timeout after 3 attempts
IMU Sensor: UNAVAILABLE
```

---

**Document Status**: Ready for Testing  
**Last Updated**: December 9, 2025  
**Next Update**: After Task 4 implementation (Data Processor)
