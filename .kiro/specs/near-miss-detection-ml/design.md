# Design Document

## Overview

The ProxiTof Near-Miss Detection System is an ESP-IDF-based embedded application for ESP32-S3 that performs real-time sensor fusion, data logging, and machine learning inference. The system combines a VL53L8CX Time-of-Flight sensor (4x4 distance grid at 60Hz) with a BNO08x 9-DOF IMU (orientation and motion at 100Hz) to detect and predict near-miss collision events.

The design follows a modular architecture with clear separation between sensor acquisition, data processing, storage, and ML inference. The system operates in two primary modes:

1. **Data Collection Mode**: Logs synchronized sensor data to SD card with manual event flagging via button
2. **ML Inference Mode**: Runs trained TensorFlow Lite model for automated near-miss detection

Key design principles:
- Non-blocking sensor operations maintaining 60Hz ToF timing using FreeRTOS tasks
- Circular buffering for reliable SD card writes without frame drops
- Modular ESP-IDF component architecture enabling independent testing and future enhancements
- ESP-IDF framework for native hardware access, ESP-NN ML acceleration, and production-grade reliability

## Architecture

### System Architecture Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                        ESP32-S3 Main Loop                        │
│                         (60Hz ToF-driven)                        │
└────────────┬────────────────────────────────────────────────────┘
             │
             ├──────────────┬──────────────┬──────────────┬────────────────┐
             │              │              │              │                │
             ▼              ▼              ▼              ▼                ▼
      ┌──────────┐   ┌──────────┐  ┌──────────┐  ┌──────────┐    ┌──────────┐
      │  Sensor  │   │  Data    │  │  Storage │  │    ML    │    │  Status  │
      │  Manager │   │ Processor│  │  Manager │  │ Inference│    │   LED    │
      └──────────┘   └──────────┘  └──────────┘  └──────────┘    └──────────┘
           │              │              │              │                │
           │              │              │              │                │
      ┌────┴────┐    ┌────┴────┐   ┌────┴────┐   ┌────┴────┐      ┌────┴────┐
      │ ToF     │    │ Feature │   │ Circular│   │ TFLite  │      │ WS2812  │
      │ VL53L8CX│    │ Extract │   │ Buffer  │   │ Model   │      │ Driver  │
      │ (I2C)   │    │         │   │         │   │         │      │         │
      └─────────┘    └─────────┘   └─────────┘   └─────────┘      └─────────┘
      ┌─────────┐    ┌─────────┐   ┌─────────┐
      │ IMU     │    │ Sync    │   │ SD Card │
      │ BNO08x  │    │ Manager │   │ FAT32   │
      │ (SPI)   │    │         │   │ (SPI)   │
      └─────────┘    └─────────┘   └─────────┘
      ┌─────────┐
      │ Button  │
      │ (GPIO)  │
      └─────────┘
```

### Data Flow

```
Sensor Data → Synchronization → Feature Extraction → Buffering → SD Write
     │                                    │
     │                                    ├──→ ML Inference → Event Detection
     │                                    │
     └────────────────────────────────────┴──→ LED Status Update
```

### Module Responsibilities

**Sensor Manager**
- Initialize and configure ToF and IMU sensors
- Poll sensors at appropriate rates (ToF: 60Hz, IMU: 100Hz)
- Handle sensor errors and provide graceful degradation
- Manage button input with debouncing

**Data Processor**
- Synchronize ToF and IMU timestamps within 10ms
- Calculate derived features (approach rate, jerk, angular velocity)
- Normalize sensor values for ML input
- Detect rule-based near-miss conditions

**Storage Manager**
- Manage SD card initialization and FAT32 filesystem
- Create date-based directory structure
- Implement circular buffer for non-blocking writes
- Handle file rotation at 100MB threshold
- Read/write configuration files

**ML Inference**
- Load TensorFlow Lite model from flash
- Prepare input tensors with 25 features
- Execute inference with ESP-NN acceleration
- Extract confidence scores and trigger alerts

**Status LED**
- Display system state via color patterns
- Provide visual feedback for events
- Implement priority-based state override

## Components and Interfaces

### 1. Sensor Manager Component

**Purpose**: Abstracts sensor hardware and provides unified data interface

**ESP-IDF Components**: sensor_manager

**Classes**:

```cpp
class SensorManager {
public:
    bool initialize();
    bool readToFData(ToFData& data);
    bool readIMUData(IMUData& data);
    bool isButtonPressed();
    uint32_t getTimestamp();
    
private:
    VL53L8CX tofSensor;
    BNO08x imuSensor;
    ButtonDebouncer button;
    uint32_t lastButtonPress;
};

struct ToFData {
    uint32_t timestamp_ms;
    uint16_t distances[16];  // 4x4 grid in row-major order
    bool valid;
};

struct IMUData {
    uint32_t timestamp_ms;
    float roll, pitch, yaw;           // degrees
    float accel_x, accel_y, accel_z;  // m/s²
    float gyro_x, gyro_y, gyro_z;     // deg/s
    bool valid;
};
```

**Dependencies**:
- RJRP44 `VL53L8CX-Library` (ESP-IDF native) for ToF sensor
- ESP-IDF SPI driver for BNO08x IMU communication
- ESP-IDF GPIO driver for button input

**Pin Configuration**:
```cpp
// ToF Sensor (I2C)
#define TOF_SDA_PIN 8
#define TOF_SCL_PIN 9
#define TOF_LPN_PIN 4

// IMU Sensor (SPI)
#define IMU_MOSI_PIN 11
#define IMU_MISO_PIN 13
#define IMU_SCK_PIN 12
#define IMU_CS_PIN 10
#define IMU_INT_PIN 5
#define IMU_RST_PIN 6

// SD Card (SPI - shared with IMU bus)
#define SD_CS_PIN 7
#define SD_MOSI_PIN 11
#define SD_MISO_PIN 13
#define SD_SCK_PIN 12

// Button
#define FLAG_BUTTON_PIN 3

// LED
#define LED_PIN 48
```

### 2. Data Processor Component

**Purpose**: Synchronizes sensors, extracts features, and prepares data for logging/ML

**ESP-IDF Components**: data_processor

**Classes**:

```cpp
class DataProcessor {
public:
    bool synchronizeData(const ToFData& tof, const IMUData& imu, SyncedData& output);
    void extractFeatures(const SyncedData& data, FeatureVector& features);
    bool detectNearMiss(const FeatureVector& features);
    
private:
    IMUData lastIMU;
    ToFData lastToF;
    uint32_t syncWindow_ms = 10;
};

struct SyncedData {
    uint32_t timestamp_ms;
    ToFData tof;
    IMUData imu;
    bool synchronized;
};

struct FeatureVector {
    // Raw sensor data (25 features)
    uint16_t tof_distances[16];
    float roll, pitch, yaw;
    float accel_x, accel_y, accel_z;
    float gyro_x, gyro_y, gyro_z;
    
    // Derived features (4 features)
    uint16_t min_distance_mm;
    float approach_rate_mm_s;
    float jerk_magnitude_m_s3;
    float angular_velocity_deg_s;
    
    // Flags
    bool flag_manual;
    bool flag_auto;
    float confidence;
};
```

**Feature Extraction Algorithms**:

```cpp
// Minimum distance across 4x4 ToF grid
min_distance = min(distances[0..15])

// Approach rate (negative = approaching)
approach_rate = (current_min_distance - previous_min_distance) / delta_time_s

// Jerk magnitude (rate of acceleration change)
jerk_x = (current_accel_x - previous_accel_x) / delta_time_s
jerk_y = (current_accel_y - previous_accel_y) / delta_time_s
jerk_z = (current_accel_z - previous_accel_z) / delta_time_s
jerk_magnitude = sqrt(jerk_x² + jerk_y² + jerk_z²)

// Angular velocity magnitude
angular_velocity = sqrt(gyro_x² + gyro_y² + gyro_z²)
```

### 3. Storage Manager Component

**Purpose**: Handles SD card operations with buffering for non-blocking writes

**ESP-IDF Components**: storage_manager

**Classes**:

```cpp
class StorageManager {
public:
    bool initialize();
    bool createLogFile();
    bool writeData(const FeatureVector& data);
    bool flush();
    bool loadConfig(Config& config);
    
private:
    CircularBuffer<FeatureVector, 120> buffer;  // 2 seconds at 60Hz
    File currentFile;
    String currentDate;
    uint32_t fileSize;
    const uint32_t MAX_FILE_SIZE = 100 * 1024 * 1024;  // 100MB
};

template<typename T, size_t N>
class CircularBuffer {
public:
    bool push(const T& item);
    bool pop(T& item);
    size_t size();
    size_t capacity();
    bool isFull();
    float fillPercentage();
};
```

**CSV Format**:
```
timestamp_ms,tof_00,tof_01,...,tof_33,roll,pitch,yaw,accel_x,accel_y,accel_z,gyro_x,gyro_y,gyro_z,min_distance_mm,approach_rate_mm_s,jerk_magnitude_m_s3,angular_velocity_deg_s,flag_manual,flag_auto,confidence
```

**Directory Structure**:
```
/
├── 2025-01-15/
│   ├── log_20250115_093045.csv
│   ├── log_20250115_113022.csv
│   └── log_20250115_143511.csv
├── 2025-01-16/
│   └── log_20250116_084512.csv
└── config.txt
```

**Configuration File Format** (config.txt):
```
log_interval_ms=16
flag_window_ms=2000
ml_threshold=0.70
approach_threshold_mm_s=500
jerk_threshold_m_s3=2.0
```

### 4. ML Inference Component

**Purpose**: Runs TensorFlow Lite model for near-miss prediction

**ESP-IDF Components**: ml_inference

**Classes**:

```cpp
class MLInference {
public:
    bool initialize(const uint8_t* model_data, size_t model_size);
    float predict(const FeatureVector& features);
    bool isModelLoaded();
    
private:
    tflite::MicroInterpreter* interpreter;
    tflite::MicroMutableOpResolver<10> resolver;
    uint8_t tensor_arena[50 * 1024];  // 50KB
    TfLiteTensor* input;
    TfLiteTensor* output;
    
    void normalizeFeatures(const FeatureVector& features, float* normalized);
};
```

**Model Input Tensor** (25 features):
- ToF distances (16): Normalized to [0, 1] range (0-4000mm)
- Roll, pitch, yaw (3): Normalized to [-1, 1] range (-180° to 180°)
- Accel x, y, z (3): Normalized to [-1, 1] range (-10 to 10 m/s²)
- Gyro x, y, z (3): Normalized to [-1, 1] range (-500 to 500 deg/s)

**Model Output**: Single confidence score [0.0, 1.0] indicating near-miss probability

**ESP-NN Integration**:
```cpp
// Register ESP-NN optimized operations
resolver.AddFullyConnected();
resolver.AddRelu();
resolver.AddSoftmax();
// ESP-NN automatically accelerates these ops on ESP32-S3
```

### 5. Status LED Component

**Purpose**: Provides visual feedback via WS2812 RGB LED using ESP-IDF RMT driver

**ESP-IDF Components**: status_led

**Classes**:

```cpp
class StatusLED {
public:
    void initialize();
    void setState(LEDState state);
    void update();  // Call in loop for animations
    
private:
    Adafruit_NeoPixel led;
    LEDState currentState;
    uint32_t lastUpdate;
};

enum class LEDState {
    INITIALIZING,      // Blue solid
    SD_ERROR,          // Red blinking 2Hz
    LOGGING_ACTIVE,    // Green breathing
    BUTTON_PRESSED,    // Yellow flash 500ms
    ML_NEAR_MISS,      // Orange alert 1s
    CRITICAL_PROXIMITY,// Red solid (priority override)
    BUFFER_WARNING     // Purple tint overlay
};
```

**LED Priority System**:
1. CRITICAL_PROXIMITY (highest - overrides all)
2. SD_ERROR
3. ML_NEAR_MISS
4. BUTTON_PRESSED
5. BUFFER_WARNING (overlay)
6. LOGGING_ACTIVE
7. INITIALIZING (lowest)

## Data Models

### Core Data Structures

**SensorFrame** - Single timestamped sensor reading
```cpp
struct SensorFrame {
    uint32_t timestamp_ms;
    uint16_t tof[16];
    float roll, pitch, yaw;
    float accel_x, accel_y, accel_z;
    float gyro_x, gyro_y, gyro_z;
};
```

**LogEntry** - Complete CSV row with derived features and flags
```cpp
struct LogEntry {
    SensorFrame sensors;
    uint16_t min_distance_mm;
    float approach_rate_mm_s;
    float jerk_magnitude_m_s3;
    float angular_velocity_deg_s;
    uint8_t flag_manual;
    uint8_t flag_auto;
    float confidence;
    
    String toCSV();  // Converts to CSV string
};
```

**SystemConfig** - Runtime configuration parameters
```cpp
struct SystemConfig {
    uint16_t log_interval_ms = 16;           // ~60Hz
    uint16_t flag_window_ms = 2000;          // 2 seconds
    float ml_threshold = 0.70;               // 70% confidence
    float approach_threshold_mm_s = 500.0;   // 500 mm/s
    float jerk_threshold_m_s3 = 2.0;         // 2 m/s³
    
    bool loadFromSD(const char* filename);
    void applyDefaults();
};
```

### State Machine

**System States**:
```cpp
enum class SystemState {
    INIT,              // Initializing sensors and SD card
    READY,             // Sensors ready, waiting to start logging
    LOGGING,           // Active data collection
    LOGGING_WITH_ML,   // Data collection + ML inference
    ERROR_SD,          // SD card error, sensors only
    ERROR_SENSOR       // Sensor error, cannot operate
};
```

**State Transitions**:
```
INIT → READY (all sensors OK, SD OK)
INIT → ERROR_SD (sensors OK, SD fail)
INIT → ERROR_SENSOR (sensor fail)

READY → LOGGING (user starts logging)
LOGGING → LOGGING_WITH_ML (ML model loaded)
LOGGING → ERROR_SD (SD write fail)

ERROR_SD → READY (SD recovered)
```

## Correctness Properties

*A property is a characteristic or behavior that should hold true across all valid executions of a system-essentially, a formal statement about what the system should do. Properties serve as the bridge between human-readable specifications and machine-verifiable correctness guarantees.*


### Property 1: SD card initialization detection
*For any* SD card state (present, absent, corrupted), the initialization function should correctly detect and report the card status
**Validates: Requirements 1.1**

### Property 2: Date directory format correctness
*For any* valid date, the created directory name should match the YYYY-MM-DD format exactly
**Validates: Requirements 1.2**

### Property 3: Log filename format correctness
*For any* timestamp, the generated log filename should match the "log_YYYYMMDD_HHMMSS.csv" format exactly
**Validates: Requirements 1.3**

### Property 4: Sensor data logging completeness
*For any* sensor reading (ToF or IMU), if written to the log, it should include a valid millisecond timestamp
**Validates: Requirements 1.4, 1.5**

### Property 5: Frame rate maintenance during buffering
*For any* sequence of sensor readings, buffering should not cause frame drops below 60Hz
**Validates: Requirements 1.6**

### Property 6: File rotation at size threshold
*For any* log file, when size exceeds 100MB, a new file should be created and logging should continue
**Validates: Requirements 1.7**

### Property 7: SD write error indication
*For any* SD write failure, the system should display red LED blink pattern and output error code to serial
**Validates: Requirements 1.8**

### Property 8: Timestamp monotonicity
*For any* sequence of sensor readings, timestamps should be monotonically increasing (never decrease)
**Validates: Requirements 2.2, 2.3**

### Property 9: Sensor synchronization within window
*For any* ToF and IMU reading pair within 10ms, they should appear in the same CSV row
**Validates: Requirements 2.4**

### Property 10: IMU interpolation correctness
*For any* IMU reading arriving between ToF frames, the synchronized data should use the most recent IMU value
**Validates: Requirements 2.5**

### Property 11: Timestamp drift detection
*For any* timestamp drift exceeding threshold, a warning should be logged with drift magnitude
**Validates: Requirements 2.6**

### Property 12: Button press recording
*For any* button press event, a timestamp should be recorded with millisecond precision
**Validates: Requirements 3.1**

### Property 13: Button press flag marker
*For any* button press, a flag marker (flag_manual=1) should appear in the Data_Log
**Validates: Requirements 3.2**

### Property 14: Button press LED feedback
*For any* button press, the Status_LED should flash yellow for 500ms
**Validates: Requirements 3.3**

### Property 15: Button debouncing
*For any* button hold duration exceeding 50ms, only one flag event should be registered
**Validates: Requirements 3.4**

### Property 16: Flag time window marking
*For any* flag event, all log entries within 1 second before and 1 second after should be marked
**Validates: Requirements 3.5**

### Property 17: CSV header presence
*For any* newly created log file, the first line should contain the complete CSV header with all column names
**Validates: Requirements 4.1**

### Property 18: CSV row format consistency
*For any* sensor reading logged, the CSV row should have comma-separated values in the correct field order
**Validates: Requirements 4.2**

### Property 19: Timestamp format consistency
*For any* log entry, the timestamp should be in milliseconds since boot format
**Validates: Requirements 4.3**

### Property 20: Sensor data precision
*For any* sensor reading, the logged values should match the input with correct units and 2 decimal precision
**Validates: Requirements 4.4, 4.5**

### Property 21: Flag column presence
*For any* log entry, flag_manual and flag_auto columns should be present with binary values (0 or 1)
**Validates: Requirements 4.6, 4.7**

### Property 22: Sensor fallback behavior
*For any* missing sensor reading, the last known valid value should be used in the log
**Validates: Requirements 4.8**

### Property 23: Circular buffer capacity
*For any* buffer state, the capacity should be at least 120 frames (2 seconds at 60Hz)
**Validates: Requirements 5.2**

### Property 24: Buffer flush trigger
*For any* buffer state, when fill level reaches 80%, a flush operation should be triggered
**Validates: Requirements 5.3**

### Property 25: Write latency warning
*For any* SD write operation exceeding 100ms, a warning should be logged to serial with latency value
**Validates: Requirements 5.4**

### Property 26: Flagged event priority during overflow
*For any* buffer overflow condition, flagged events should be preserved while non-flagged frames may be dropped
**Validates: Requirements 5.6**

### Property 27: Approach rate calculation correctness
*For any* two consecutive ToF readings, the calculated approach rate should equal (distance_delta / time_delta)
**Validates: Requirements 7.1**

### Property 28: Approach rate threshold detection
*For any* approach rate exceeding 500 mm/s, a potential near-miss flag should be set
**Validates: Requirements 7.2**

### Property 29: Jerk magnitude calculation correctness
*For any* two consecutive acceleration readings, the calculated jerk magnitude should equal sqrt(jerk_x² + jerk_y² + jerk_z²)
**Validates: Requirements 7.3**

### Property 30: Jerk threshold detection
*For any* jerk magnitude exceeding 2 m/s³, a potential near-miss flag should be set
**Validates: Requirements 7.4**

### Property 31: Combined near-miss detection
*For any* sequence where rapid approach (>500 mm/s) and high jerk (>2 m/s³) occur within 500ms, Status_LED should display orange alert
**Validates: Requirements 7.5**

### Property 32: Critical proximity alert
*For any* condition where minimum distance <300mm and approach rate >200 mm/s, Status_LED should display red alert
**Validates: Requirements 7.6**

### Property 33: ML model integrity verification
*For any* loaded ML model, integrity checks should pass before inference is enabled
**Validates: Requirements 8.2**

### Property 34: Input tensor normalization
*For any* sensor data, the prepared input tensor should have all 25 features normalized to appropriate ranges
**Validates: Requirements 8.3**

### Property 35: ML inference completion
*For any* input tensor, inference should complete successfully and return a confidence score
**Validates: Requirements 8.4**

### Property 36: ML inference latency
*For any* inference operation, completion time should be within 50ms
**Validates: Requirements 8.5**

### Property 37: ML confidence threshold alerting
*For any* ML confidence score exceeding threshold (default 0.7), Status_LED should display orange alert and event should be logged
**Validates: Requirements 8.6**

### Property 38: ML latency warning
*For any* inference operation exceeding 50ms, a performance warning should be logged
**Validates: Requirements 8.7**

### Property 39: ML auto-flag logging
*For any* ML-detected near-miss event, an auto-flag marker (flag_auto=1) should appear in the Data_Log
**Validates: Requirements 9.1**

### Property 40: ML confidence score logging
*For any* auto-flag marker, the ML confidence score should be included in the confidence column
**Validates: Requirements 9.2**

### Property 41: Dual flag recording
*For any* simultaneous manual and ML detection, both flag_manual=1 and flag_auto=1 should be recorded
**Validates: Requirements 9.3**

### Property 42: ML inference frequency
*For any* frame when ML is active, inference should be executed (60Hz operation)
**Validates: Requirements 9.4**

### Property 43: LED state correctness
*For any* system state, the Status_LED should display the correct color and pattern according to the priority system
**Validates: Requirements 10.1, 10.2, 10.3, 10.4, 10.5, 10.6, 10.7**

### Property 44: Sensor initialization error reporting
*For any* sensor initialization failure, a detailed error message with sensor name and error code should be output to serial
**Validates: Requirements 11.1**

### Property 45: SD failure graceful degradation
*For any* SD card failure, sensor operation should continue and error should be displayed on Status_LED
**Validates: Requirements 11.2**

### Property 46: Write error type logging
*For any* file write failure, the error type (timeout, disk full, filesystem error) should be logged to serial
**Validates: Requirements 11.3**

### Property 47: Buffer overflow frame count logging
*For any* buffer overflow event, the number of dropped frames should be logged
**Validates: Requirements 11.4**

### Property 48: ML failure graceful degradation
*For any* ML model load failure, error details should be logged and system should continue in data collection mode
**Validates: Requirements 11.5**

### Property 49: Error counter maintenance
*For any* system anomaly, the error counter should increment and be accessible via serial command
**Validates: Requirements 11.6**

### Property 50: Configuration parameter application
*For any* valid config.txt file, all specified parameters should be applied to system settings
**Validates: Requirements 12.2**

### Property 51: Configuration fallback
*For any* missing or invalid config.txt, default values should be used and a warning should be logged
**Validates: Requirements 12.3**

### Property 52: Configuration startup reporting
*For any* system boot, active configuration settings should be output to serial
**Validates: Requirements 12.4**

### Property 53: GNSS data integration
*For any* valid GNSS fix, latitude, longitude, altitude, and GPS timestamp should be appended to log rows
**Validates: Requirements 13.2**

### Property 54: GNSS failure handling
*For any* GNSS state (no fix or not detected), the system should handle it gracefully and continue logging sensor data
**Validates: Requirements 13.3, 13.4**

### Property 55: Minimum distance calculation
*For any* ToF 4x4 grid, the calculated minimum distance should equal the smallest value across all 16 zones
**Validates: Requirements 14.1**

### Property 56: Approach rate derivation
*For any* two consecutive ToF frames, the computed approach rate should equal (current_min - previous_min) / time_delta
**Validates: Requirements 14.2**

### Property 57: Jerk magnitude derivation
*For any* two consecutive IMU acceleration readings, the computed jerk magnitude should equal sqrt((Δaccel_x/Δt)² + (Δaccel_y/Δt)² + (Δaccel_z/Δt)²)
**Validates: Requirements 14.3**

### Property 58: Angular velocity derivation
*For any* two consecutive IMU orientation readings, the computed angular velocity should equal sqrt(gyro_x² + gyro_y² + gyro_z²)
**Validates: Requirements 14.4**

### Property 59: Derived features logging
*For any* log entry, derived features (min_distance, approach_rate, jerk_magnitude, angular_velocity) should be present in the CSV
**Validates: Requirements 14.5**

### Property 60: Derived features in ML input
*For any* ML inference, the input tensor should include derived features alongside raw sensor values
**Validates: Requirements 14.6**

## Error Handling

### Error Categories

**1. Sensor Errors**
- ToF sensor not detected (I2C communication failure)
- IMU sensor not detected (SPI communication failure)
- Sensor data timeout (no new data within expected interval)
- Sensor data invalid (out-of-range values, checksum failure)

**Recovery Strategy**:
- Retry initialization up to 3 times with 1-second delay
- If persistent, mark sensor as unavailable and continue with remaining sensors
- Log detailed error to serial with I2C/SPI error codes
- Display error on Status_LED (red blink pattern)

**2. Storage Errors**
- SD card not detected
- SD card full (no space for new files)
- File write timeout (>100ms)
- Filesystem corruption

**Recovery Strategy**:
- Retry SD initialization once
- If SD unavailable, continue sensor operation without logging
- Buffer data in RAM (up to 2 seconds) in case SD recovers
- Display SD error on Status_LED (red blink pattern)
- Log error details to serial

**3. ML Errors**
- Model file not found in flash
- Model integrity check failure
- Tensor allocation failure (insufficient RAM)
- Inference timeout (>50ms)

**Recovery Strategy**:
- Log detailed error to serial
- Continue operation in data collection mode only
- Do not attempt to reload model (requires reflash)
- Display warning on Status_LED (purple tint)

**4. Configuration Errors**
- config.txt missing
- config.txt malformed (invalid syntax)
- Parameter out of valid range

**Recovery Strategy**:
- Use default configuration values
- Log warning to serial with specific parameter that failed
- Continue normal operation
- Output active configuration to serial at startup

### Error Reporting Format

**Serial Output**:
```
[ERROR] <timestamp_ms> <component> <error_code> <description>
[WARN]  <timestamp_ms> <component> <warning_code> <description>
```

**Examples**:
```
[ERROR] 1234 TOF_SENSOR 0x01 I2C communication timeout
[ERROR] 5678 SD_CARD 0x10 Card not detected
[WARN]  9012 ML_INFERENCE 0x20 Latency exceeded 50ms: 67ms
[WARN]  3456 SYNC 0x30 Timestamp drift detected: 15ms
```

### Graceful Degradation Hierarchy

**Full Operation**: ToF + IMU + SD + ML
↓ (SD failure)
**Sensor Only**: ToF + IMU + LED feedback (no logging)
↓ (IMU failure)
**ToF Only**: ToF + LED feedback (basic proximity detection)
↓ (ToF failure)
**Error State**: LED error indication, serial diagnostics

## Testing Strategy

### Unit Testing

**Test Framework**: ESP-IDF Unity Test Framework with PlatformIO

**Unit Test Coverage**:

1. **Sensor Manager Tests**
   - ToF sensor initialization and data reading
   - IMU sensor initialization and data reading
   - Button debouncing logic
   - Timestamp generation

2. **Data Processor Tests**
   - Sensor synchronization within 10ms window
   - Feature extraction calculations (approach rate, jerk, angular velocity)
   - Near-miss detection logic (threshold checks)
   - Data normalization for ML input

3. **Storage Manager Tests**
   - Circular buffer push/pop operations
   - Buffer fill percentage calculation
   - CSV formatting and header generation
   - Filename and directory name generation
   - Configuration file parsing

4. **ML Inference Tests**
   - Model loading and integrity verification
   - Input tensor preparation and normalization
   - Inference execution (with mock model)
   - Confidence score extraction

5. **Status LED Tests**
   - State transitions and priority handling
   - Color and pattern generation
   - Animation timing

**Example Unit Test**:
```cpp
TEST_CASE("Approach rate calculation", "[data_processor]") {
    DataProcessor processor;
    
    // Setup: Two ToF readings 16ms apart
    ToFData frame1 = {1000, {500, 500, ...}, true};  // 500mm min distance
    ToFData frame2 = {1016, {400, 400, ...}, true};  // 400mm min distance
    
    // Calculate approach rate
    float rate = processor.calculateApproachRate(frame1, frame2);
    
    // Expected: (400 - 500) / 0.016 = -6250 mm/s (negative = approaching)
    TEST_ASSERT_FLOAT_WITHIN(1.0, -6250.0, rate);
}
```

### Property-Based Testing

**Test Framework**: We will use a custom property testing implementation for ESP-IDF, as standard PBT libraries (QuickCheck, Hypothesis) are not available for embedded systems.

**Property Test Implementation Strategy**:
- Generate random test inputs within valid ranges
- Run each property test for minimum 100 iterations
- Use deterministic random seed for reproducibility
- Log failing test cases for debugging

**Property Test Coverage**:

Each correctness property from the design document will be implemented as a property-based test. The tests will be tagged with comments referencing the specific property number.

**Example Property Test**:
```cpp
// Feature: near-miss-detection-ml, Property 27: Approach rate calculation correctness
TEST_CASE("Property: Approach rate calculation correctness", "[data_processor][property]") {
    DataProcessor processor;
    
    for (int i = 0; i < 100; i++) {
        // Generate random distances and timestamps
        uint16_t dist1 = esp_random() % 3900 + 100;  // 100-4000mm
        uint16_t dist2 = esp_random() % 3900 + 100;
        uint32_t time1 = esp_random() % 99000 + 1000;
        uint32_t time2 = time1 + (esp_random() % 40 + 10);  // 10-50ms later
        
        ToFData frame1 = {time1, {dist1, dist1, ...}, true};
        ToFData frame2 = {time2, {dist2, dist2, ...}, true};
        
        // Calculate approach rate
        float rate = processor.calculateApproachRate(frame1, frame2);
        
        // Property: rate should equal (dist2 - dist1) / ((time2 - time1) / 1000.0)
        float expected = (float)(dist2 - dist1) / ((time2 - time1) / 1000.0);
        TEST_ASSERT_FLOAT_WITHIN(0.1, expected, rate);
    }
}
```

**Property Test Organization**:
- Tests grouped by component (sensor, processor, storage, ml)
- Each test file contains related properties
- Minimum 100 iterations per property test
- Configurable via `#define PBT_ITERATIONS 100`

### Integration Testing

**Integration Test Scenarios**:

1. **End-to-End Data Collection**
   - Start system with SD card
   - Generate sensor data for 10 seconds
   - Press button to flag events
   - Verify CSV file contains all data with correct format
   - Verify flagged events are marked correctly

2. **ML Inference Pipeline**
   - Load ML model
   - Feed sensor data
   - Verify inference runs at 60Hz
   - Verify confidence scores are logged
   - Verify auto-flags appear in CSV

3. **Error Recovery**
   - Simulate SD card removal during logging
   - Verify system continues sensor operation
   - Verify LED displays error state
   - Reinsert SD card
   - Verify logging resumes

4. **Buffer Overflow Handling**
   - Slow down SD writes artificially
   - Generate sensor data faster than write speed
   - Verify flagged events are preserved
   - Verify non-flagged frames are dropped
   - Verify overflow is logged

### Hardware-in-Loop Testing

**Test Setup**:
- ESP32-S3 connected to PC via USB
- ToF sensor and IMU connected
- SD card inserted
- Button connected
- Serial monitor for logging

**Test Procedures**:
1. Flash firmware with test mode enabled
2. Run automated test sequences via serial commands
3. Verify sensor readings are within expected ranges
4. Verify LED patterns match expected states
5. Verify CSV files are created and formatted correctly

### Performance Testing

**Metrics to Measure**:
- Frame rate (should maintain 60Hz)
- Sensor-to-LED latency (should be <20ms)
- ML inference latency (should be <50ms)
- SD write latency (should be <100ms)
- Buffer utilization (should stay <80% normally)
- RAM usage (should be <200KB)
- CPU usage (should be <70%)

**Performance Test Tools**:
- Built-in timing measurements using `micros()`
- Serial output of performance metrics every second
- CSV logging of performance data for analysis

## Implementation Notes

### ESP-IDF Component Dependencies

**Required Components** (specified in platformio.ini):
```ini
[env:esp32-s3-devkitc-1]
platform = espressif32
board = esp32-s3-devkitc-1
framework = espidf

lib_deps =
    https://github.com/RJRP44/VL53L8CX-Library.git
    
platform_packages =
    platformio/framework-espidf @ ~3.50000.0

build_flags =
    -DCONFIG_ESP_NN_ENABLE=1
```

**ESP-IDF Native Components**:
- `esp_vfs_fat` - SD card filesystem
- `driver/gpio` - GPIO control
- `driver/i2c` - I2C communication for ToF
- `driver/spi_master` - SPI communication for IMU and SD
- `driver/rmt` - RMT driver for WS2812 LED
- `esp-tflite-micro` - TensorFlow Lite with ESP-NN
- `esp_timer` - High-resolution timing

### Memory Considerations

**ESP32-S3 Memory Budget**:
- Total RAM: 512KB
- Arduino framework overhead: ~50KB
- Sensor libraries: ~30KB
- Circular buffer (120 frames × 150 bytes): ~18KB
- TFLite tensor arena: 50KB
- ML model in flash: ~100KB
- Application code: ~50KB
- **Total estimated usage**: ~200KB RAM, ~200KB Flash
- **Remaining**: ~300KB RAM, ~3.8MB Flash (plenty of headroom)

### Pin Assignment Rationale

- **I2C for ToF**: Standard I2C bus, no conflicts
- **SPI for IMU**: Dedicated SPI bus, reliable communication
- **SPI for SD**: Shared with IMU SPI bus, different CS pins
- **GPIO for Button**: Internal pull-up, active-low
- **GPIO for LED**: Supports WS2812 timing requirements

### Real-Time Considerations

**Timing Critical Sections**:
1. ToF data reading (must complete within 16.67ms)
2. LED updates (must not block sensor reading)
3. SD writes (must be asynchronous via buffering)

**FreeRTOS Task Design**:
- **Sensor Task** (Priority: High): Reads ToF and IMU at 60Hz, uses `vTaskDelay()` for timing
- **Storage Task** (Priority: Medium): Flushes circular buffer to SD card asynchronously
- **ML Task** (Priority: Medium): Runs inference on queued sensor data
- **LED Task** (Priority: Low): Updates LED patterns without blocking sensors
- Use FreeRTOS queues for inter-task communication
- Use `esp_timer_get_time()` for microsecond-precision timestamps

### Future Enhancements

**Potential Improvements**:
1. WiFi data streaming for real-time monitoring
2. Bluetooth configuration interface
3. Battery power management with sleep modes
4. Over-the-air (OTA) firmware updates
5. Web dashboard for data visualization
6. Multi-model ML ensemble for improved accuracy
7. Kalman filtering for smoother sensor fusion
8. GNSS integration for spatial analysis

**Scalability Considerations**:
- Modular architecture allows easy addition of new sensors
- Configuration system supports new parameters without code changes
- CSV format can be extended with new columns
- ML model can be retrained and reflashed independently
