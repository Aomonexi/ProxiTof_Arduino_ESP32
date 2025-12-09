# Implementation Status and Next Steps

**Date:** December 9, 2024  
**Current Phase:** Sensor Integration Complete ✅  
**Next Phase:** Data Logging and Processing

---

## Completed Tasks ✅

### 1. Project Setup ✅
- [x] ESP-IDF project structure
- [x] Git repository with proper .gitignore
- [x] Component architecture
- [x] Build system configuration
- [x] Documentation structure

### 2. Data Structures ✅
- [x] ToFData, IMUData structs defined
- [x] Configuration parser implemented
- [x] System state management

### 3. Sensor Manager Component ✅
- [x] VL53L8CX ToF sensor integration (60Hz, 4x4 grid)
- [x] BNO08x IMU integration (100Hz, orientation + acceleration)
- [x] Button input with debouncing
- [x] Timestamp generation
- [x] Health monitoring
- [x] Error handling

### 4. Display System ✅
- [x] Real-time 4x4 ToF grid visualization
- [x] LED distance indicator (RED/YELLOW/GREEN)
- [x] IMU data display
- [x] Frame counter and status

### 5. Performance Optimization ✅
- [x] Identified UART overflow issue
- [x] Reduced display rate (20Hz → 2Hz)
- [x] Updated .gitignore for build artifacts

---

## Current Issues and Solutions

### Issue 1: System Slowdown After 5 Minutes ✅ FIXED

**Problem:** Excessive UART logging (800 lines/sec) causing buffer overflow

**Solution Applied:**
- Reduced display update rate from 20Hz to 2Hz
- Changed delay from 50ms to 500ms
- System now stable for extended operation

**Result:** No more slowdown, system runs indefinitely

### Issue 2: Build Artifacts in Git ✅ FIXED

**Problem:** `build/`, `managed_components/` should not be in repository

**Solution Applied:**
- Updated .gitignore to exclude:
  - `build/`
  - `managed_components/`
  - `sdkconfig.old`
  - `dependencies.lock`

**Result:** Clean repository, reproducible builds

---

## Next Implementation Tasks

### Priority 1: SD Card Data Logging 🎯 NEXT

**Why This is Critical:**
- Current UART logging is too slow for ML training data
- Need to collect large datasets (hours of sensor data)
- SD card provides high-speed storage (MB/s vs KB/s)
- Essential for offline analysis and model training

**Implementation Steps:**

1. **Hardware Setup** (30 minutes)
   - Connect SD card module via SPI
   - Recommended pins:
     ```
     SD Card    ESP32-S3
     --------   ---------
     MOSI   ->  GPIO 35
     MISO   ->  GPIO 37
     SCK    ->  GPIO 36
     CS     ->  GPIO 34
     ```

2. **Create SD Logger Component** (2 hours)
   ```
   components/sd_logger/
   ├── sd_logger.c
   ├── include/sd_logger.h
   ├── CMakeLists.txt
   └── idf_component.yml
   ```

3. **Implement CSV Logging** (1 hour)
   - Format: `timestamp,d0,d1,...,d15,roll,pitch,yaw,ax,ay,az`
   - Buffered writes for performance
   - File rotation at size limit

4. **Add Logging Control** (30 minutes)
   - Button press to start/stop logging
   - LED indicator for logging status
   - Session management

**Expected Outcome:**
- ✅ 60Hz ToF + 100Hz IMU logging to SD card
- ✅ No performance degradation
- ✅ Hours of continuous data collection
- ✅ Ready for ML training dataset

**Files to Create:**
- `components/sd_logger/sd_logger.c`
- `components/sd_logger/include/sd_logger.h`
- `components/sd_logger/CMakeLists.txt`
- `components/sd_logger/idf_component.yml`

**Dependencies to Add:**
```yaml
# components/sd_logger/idf_component.yml
dependencies:
  espressif/esp_vfs_fat: "^1.0.0"
```

### Priority 2: GNSS Integration 🌍

**Why Important:**
- Location tracking for near-miss events
- Timestamp synchronization with GPS time
- Speed and heading information

**Implementation Steps:**

1. **Hardware Selection**
   - Recommended: NEO-6M or NEO-M8N GPS module
   - Interface: UART
   - Pins: TX/RX (e.g., GPIO 17/18)

2. **Create GNSS Component**
   - NMEA parser for GPS data
   - Extract: latitude, longitude, speed, heading, time
   - Validate GPS fix quality

3. **Integration**
   - Add GPS data to log entries
   - Synchronize system time with GPS
   - Display GPS status

**Expected Outcome:**
- ✅ Location tracking
- ✅ Accurate timestamps
- ✅ Speed/heading data for context

### Priority 3: Data Processor Component

**Purpose:**
- Synchronize ToF and IMU data
- Extract features for ML model
- Prepare data for inference

**Implementation:**
- Sensor fusion (align timestamps)
- Feature extraction (min distance, approach rate, etc.)
- Data buffering for ML input

### Priority 4: TensorFlow Lite Integration

**Purpose:**
- Load pre-trained ML model
- Run inference on device
- Detect near-miss events in real-time

**Implementation:**
- Port TFLite Micro to ESP-IDF
- Load model from flash/SD card
- Implement inference pipeline
- Optimize for PSRAM

---

## Recommended Implementation Order

### Week 1: Data Collection Infrastructure
1. ✅ **Day 1-2:** Sensor integration (DONE)
2. 🎯 **Day 3:** SD card logging implementation
3. 🌍 **Day 4:** GNSS integration
4. 📊 **Day 5:** Data collection testing

### Week 2: Data Processing
5. **Day 6-7:** Data processor component
6. **Day 8-9:** Feature extraction
7. **Day 10:** Data validation and quality checks

### Week 3: ML Integration
8. **Day 11-12:** TensorFlow Lite setup
9. **Day 13-14:** Model loading and inference
10. **Day 15:** Performance optimization

### Week 4: Testing and Validation
11. **Day 16-17:** Field testing
12. **Day 18-19:** Dataset collection
13. **Day 20:** System validation

---

## Technical Decisions to Make

### 1. SD Card Logging Strategy

**Option A: Continuous Logging**
- Log every sample to SD card
- Pros: Complete dataset
- Cons: High SD card wear, large files

**Option B: Event-Based Logging** ⭐ RECOMMENDED
- Log continuously but mark events
- Button press flags interesting segments
- Pros: Efficient, focused dataset
- Cons: Might miss some events

**Option C: Circular Buffer**
- Keep last N seconds in RAM
- Save to SD on button press
- Pros: Captures pre-event data
- Cons: Limited buffer size

**Recommendation:** Hybrid approach
- Continuous logging to SD card
- Button press adds event marker
- Circular buffer for pre-event context

### 2. Data Format

**Option A: CSV** ⭐ RECOMMENDED
- Human-readable
- Easy to analyze in Python/Excel
- Standard format
- Pros: Simple, universal
- Cons: Larger file size

**Option B: Binary**
- Compact format
- Faster writes
- Pros: Efficient, fast
- Cons: Needs decoder, not human-readable

**Recommendation:** CSV for development, binary for production

### 3. File Management

**Strategy:**
- One file per logging session
- Filename: `YYYYMMDD_HHMMSS.csv`
- Header row with column names
- Automatic file rotation at 100MB
- Keep last 10 files, delete oldest

---

## Resource Requirements

### Hardware Needed
- ✅ ESP32-S3 DevKit (have)
- ✅ VL53L8CX ToF sensor (have)
- ✅ BNO08x IMU (have)
- ✅ RGB LED (have)
- 🛒 SD card module (need)
- 🛒 GPS module (need)
- 🛒 MicroSD card 8GB+ (need)

### Software Dependencies
- ✅ ESP-IDF v5.5.1 (installed)
- ✅ VL53L8CX library (integrated)
- ✅ BNO08x library (integrated)
- 🔄 SD card driver (ESP-IDF built-in)
- 🔄 GPS NMEA parser (to implement)
- 🔄 TensorFlow Lite Micro (to integrate)

### Development Time Estimates
- SD card logging: 4 hours
- GNSS integration: 6 hours
- Data processor: 8 hours
- TFLite integration: 16 hours
- Testing and validation: 8 hours
- **Total:** ~42 hours (~1 week full-time)

---

## Success Criteria

### Phase 1: Data Collection (Current) ✅
- [x] Sensors reading reliably
- [x] Real-time display working
- [x] System stable for extended operation
- [ ] SD card logging functional
- [ ] GPS data integrated

### Phase 2: Data Processing
- [ ] Sensor synchronization working
- [ ] Feature extraction implemented
- [ ] Data quality validated

### Phase 3: ML Integration
- [ ] TFLite model loaded
- [ ] Inference running in real-time
- [ ] Near-miss detection working
- [ ] Performance acceptable (< 100ms latency)

### Phase 4: Field Testing
- [ ] Dataset collected (10+ hours)
- [ ] Model trained and validated
- [ ] System deployed and tested
- [ ] Thesis results documented

---

## Questions to Answer

### For SD Card Implementation:
1. Which SPI bus to use? (SPI2 recommended, SPI3 used by IMU)
2. What SD card speed class? (Class 10 minimum)
3. File rotation strategy? (Size-based, 100MB per file)
4. Error handling? (Retry, fallback to RAM buffer)

### For GNSS Integration:
1. Which GPS module? (NEO-6M or NEO-M8N)
2. Update rate? (1Hz sufficient, 5Hz better)
3. Fix timeout? (30 seconds acceptable)
4. Indoor testing? (Use GPS simulator or outdoor)

### For ML Integration:
1. Model size? (< 1MB for flash storage)
2. Input features? (16 ToF + 6 IMU = 22 features)
3. Inference rate? (10Hz sufficient)
4. PSRAM usage? (Reserve 4MB for model + buffers)

---

## Conclusion

**Current Status:** ✅ Sensor integration complete and stable

**Next Step:** 🎯 Implement SD card logging (Priority 1)

**Timeline:** SD card implementation can be done in 1 day

**Blocker:** Need SD card module hardware

**Recommendation:** 
1. Order SD card module immediately
2. Meanwhile, prepare SD logger component code
3. Test with SD card simulator if needed
4. Integrate when hardware arrives

**Ready to proceed with:** SD card logging implementation

---

## Action Items

### Immediate (Today)
- [x] Fix .gitignore for build artifacts
- [x] Fix UART overflow issue
- [x] Document current status
- [ ] Review SD card module options
- [ ] Order hardware if needed

### This Week
- [ ] Implement SD logger component
- [ ] Test SD card write performance
- [ ] Integrate GPS module
- [ ] Collect first dataset

### Next Week
- [ ] Implement data processor
- [ ] Feature extraction
- [ ] Begin TFLite integration

---

**Document Version:** 1.0  
**Last Updated:** December 9, 2024  
**Status:** Ready for SD Card Implementation 🎯
