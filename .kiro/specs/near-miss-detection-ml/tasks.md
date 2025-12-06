# Implementation Plan

- [x] 1. Set up project structure and Git repository





  - Create PlatformIO project with ESP32-S3 configuration
  - Initialize Git repository with proper .gitignore
  - Create directory structure: src/, lib/, include/, data/, docs/
  - Configure platformio.ini with board settings and library dependencies
  - Create README.md with hardware setup and build instructions
  - _Requirements: 16.1, 16.2, 16.3, 16.5, 16.9, 17.1_

- [ ] 2. Implement core data structures and configuration
  - [x] 2.1 Create data model header files




    - Define ToFData, IMUData, SyncedData, FeatureVector, LogEntry structs
    - Define SystemConfig struct with default values
    - Define SystemState enum and LEDState enum
    - _Requirements: 15.1, 15.2_

  - [ ] 2.2 Implement configuration file parser





    - Write config.txt parser for key=value format
    - Implement parameter validation and range checking
    - Implement default value fallback for missing/invalid config
    - _Requirements: 12.1, 12.2, 12.3_

  - [ ]* 2.3 Write property test for configuration parsing
    - **Property 50: Configuration parameter application**
    - **Property 51: Configuration fallback**
    - **Validates: Requirements 12.2, 12.3**

- [ ] 3. Implement Sensor Manager component
  - [ ] 3.1 Create SensorManager class with initialization
    - Initialize VL53L8CX ToF sensor on I2C (GPIO 8, 9)
    - Initialize BNO08x IMU sensor on SPI (GPIO 10, 11, 12, 13)
    - Implement sensor health checks and error reporting
    - Implement timestamp generation using millis()
    - _Requirements: 1.1, 15.1, 15.2_

  - [ ] 3.2 Implement ToF sensor data reading
    - Read 4x4 distance grid at 60Hz
    - Package data into ToFData struct with timestamp
    - Handle sensor timeouts and invalid data
    - _Requirements: 1.4, 2.2_

  - [ ] 3.3 Implement IMU sensor data reading
    - Read orientation (roll, pitch, yaw) at 100Hz
    - Read linear acceleration (x, y, z) at 100Hz
    - Read angular velocity (gyro x, y, z) at 100Hz
    - Package data into IMUData struct with timestamp
    - _Requirements: 1.5, 2.3_

  - [ ] 3.4 Implement button input with debouncing
    - Configure button on GPIO 3 with internal pull-up
    - Implement 50ms debounce logic
    - Return button press timestamp
    - _Requirements: 3.1, 3.4_

  - [ ]* 3.5 Write property tests for sensor manager
    - **Property 8: Timestamp monotonicity**
    - **Property 12: Button press recording**
    - **Property 15: Button debouncing**
    - **Validates: Requirements 2.2, 2.3, 3.1, 3.4**

- [ ] 4. Implement Data Processor component
  - [ ] 4.1 Create DataProcessor class with synchronization
    - Implement sensor synchronization within 10ms window
    - Implement IMU interpolation for ToF frame alignment
    - Detect and log timestamp drift
    - _Requirements: 2.1, 2.4, 2.5, 2.6_

  - [ ] 4.2 Implement feature extraction functions
    - Calculate minimum distance across ToF 4x4 grid
    - Calculate approach rate from consecutive ToF frames
    - Calculate jerk magnitude from consecutive IMU acceleration
    - Calculate angular velocity magnitude from gyroscope data
    - _Requirements: 14.1, 14.2, 14.3, 14.4_

  - [ ] 4.3 Implement rule-based near-miss detection
    - Detect rapid approach (>500 mm/s)
    - Detect high jerk (>2 m/s³)
    - Detect combined conditions within 500ms window
    - Detect critical proximity (<300mm with >200 mm/s approach)
    - _Requirements: 7.1, 7.2, 7.3, 7.4, 7.5, 7.6_

  - [ ]* 4.4 Write property tests for data processor
    - **Property 9: Sensor synchronization within window**
    - **Property 10: IMU interpolation correctness**
    - **Property 27: Approach rate calculation correctness**
    - **Property 29: Jerk magnitude calculation correctness**
    - **Property 55: Minimum distance calculation**
    - **Property 56: Approach rate derivation**
    - **Property 57: Jerk magnitude derivation**
    - **Property 58: Angular velocity derivation**
    - **Validates: Requirements 2.4, 2.5, 7.1, 7.3, 14.1, 14.2, 14.3, 14.4**

- [ ] 5. Implement Storage Manager component
  - [ ] 5.1 Create CircularBuffer template class
    - Implement push, pop, size, capacity, isFull methods
    - Implement fillPercentage calculation
    - Allocate buffer for 120 frames (2 seconds at 60Hz)
    - _Requirements: 5.2_

  - [ ] 5.2 Create StorageManager class with SD card initialization
    - Initialize SD card with FAT32 filesystem
    - Verify card presence and report errors
    - Create date-based directory structure (YYYY-MM-DD)
    - _Requirements: 1.1, 1.2_

  - [ ] 5.3 Implement log file creation and management
    - Generate log filename with timestamp (log_YYYYMMDD_HHMMSS.csv)
    - Write CSV header row with all column names
    - Implement file rotation at 100MB threshold
    - _Requirements: 1.3, 4.1, 1.7_

  - [ ] 5.4 Implement buffered data writing
    - Buffer sensor data in circular buffer
    - Flush buffer to SD card asynchronously at 80% capacity
    - Handle write errors with LED indication and serial logging
    - Prioritize flagged events during buffer overflow
    - _Requirements: 1.6, 5.3, 5.6, 1.8_

  - [ ] 5.5 Implement CSV formatting
    - Format LogEntry as CSV row with correct field order
    - Include all 16 ToF zones, 9 IMU values, derived features, flags
    - Handle missing sensor data with last known values
    - _Requirements: 4.2, 4.3, 4.4, 4.5, 4.6, 4.7, 4.8_

  - [ ]* 5.6 Write property tests for storage manager
    - **Property 2: Date directory format correctness**
    - **Property 3: Log filename format correctness**
    - **Property 17: CSV header presence**
    - **Property 18: CSV row format consistency**
    - **Property 23: Circular buffer capacity**
    - **Property 24: Buffer flush trigger**
    - **Validates: Requirements 1.2, 1.3, 4.1, 4.2, 5.2, 5.3**

- [ ] 6. Implement Status LED component
  - [ ] 6.1 Create StatusLED class with WS2812 control
    - Initialize WS2812 LED on GPIO 48
    - Implement color and pattern generation for all states
    - Implement priority-based state override system
    - _Requirements: 10.1, 10.2, 10.3, 10.4, 10.5, 10.6, 10.7_

  - [ ] 6.2 Implement LED animations
    - Breathing pattern for logging active (1-second cycle)
    - Blinking pattern for SD error (2Hz)
    - Flash pattern for button press (500ms yellow)
    - Alert patterns for near-miss detection
    - _Requirements: 10.3, 10.4, 10.5_

  - [ ]* 6.3 Write property tests for status LED
    - **Property 43: LED state correctness**
    - **Property 14: Button press LED feedback**
    - **Validates: Requirements 10.1-10.7, 3.3**

- [ ] 7. Implement ML Inference component
  - [ ] 7.1 Create MLInference class with TFLite initialization
    - Load TensorFlow Lite model from flash memory
    - Allocate 50KB tensor arena
    - Verify model integrity
    - Register ESP-NN optimized operations
    - _Requirements: 8.1, 8.2, 8.4_

  - [ ] 7.2 Implement input tensor preparation
    - Normalize 16 ToF distances to [0, 1] range
    - Normalize orientation (roll, pitch, yaw) to [-1, 1] range
    - Normalize acceleration to [-1, 1] range
    - Normalize gyroscope to [-1, 1] range
    - Prepare 25-feature input tensor
    - _Requirements: 8.3_

  - [ ] 7.3 Implement inference execution
    - Execute TFLite inference with ESP-NN acceleration
    - Extract confidence score from output tensor
    - Measure and log inference latency
    - Trigger alerts when confidence exceeds threshold
    - _Requirements: 8.4, 8.5, 8.6, 8.7_

  - [ ]* 7.4 Write property tests for ML inference
    - **Property 33: ML model integrity verification**
    - **Property 34: Input tensor normalization**
    - **Property 35: ML inference completion**
    - **Property 37: ML confidence threshold alerting**
    - **Validates: Requirements 8.2, 8.3, 8.4, 8.6**

- [ ] 8. Implement main application loop
  - [ ] 8.1 Create main setup function
    - Initialize serial communication at 115200 baud
    - Initialize all components (sensors, storage, LED, ML)
    - Load configuration from SD card
    - Output system status and configuration to serial
    - Handle initialization errors with graceful degradation
    - _Requirements: 11.1, 11.2, 11.5, 12.4_

  - [ ] 8.2 Create main loop with 60Hz timing
    - Poll ToF sensor at 60Hz (primary timing source)
    - Poll IMU sensor asynchronously
    - Check button state with debouncing
    - Synchronize sensor data
    - Extract features and detect near-miss conditions
    - Buffer data for SD writing
    - Update LED status
    - Run ML inference if enabled
    - _Requirements: 1.4, 1.5, 5.1, 9.4_

  - [ ] 8.3 Implement event handling
    - Handle button press events with flag marking
    - Handle ML detection events with auto-flag marking
    - Handle dual flag events (manual + ML)
    - Apply 2-second flag window around events
    - _Requirements: 3.2, 3.5, 9.1, 9.2, 9.3_

  - [ ] 8.4 Implement error handling and recovery
    - Handle sensor errors with retry logic
    - Handle SD errors with graceful degradation
    - Handle ML errors with fallback to data collection mode
    - Maintain error counters
    - Log all errors to serial with detailed information
    - _Requirements: 11.1, 11.2, 11.3, 11.4, 11.5, 11.6_

  - [ ]* 8.5 Write property tests for main loop
    - **Property 4: Sensor data logging completeness**
    - **Property 5: Frame rate maintenance during buffering**
    - **Property 13: Button press flag marker**
    - **Property 16: Flag time window marking**
    - **Property 39: ML auto-flag logging**
    - **Property 41: Dual flag recording**
    - **Property 42: ML inference frequency**
    - **Validates: Requirements 1.4, 1.5, 1.6, 3.2, 3.5, 9.1, 9.3, 9.4**

- [ ] 9. Implement optional GNSS integration
  - [ ] 9.1 Create GNSS module interface
    - Initialize Neo-M9N module via UART
    - Parse NMEA sentences for GPS data
    - Extract latitude, longitude, altitude, GPS timestamp
    - _Requirements: 13.1_

  - [ ] 9.2 Integrate GNSS data into logging
    - Append GPS columns to CSV header if GNSS detected
    - Update GPS data at 1Hz
    - Handle no-fix condition with null values
    - Handle GNSS not detected gracefully
    - _Requirements: 13.2, 13.3, 13.4, 13.5_

  - [ ]* 9.3 Write property tests for GNSS integration
    - **Property 53: GNSS data integration**
    - **Property 54: GNSS failure handling**
    - **Validates: Requirements 13.2, 13.3, 13.4**

- [ ] 10. Implement serial command interface
  - [ ] 10.1 Create command parser
    - Parse serial input for commands
    - Implement "config" command to display current settings
    - Implement "status" command to display system state
    - Implement "errors" command to display error counters
    - _Requirements: 11.6, 12.5_

  - [ ] 10.2 Implement diagnostic output
    - Output performance metrics every second
    - Output frame rate, latency, buffer utilization
    - Output error counts and warnings
    - _Requirements: 5.4, 8.7_

- [ ] 11. Create documentation and examples
  - [ ] 11.1 Write comprehensive README.md
    - Document hardware setup and pin connections
    - Document build and flash instructions for PlatformIO
    - Document serial commands and configuration options
    - Include troubleshooting guide
    - _Requirements: 16.4, 17.2, 17.3, 17.4, 17.5_

  - [ ] 11.2 Create wiring diagrams
    - Create Fritzing or similar diagram for all connections
    - Document ToF, IMU, SD card, button, LED wiring
    - Include optional GNSS wiring
    - _Requirements: 16.9_

  - [ ] 11.3 Document data format and ML training guide
    - Document CSV format with example rows
    - Provide Python script for data preprocessing
    - Provide TensorFlow training example
    - Document model conversion to TFLite
    - Document model flashing to ESP32-S3
    - _Requirements: 16.9_

  - [ ] 11.4 Create example config.txt
    - Provide example configuration file with comments
    - Document all configurable parameters
    - Include recommended values for different scenarios
    - _Requirements: 12.1, 12.2_

- [ ] 12. Final integration and testing
  - [ ] 12.1 Run all property-based tests
    - Execute all property tests with 100+ iterations
    - Verify all 60 correctness properties pass
    - Fix any failing tests
    - _Requirements: All_

  - [ ]* 12.2 Perform integration testing
    - Test end-to-end data collection for 10+ minutes
    - Test button flagging during data collection
    - Test ML inference with sample model
    - Test error recovery scenarios
    - Verify CSV files are correctly formatted
    - _Requirements: All_

  - [ ]* 12.3 Perform performance testing
    - Measure and verify 60Hz frame rate maintenance
    - Measure and verify <20ms sensor-to-LED latency
    - Measure and verify <50ms ML inference latency
    - Measure and verify <100ms SD write latency
    - Verify RAM usage <200KB
    - _Requirements: 5.1, 5.4, 5.5, 8.5, 8.7_

  - [ ] 12.4 Create initial Git commit and push to GitHub
    - Commit all source code with descriptive message
    - Create develop branch for active development
    - Push to GitHub repository "ProxiTof_Arduino_ESP32"
    - _Requirements: 16.7, 16.8_

- [ ] 13. Checkpoint - Ensure all tests pass
  - Ensure all tests pass, ask the user if questions arise.
