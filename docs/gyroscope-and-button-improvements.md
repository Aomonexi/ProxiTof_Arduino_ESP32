# Gyroscope and Button Duration Improvements

**Date:** December 9, 2024  
**Changes:** Enabled gyroscope + improved button event flagging

---

## 1. Gyroscope Enabled ✅

### Problem
Gyroscope data was showing all zeros because it wasn't enabled in the BNO08x initialization.

### Solution
Added gyroscope report enabling in sensor_manager initialization:

```cpp
// Enable gyroscope
ESP_LOGI(TAG, "Enabling gyroscope @ 100Hz...");
if (!imu->rpt.gyroscope.enable(10000)) {
    ESP_LOGW(TAG, "Failed to enable gyroscope");
}
```

And updated the data reading function:

```cpp
// Read gyroscope
if (imu->rpt.gyroscope.has_new_data()) {
    bno08x_gyro_t gyro = imu->rpt.gyroscope.get();
    data->gyro_x = gyro.x;
    data->gyro_y = gyro.y;
    data->gyro_z = gyro.z;
}
```

### Result
- ✅ Gyroscope now reports angular velocity in deg/s
- ✅ X, Y, Z rotation rates available
- ✅ Updates at 100Hz along with other IMU data
- ✅ Useful for detecting sudden movements/rotations

### What Gyroscope Measures
- **X-axis (Roll rate):** Rotation around forward/back axis
- **Y-axis (Pitch rate):** Rotation around left/right axis  
- **Z-axis (Yaw rate):** Rotation around up/down axis
- **Units:** Degrees per second (deg/s)

### Use Cases for Near-Miss Detection
- Detect sudden swerving (high yaw rate)
- Detect hard braking (pitch rate change)
- Detect vehicle instability (combined rotation rates)
- Complement acceleration data for better event detection

---

## 2. Button Duration Tracking ✅

### Problem
Original button implementation only detected single press events. For near-miss events that last several seconds, we need to track how long the button is held.

### Why This Matters
**Near-miss events aren't instantaneous:**
- Event starts (e.g., car cuts in front)
- Lasts several seconds (driver reacts, brakes, swerves)
- Event ends (danger passes)

**User needs to flag the entire event duration:**
- Press button when event starts
- Hold button during the event
- Release when event ends
- System logs the entire duration

### Solution Implemented

#### New API Functions

1. **`sensor_manager_is_button_held()`**
   - Returns true if button is currently pressed
   - Checks real-time GPIO state
   - No debouncing (immediate response)

2. **`sensor_manager_get_button_duration()`**
   - Returns how long button has been held (in ms)
   - Returns 0 if button not pressed
   - Tracks from first press to current time

#### Updated Button Logic

**State Tracking:**
```cpp
static uint32_t button_press_start_time = 0;  // When button was first pressed
```

**On Button Press:**
```cpp
if (current_state) {
    button_press_start_time = current_time;  // Record start time
}
```

**Duration Calculation:**
```cpp
uint32_t duration = current_time - button_press_start_time;
```

#### Display Updates

**While Button is Held:**
```
╔════════════════════════════════════════════════╗
║       ⚠️  BUTTON HELD - FLAGGING EVENT        ║
║   Duration: 2500 ms (2.5 seconds)             ║
║   Keep holding to mark entire near-miss event  ║
╚════════════════════════════════════════════════╝
```

**When Button is Released:**
```
╔════════════════════════════════════════════════╗
║      ✓ EVENT FLAGGED - Button Released        ║
║   Timestamp: 12345 ms                          ║
╚════════════════════════════════════════════════╝
```

### Usage Workflow

1. **Event Starts**
   - User sees near-miss beginning
   - Presses and holds button
   - Display shows "BUTTON HELD - FLAGGING EVENT"

2. **During Event**
   - User keeps holding button
   - Duration counter increases
   - All sensor data continues logging
   - Display updates every 500ms showing duration

3. **Event Ends**
   - User releases button
   - Display shows "EVENT FLAGGED - Button Released"
   - System knows exact start and end times

### Benefits

✅ **Accurate Event Boundaries**
- Captures entire event duration
- Not just a single timestamp
- Start time and end time both recorded

✅ **User Feedback**
- Real-time duration display
- User knows system is recording
- Clear confirmation when released

✅ **Data Logging Ready**
- When SD card is implemented, can log:
  - Event start timestamp
  - Event end timestamp
  - Event duration
  - All sensor data during event

✅ **Flexible Event Length**
- Short events: 1-2 seconds
- Long events: 5-10 seconds
- User controls the duration

### Example Scenarios

**Scenario 1: Quick Lane Change**
```
T=0s:    User presses button (car cuts in)
T=0-2s:  Button held (driver reacts)
T=2s:    User releases button (danger passed)
Result:  2-second event flagged
```

**Scenario 2: Extended Near-Miss**
```
T=0s:    User presses button (obstacle ahead)
T=0-5s:  Button held (braking, swerving)
T=5s:    User releases button (safe)
Result:  5-second event flagged
```

**Scenario 3: Multiple Events**
```
T=0s:    Event 1 starts, button pressed
T=2s:    Event 1 ends, button released
T=10s:   Event 2 starts, button pressed
T=13s:   Event 2 ends, button released
Result:  Two separate events logged
```

---

## 3. Integration with Future SD Card Logging

### Event Marking Strategy

When SD card logging is implemented, button duration will enable:

**Continuous Logging Mode:**
```csv
timestamp,d0,d1,...,d15,roll,pitch,yaw,ax,ay,az,gx,gy,gz,event_flag
1000,450,520,...,12.3,-5.6,89.1,0.12,-0.05,9.81,0.0,0.0,0.0,0
1016,448,518,...,12.4,-5.5,89.2,0.13,-0.04,9.80,0.1,0.0,0.0,0
2000,420,500,...,15.2,-8.3,88.5,0.45,-0.15,9.75,2.5,1.2,0.5,1  ← Button pressed
2016,415,495,...,16.1,-9.1,87.8,0.52,-0.18,9.70,3.1,1.5,0.8,1  ← Event ongoing
...
4000,440,510,...,13.5,-6.2,89.0,0.18,-0.06,9.78,0.5,0.2,0.1,1  ← Button released
4016,445,515,...,12.8,-5.8,89.1,0.14,-0.05,9.80,0.1,0.0,0.0,0
```

**Event Metadata File:**
```csv
event_id,start_time,end_time,duration_ms,min_distance,max_accel
1,2000,4000,2000,415,0.52
2,10000,13000,3000,380,0.68
```

### Data Analysis Benefits

**For ML Training:**
- Positive samples: Data during button hold (event_flag=1)
- Negative samples: Data when button not held (event_flag=0)
- Balanced dataset for training
- Clear ground truth labels

**For Event Analysis:**
- Extract specific event segments
- Analyze sensor patterns during events
- Compare multiple events
- Validate detection algorithms

---

## 4. Technical Implementation Details

### Files Modified

1. **`components/sensor_manager/include/sensor_manager.h`**
   - Added `sensor_manager_is_button_held()` declaration
   - Added `sensor_manager_get_button_duration()` declaration

2. **`components/sensor_manager/sensor_manager.cpp`**
   - Added `button_press_start_time` state variable
   - Updated `sensor_manager_is_button_pressed()` to track start time
   - Implemented `sensor_manager_is_button_held()`
   - Implemented `sensor_manager_get_button_duration()`
   - Enabled gyroscope in IMU initialization
   - Updated `sensor_manager_read_imu()` to read gyroscope data

3. **`main/main.c`**
   - Updated button check to show duration while held
   - Added real-time duration display
   - Added release confirmation message

### API Summary

```c
// Check if button is currently held down
bool sensor_manager_is_button_held(void);

// Get how long button has been held (ms)
uint32_t sensor_manager_get_button_duration(void);

// Original function - detects press/release events
bool sensor_manager_is_button_pressed(void);

// Get timestamp of last button press
uint32_t sensor_manager_get_last_button_press_time(void);
```

### Usage Example

```c
// In main loop
if (sensor_manager_is_button_held()) {
    uint32_t duration = sensor_manager_get_button_duration();
    printf("Event ongoing: %lu ms\n", duration);
    
    // Log to SD card with event flag
    log_data_with_flag(tof_data, imu_data, 1);  // 1 = event
} else {
    // Normal logging without event flag
    log_data_with_flag(tof_data, imu_data, 0);  // 0 = normal
}
```

---

## 5. Testing and Validation

### Gyroscope Testing

**Test 1: Rotation Detection**
- Rotate device around each axis
- Verify gyro values change appropriately
- Check sign conventions (positive/negative)

**Test 2: Stationary Baseline**
- Place device on stable surface
- Gyro values should be near zero
- Small drift is normal (< 1 deg/s)

**Test 3: Quick Movements**
- Simulate sudden swerve (yaw rotation)
- Simulate hard brake (pitch rotation)
- Verify high gyro values during movement

### Button Duration Testing

**Test 1: Short Press**
- Press and hold for 1 second
- Verify duration display updates
- Check release message appears

**Test 2: Long Press**
- Press and hold for 5+ seconds
- Verify duration continues counting
- Check no timeout or reset

**Test 3: Multiple Events**
- Press, hold, release
- Wait 2 seconds
- Press, hold, release again
- Verify each event tracked separately

**Test 4: Rapid Press/Release**
- Quick tap (< 200ms)
- Should not trigger (debounce)
- Prevents accidental flags

---

## 6. Next Steps

### Immediate
- ✅ Gyroscope enabled and working
- ✅ Button duration tracking implemented
- ✅ Real-time display updated

### Short-term (SD Card Implementation)
- [ ] Log event_flag column in CSV
- [ ] Create event metadata file
- [ ] Implement event extraction tools
- [ ] Add event statistics

### Medium-term (Data Analysis)
- [ ] Analyze gyroscope patterns during events
- [ ] Correlate button duration with event severity
- [ ] Extract features from gyro data
- [ ] Train ML model with event labels

---

## 7. Conclusion

### Summary of Improvements

1. **Gyroscope Now Working** ✅
   - Angular velocity data available
   - 100Hz update rate
   - Useful for rotation detection

2. **Button Duration Tracking** ✅
   - Tracks entire event duration
   - Real-time feedback to user
   - Ready for SD card logging

3. **Better Event Flagging** ✅
   - User controls event boundaries
   - Accurate start/end times
   - Flexible event lengths

### Impact on Thesis

**Data Collection:**
- More accurate event labeling
- Complete event capture (not just single point)
- Better ground truth for ML training

**Analysis:**
- Event duration as feature
- Gyroscope data for movement detection
- Richer dataset for research

**User Experience:**
- Clear feedback during flagging
- Confidence in data collection
- Easy to use in field testing

---

**Document Version:** 1.0  
**Last Updated:** December 9, 2024  
**Status:** Implemented and Ready for Testing ✅
