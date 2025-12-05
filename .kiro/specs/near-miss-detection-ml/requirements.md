# Requirements Document

## Introduction

This specification defines the requirements for extending the ProxiTof multi-sensor proximity detection system to include data logging capabilities, manual near-miss event flagging, and machine learning-based automated near-miss detection. The system will collect synchronized sensor data (ToF distance grid, IMU orientation and acceleration) to an SD card, allow users to manually flag near-miss events during data collection, and eventually deploy a trained TensorFlow Lite model on the ESP32-S3 for real-time near-miss prediction using ESP-NN acceleration.

A near-miss event is characterized by rapid distance closure detected by the ToF sensor combined with sudden acceleration or jerk patterns from the IMU, indicating potential collision scenarios.

## Glossary

- **ProxiTof_System**: The ESP32-S3 based multi-sensor proximity detection system combining VL53L8CX ToF sensor and BNO08x IMU
- **ToF_Sensor**: VL53L8CX Time-of-Flight sensor providing 4x4 distance grid at 60Hz
- **IMU_Sensor**: BNO08x 9-DOF Inertial Measurement Unit providing orientation, linear acceleration, and angular velocity at 100Hz
- **SD_Card_Module**: SD card adapter connected via SPI for data storage, formatted as FAT32
- **Flag_Button**: Physical button for manual near-miss event annotation during data collection
- **Near_Miss_Event**: A situation characterized by rapid distance closure (>500mm/s approach rate) combined with sudden acceleration changes (>2 m/s² jerk), indicating collision risk
- **Data_Log**: Time-series sensor data stored on SD card in CSV format with timestamps and event flags, organized by date
- **ML_Model**: TensorFlow Lite machine learning model trained to detect near-miss patterns from sensor data
- **ESP_NN**: Espressif's neural network acceleration library for optimized inference on ESP32 series
- **TFLite_Micro**: TensorFlow Lite for Microcontrollers runtime for embedded ML inference
- **GNSS_Module**: Neo-M9N GPS module for optional location tracking (future enhancement)
- **Status_LED**: Built-in WS2812 RGB LED for system status and near-miss indication
- **Sensor_Sync**: Timestamp synchronization mechanism ensuring ToF and IMU data alignment within 10ms
- **Jerk**: Rate of change of acceleration, measured in m/s³, indicating sudden motion changes

## Requirements

### Requirement 1

**User Story:** As a researcher, I want to log synchronized multi-sensor data to an SD card, so that I can collect training data for machine learning model development.

#### Acceptance Criteria

1. WHEN the ProxiTof_System starts THEN the SD_Card_Module SHALL initialize with FAT32 filesystem and verify card presence
2. WHEN the SD_Card_Module initialization succeeds THEN the ProxiTof_System SHALL create a date-based directory structure (YYYY-MM-DD format)
3. WHEN creating a new Data_Log THEN the ProxiTof_System SHALL generate filename with timestamp format "log_YYYYMMDD_HHMMSS.csv"
4. WHEN ToF_Sensor data is available THEN the ProxiTof_System SHALL write the 4x4 distance grid to the Data_Log with millisecond timestamp
5. WHEN IMU_Sensor data is available THEN the ProxiTof_System SHALL write orientation (roll, pitch, yaw), linear acceleration (x, y, z), and angular velocity (x, y, z) to the Data_Log with millisecond timestamp
6. WHEN writing to SD_Card_Module THEN the ProxiTof_System SHALL buffer data to minimize write latency and maintain 60Hz operation
7. WHEN the Data_Log file size exceeds 100MB THEN the ProxiTof_System SHALL close the current file and create a new log file in the same date directory
8. WHEN SD_Card_Module write fails THEN the ProxiTof_System SHALL indicate error via Status_LED red blink pattern and log error code to serial output

### Requirement 2

**User Story:** As a researcher, I want sensor data synchronized with consistent timestamps, so that I can accurately correlate ToF and IMU measurements during analysis.

#### Acceptance Criteria

1. WHEN the ProxiTof_System boots THEN the Sensor_Sync mechanism SHALL establish a common millisecond timebase
2. WHEN ToF_Sensor produces data at 60Hz THEN the ProxiTof_System SHALL timestamp each frame with milliseconds since boot
3. WHEN IMU_Sensor produces data at 100Hz THEN the ProxiTof_System SHALL timestamp each sample with milliseconds since boot
4. WHEN writing Data_Log entries THEN the ProxiTof_System SHALL ensure ToF and IMU data within 10ms are written to the same row
5. WHEN IMU_Sensor data arrives between ToF frames THEN the ProxiTof_System SHALL interpolate or use most recent IMU values for synchronization
6. WHEN timestamp drift is detected THEN the ProxiTof_System SHALL log a synchronization warning with drift magnitude

### Requirement 3

**User Story:** As a researcher, I want to manually flag near-miss events during data collection, so that I can create labeled training data for supervised learning.

#### Acceptance Criteria

1. WHEN the Flag_Button is pressed THEN the ProxiTof_System SHALL record the button press timestamp with millisecond precision
2. WHEN a Flag_Button press is detected THEN the ProxiTof_System SHALL write a near-miss flag marker to the Data_Log
3. WHEN a Flag_Button press occurs THEN the ProxiTof_System SHALL provide immediate visual feedback via Status_LED yellow flash for 500ms
4. WHEN the Flag_Button is held for more than 50 milliseconds THEN the ProxiTof_System SHALL debounce and register only one flag event
5. WHEN a near-miss flag is recorded THEN the ProxiTof_System SHALL mark a 2-second time window (1 second before and 1 second after button press) in the flag column

### Requirement 4

**User Story:** As a data scientist, I want sensor data logged in a structured CSV format optimized for TensorFlow training, so that I can easily parse and process the data for model development.

#### Acceptance Criteria

1. WHEN creating a Data_Log file THEN the ProxiTof_System SHALL write a CSV header row with column names: timestamp_ms, tof_00 through tof_33 (16 zones), roll, pitch, yaw, accel_x, accel_y, accel_z, gyro_x, gyro_y, gyro_z, flag_manual, flag_auto, confidence
2. WHEN logging sensor data THEN the ProxiTof_System SHALL format each row as comma-separated values with consistent field ordering
3. WHEN writing timestamps THEN the ProxiTof_System SHALL use milliseconds since boot as the time reference
4. WHEN writing ToF_Sensor data THEN the ProxiTof_System SHALL include all 16 distance values in millimeters as separate columns (tof_00 to tof_33 in row-major order)
5. WHEN writing IMU_Sensor data THEN the ProxiTof_System SHALL include roll, pitch, yaw in degrees, linear acceleration x, y, z in m/s², and angular velocity x, y, z in deg/s with 2 decimal precision
6. WHEN writing near-miss flags THEN the ProxiTof_System SHALL include flag_manual (0 or 1) and flag_auto (0 or 1) columns
7. WHEN ML_Model is active THEN the ProxiTof_System SHALL include confidence score (0.00 to 1.00) in the confidence column
8. WHEN sensor data is unavailable THEN the ProxiTof_System SHALL write the last known valid value for that sensor

### Requirement 5

**User Story:** As a researcher, I want the system to maintain real-time sensor operation while logging data, so that data collection does not degrade system responsiveness.

#### Acceptance Criteria

1. WHEN SD_Card_Module write operations occur THEN the ToF_Sensor SHALL continue operating at 60Hz without frame drops
2. WHEN buffering sensor data THEN the ProxiTof_System SHALL allocate a circular buffer to hold at least 2 seconds of data (120 ToF frames)
3. WHEN the write buffer reaches 80% capacity THEN the ProxiTof_System SHALL flush data to SD_Card_Module asynchronously
4. WHEN SD_Card_Module write latency exceeds 100ms THEN the ProxiTof_System SHALL log a warning to serial output with latency value
5. WHILE logging is active THEN the ProxiTof_System SHALL maintain total system latency below 20ms for sensor-to-LED response
6. WHEN buffer overflow is imminent THEN the ProxiTof_System SHALL prioritize flagged events and drop non-flagged frames if necessary

### Requirement 6

**User Story:** As a researcher, I want to collect sufficient training data volume, so that the machine learning model can learn robust near-miss patterns.

#### Acceptance Criteria

1. WHEN data collection begins THEN the ProxiTof_System SHALL support continuous logging for at least 2 hours per session
2. WHEN estimating data volume THEN the ProxiTof_System SHALL generate approximately 216,000 samples per hour (60 samples/sec × 3600 sec)
3. WHEN collecting training data THEN the researcher SHALL aim for minimum 10 hours of diverse scenarios (walking, running, obstacles, vehicles)
4. WHEN flagging near-miss events THEN the researcher SHALL target minimum 200 manually flagged events for initial model training
5. WHEN organizing Data_Log files THEN the ProxiTof_System SHALL maintain date-based directory structure for easy dataset management

### Requirement 7

**User Story:** As a researcher, I want the system to detect near-miss characteristics in real-time, so that I can validate sensor fusion effectiveness before ML deployment.

#### Acceptance Criteria

1. WHEN ToF_Sensor detects distance change THEN the ProxiTof_System SHALL calculate approach rate in mm/s using consecutive frame differences
2. WHEN approach rate exceeds 500 mm/s THEN the ProxiTof_System SHALL flag potential near-miss condition
3. WHEN IMU_Sensor detects acceleration change THEN the ProxiTof_System SHALL calculate jerk magnitude in m/s³
4. WHEN jerk magnitude exceeds 2 m/s³ THEN the ProxiTof_System SHALL flag potential near-miss condition
5. WHEN both rapid approach (>500 mm/s) and high jerk (>2 m/s³) occur within 500ms THEN the ProxiTof_System SHALL trigger Status_LED orange alert
6. WHEN minimum distance falls below 300mm with approach rate >200 mm/s THEN the ProxiTof_System SHALL trigger Status_LED red alert

### Requirement 8

**User Story:** As a researcher, I want to deploy a trained TensorFlow Lite model on the ESP32-S3, so that the system can perform real-time near-miss detection without external processing.

#### Acceptance Criteria

1. WHEN the ProxiTof_System boots with ML_Model enabled THEN the ProxiTof_System SHALL load the TFLite_Micro model from flash memory
2. WHEN ML_Model is loaded THEN the ProxiTof_System SHALL verify model integrity and allocate tensor arena memory (minimum 50KB)
3. WHEN sensor data is available THEN the ProxiTof_System SHALL prepare input tensor with normalized sensor values (16 ToF + 9 IMU = 25 features)
4. WHEN ML_Model inference is triggered THEN the ProxiTof_System SHALL execute prediction using ESP_NN acceleration if available
5. WHEN ML_Model inference completes THEN the ProxiTof_System SHALL extract confidence score for near-miss prediction within 50ms
6. WHEN near-miss confidence exceeds threshold (default 0.7) THEN the ProxiTof_System SHALL trigger Status_LED orange alert and log the event
7. WHEN ML_Model inference latency exceeds 50ms THEN the ProxiTof_System SHALL log a performance warning with actual latency value

### Requirement 9

**User Story:** As a researcher, I want the system to automatically log ML-detected near-miss events, so that I can evaluate model performance and collect additional training data.

#### Acceptance Criteria

1. WHEN ML_Model detects a near-miss event THEN the ProxiTof_System SHALL write an auto-flag marker (flag_auto=1) to the Data_Log
2. WHEN writing auto-flag markers THEN the ProxiTof_System SHALL include the ML_Model confidence score in the confidence column
3. WHEN both manual Flag_Button and ML_Model detect near-miss simultaneously THEN the ProxiTof_System SHALL record both flag types (flag_manual=1, flag_auto=1)
4. WHEN ML_Model is active THEN the ProxiTof_System SHALL run inference every frame (60Hz) to minimize detection latency
5. WHEN ML_Model produces false positives THEN the researcher SHALL be able to review confidence scores to adjust threshold

### Requirement 10

**User Story:** As a researcher, I want clear visual feedback on system status, so that I can monitor data collection and model operation without serial connection.

#### Acceptance Criteria

1. WHEN the ProxiTof_System is initializing THEN the Status_LED SHALL display blue color
2. WHEN SD_Card_Module initialization fails THEN the Status_LED SHALL display red blinking pattern at 2Hz
3. WHEN data logging is active and healthy THEN the Status_LED SHALL display green breathing pattern (1-second cycle)
4. WHEN Flag_Button is pressed THEN the Status_LED SHALL flash yellow for 500ms
5. WHEN ML_Model detects near-miss THEN the Status_LED SHALL display orange alert for 1 second
6. WHEN proximity is critically close (<300mm) with rapid approach THEN the Status_LED SHALL override other states and display solid red
7. WHEN buffer is near full (>90%) THEN the Status_LED SHALL add purple tint to current color as warning

### Requirement 11

**User Story:** As a researcher, I want comprehensive error handling and diagnostics, so that I can quickly identify and resolve issues during field data collection.

#### Acceptance Criteria

1. WHEN any sensor initialization fails THEN the ProxiTof_System SHALL output detailed error message to serial with sensor name and error code
2. WHEN SD_Card_Module is not detected THEN the ProxiTof_System SHALL display error code on Status_LED (red blink count) and continue sensor operation
3. WHEN file write operations fail THEN the ProxiTof_System SHALL log error type (write timeout, disk full, file system error) to serial
4. WHEN buffer overflow occurs THEN the ProxiTof_System SHALL log number of dropped frames to serial and Data_Log
5. WHEN ML_Model fails to load THEN the ProxiTof_System SHALL log error details and continue operation in data collection mode only
6. WHEN system detects anomalies THEN the ProxiTof_System SHALL maintain error counter accessible via serial command interface

### Requirement 12

**User Story:** As a researcher, I want to configure system parameters without recompiling firmware, so that I can adjust data collection settings in the field.

#### Acceptance Criteria

1. WHEN the ProxiTof_System boots THEN the ProxiTof_System SHALL read configuration from a config.txt file on SD_Card_Module if present
2. WHEN config.txt contains valid parameters THEN the ProxiTof_System SHALL apply settings for: log_interval_ms, flag_window_ms, ml_threshold, approach_threshold_mm_s, jerk_threshold_m_s3
3. WHEN config.txt is missing or invalid THEN the ProxiTof_System SHALL use default values and log warning to serial
4. WHEN configuration is loaded THEN the ProxiTof_System SHALL output active settings to serial at startup
5. WHEN user sends serial command "config" THEN the ProxiTof_System SHALL display current configuration values

### Requirement 13

**User Story:** As a researcher, I want to add optional GNSS location data to logs, so that I can correlate near-miss events with geographic locations for spatial analysis.

#### Acceptance Criteria

1. WHEN GNSS_Module is connected and detected THEN the ProxiTof_System SHALL initialize Neo-M9N module via UART
2. WHEN GNSS_Module has valid fix THEN the ProxiTof_System SHALL append latitude, longitude, altitude, and GPS timestamp to Data_Log rows
3. WHEN GNSS_Module has no fix THEN the ProxiTof_System SHALL write null values for GPS fields and continue logging sensor data
4. WHEN GNSS_Module is not detected THEN the ProxiTof_System SHALL omit GPS columns from CSV header and operate normally
5. WHEN GNSS data is available THEN the ProxiTof_System SHALL update location at 1Hz to minimize processing overhead

### Requirement 14

**User Story:** As a researcher, I want the system to compute derived features from raw sensor data, so that the ML model can learn from meaningful patterns rather than raw values.

#### Acceptance Criteria

1. WHEN ToF_Sensor data is received THEN the ProxiTof_System SHALL calculate minimum distance across all 16 zones
2. WHEN consecutive ToF frames are available THEN the ProxiTof_System SHALL compute approach rate (distance delta / time delta)
3. WHEN IMU_Sensor acceleration data is received THEN the ProxiTof_System SHALL calculate jerk magnitude from consecutive acceleration samples
4. WHEN IMU_Sensor orientation changes THEN the ProxiTof_System SHALL compute angular velocity from consecutive orientation samples
5. WHEN logging derived features THEN the ProxiTof_System SHALL append columns: min_distance_mm, approach_rate_mm_s, jerk_magnitude_m_s3, angular_velocity_deg_s to Data_Log
6. WHEN ML_Model is active THEN the ProxiTof_System SHALL include derived features in the input tensor alongside raw sensor values

### Requirement 15

**User Story:** As a developer, I want the system built on Arduino framework with PlatformIO, so that I can leverage Arduino libraries and have a streamlined development workflow.

#### Acceptance Criteria

1. WHEN developing the ProxiTof_System THEN the system SHALL use Arduino framework for ESP32-S3
2. WHEN using Arduino libraries THEN the ProxiTof_System SHALL utilize Arduino-compatible libraries for VL53L8CX, BNO08x, SD card, and WS2812 LED
3. WHEN building firmware THEN the ProxiTof_System SHALL use PlatformIO as the primary build system with platformio.ini configuration
4. WHEN configuring PlatformIO THEN the platformio.ini SHALL specify esp32-s3-devkitc-1 board and Arduino framework
5. WHEN integrating TFLite_Micro THEN the ProxiTof_System SHALL use Arduino-compatible TensorFlow Lite library
6. WHEN compiling THEN the build process SHALL support both PlatformIO CLI and PlatformIO IDE
7. WHEN uploading firmware THEN the system SHALL support serial upload via USB connection

### Requirement 16

**User Story:** As a developer, I want a properly organized Git repository with clear structure, so that the project is maintainable and can be easily shared with collaborators.

#### Acceptance Criteria

1. WHEN initializing the repository THEN the Git_Repository SHALL be named "ProxiTof_Arduino_ESP32"
2. WHEN organizing project structure THEN the Git_Repository SHALL follow Arduino/PlatformIO conventions with src/, lib/, include/, and data/ directories
3. WHEN creating .gitignore THEN the Git_Repository SHALL exclude build artifacts (.pio/, .vscode/, *.bin), IDE files, and OS-specific files
4. WHEN documenting the project THEN the Git_Repository SHALL include README.md with hardware setup, pin configuration, build instructions, and usage guide
5. WHEN managing dependencies THEN the Git_Repository SHALL specify all library dependencies in platformio.ini with version constraints
6. WHEN organizing code THEN the Git_Repository SHALL separate concerns into modules: sensors/, logging/, ml_inference/, and config/
7. WHEN committing changes THEN the Git_Repository SHALL use descriptive commit messages following conventional commits format
8. WHEN creating branches THEN the Git_Repository SHALL maintain main branch for stable releases and develop branch for active development
9. WHEN including documentation THEN the Git_Repository SHALL provide docs/ directory with wiring diagrams, data format specifications, and ML model training guide

### Requirement 17

**User Story:** As a developer, I want clear build and flash instructions for Arduino framework, so that I can quickly set up the development environment and deploy firmware.

#### Acceptance Criteria

1. WHEN setting up development environment THEN the documentation SHALL provide step-by-step PlatformIO installation instructions
2. WHEN installing dependencies THEN the ProxiTof_System SHALL automatically download required libraries via PlatformIO Library Manager
3. WHEN building firmware THEN the developer SHALL execute "pio run" command to compile the project
4. WHEN flashing firmware THEN the developer SHALL execute "pio run -t upload" to flash ESP32-S3 via USB
5. WHEN monitoring serial output THEN the developer SHALL execute "pio device monitor" to view real-time logs at 115200 baud
6. WHEN using Arduino IDE THEN the documentation SHALL provide alternative instructions for Arduino IDE setup and compilation
