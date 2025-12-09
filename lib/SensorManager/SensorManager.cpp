/**
 * SensorManager.cpp
 * 
 * Implementation of SensorManager class
 */

#include "SensorManager.h"

/**
 * Constructor
 */
SensorManager::SensorManager() 
    : tofSensor(nullptr),
      tofHealthy(false),
      imuHealthy(false),
      lastButtonPressTime(0),
      lastButtonReadTime(0),
      lastButtonState(HIGH) {
}

/**
 * Destructor
 */
SensorManager::~SensorManager() {
    if (tofSensor != nullptr) {
        delete tofSensor;
    }
}

/**
 * Initialize all sensors
 * Attempts to initialize ToF, IMU, and button
 * Returns true if at least one sensor initialized successfully
 */
bool SensorManager::initialize() {
    Serial.println("\n=== Initializing Sensor Manager ===");
    
    bool anySuccess = false;
    
    // Initialize button (always succeeds)
    if (initializeButton()) {
        Serial.println("[OK] Button initialized");
        anySuccess = true;
    }
    
    // Initialize ToF sensor
    if (initializeToF()) {
        Serial.println("[OK] ToF sensor initialized");
        tofHealthy = true;
        anySuccess = true;
    } else {
        Serial.println("[FAIL] ToF sensor initialization failed");
        tofHealthy = false;
    }
    
    // Initialize IMU sensor
    if (initializeIMU()) {
        Serial.println("[OK] IMU sensor initialized");
        imuHealthy = true;
        anySuccess = true;
    } else {
        Serial.println("[FAIL] IMU sensor initialization failed");
        imuHealthy = false;
    }
    
    Serial.println("===================================\n");
    
    return anySuccess;
}

/**
 * Initialize ToF sensor (VL53L8CX on I2C)
 */
bool SensorManager::initializeToF() {
    Serial.println("Initializing VL53L8CX ToF sensor...");
    
    // Configure LPN (Low Power/Enable) pin FIRST
    pinMode(TOF_LPN_PIN, OUTPUT);
    digitalWrite(TOF_LPN_PIN, LOW);   // Disable sensor first
    delay(10);
    
    // Configure I2C pins for ESP32
    Wire.begin(TOF_SDA_PIN, TOF_SCL_PIN);
    Wire.setClock(400000);  // 400kHz I2C
    delay(10);
    
    // Enable sensor
    digitalWrite(TOF_LPN_PIN, HIGH);
    delay(100);  // Wait for sensor to power up
    
    // Create ToF sensor object
    tofSensor = new VL53L8CX(&Wire, TOF_LPN_PIN);
    
    // Initialize sensor with retry logic (up to 3 attempts)
    for (int attempt = 0; attempt < 3; attempt++) {
        if (attempt > 0) {
            Serial.print("Retry attempt ");
            Serial.print(attempt);
            Serial.println("...");
            delay(1000);
        }
        
        // Initialize VL53L8CX
        if (tofSensor->begin() == 0) {
            Serial.println("ToF sensor detected!");
            
            if (tofSensor->init() == 0) {
                // Configure sensor for 4x4 mode at 60Hz
                tofSensor->set_resolution(VL53L8CX_RESOLUTION_4X4);
                tofSensor->set_ranging_frequency_hz(60);
                tofSensor->set_ranging_mode(VL53L8CX_RANGING_MODE_CONTINUOUS);
                tofSensor->start_ranging();
                
                Serial.println("ToF sensor configured: 4x4 mode, 60Hz");
                return true;
            }
        }
    }
    
    logError("TOF_SENSOR", 0x01, "I2C communication timeout after 3 attempts");
    return false;
}

/**
 * Initialize IMU sensor (BNO08x on SPI)
 */
bool SensorManager::initializeIMU() {
    Serial.println("Initializing BNO08x IMU sensor...");
    
    // Configure SPI
    SPI.begin(IMU_SCK_PIN, IMU_MISO_PIN, IMU_MOSI_PIN, IMU_CS_PIN);
    
    // Configure reset pin
    pinMode(IMU_RST_PIN, OUTPUT);
    digitalWrite(IMU_RST_PIN, LOW);
    delay(10);
    digitalWrite(IMU_RST_PIN, HIGH);
    delay(100);
    
    // Configure interrupt pin
    pinMode(IMU_INT_PIN, INPUT);
    
    // Initialize sensor with retry logic (up to 3 attempts)
    for (int attempt = 0; attempt < 3; attempt++) {
        if (attempt > 0) {
            Serial.print("Retry attempt ");
            Serial.print(attempt);
            Serial.println("...");
            delay(1000);
        }
        
        // Initialize BNO08x
        if (imuSensor.beginSPI(IMU_CS_PIN, IMU_INT_PIN, IMU_RST_PIN)) {
            // Sensor initialized successfully
            
            // Enable required reports at appropriate rates
            // Rotation Vector for orientation (roll, pitch, yaw) at 100Hz
            imuSensor.enableRotationVector(10);  // 10ms = 100Hz
            
            // Linear Acceleration (gravity removed) at 100Hz
            imuSensor.enableLinearAccelerometer(10);  // 10ms = 100Hz
            
            // Gyroscope (angular velocity) at 100Hz
            imuSensor.enableGyro(10);  // 10ms = 100Hz
            
            Serial.println("IMU sensor configured: Rotation Vector, Linear Accel, Gyro at 100Hz");
            return true;
        }
    }
    
    logError("IMU_SENSOR", 0x02, "SPI communication timeout after 3 attempts");
    return false;
}

/**
 * Initialize button input
 */
bool SensorManager::initializeButton() {
    pinMode(FLAG_BUTTON_PIN, INPUT_PULLUP);
    lastButtonState = digitalRead(FLAG_BUTTON_PIN);
    lastButtonReadTime = millis();
    return true;
}

/**
 * Read ToF sensor data
 */
bool SensorManager::readToFData(ToFData& data) {
    if (!tofHealthy || tofSensor == nullptr) {
        data.valid = false;
        return false;
    }
    
    // Check if data is ready
    uint8_t isReady = 0;
    if (tofSensor->check_data_ready(&isReady) != 0 || !isReady) {
        data.valid = false;
        return false;
    }
    
    // Read ranging data
    VL53L8CX_ResultsData results;
    if (tofSensor->get_ranging_data(&results) != 0) {
        logWarning("TOF_SENSOR", 0x10, "Failed to read ranging data");
        data.valid = false;
        return false;
    }
    
    // Package data into ToFData structure
    data.timestamp_ms = getTimestamp();
    data.valid = true;
    
    // Copy distance data (4x4 grid in row-major order)
    for (int i = 0; i < 16; i++) {
        data.distances[i] = results.distance_mm[i];
    }
    
    return true;
}

/**
 * Read IMU sensor data
 */
bool SensorManager::readIMUData(IMUData& data) {
    if (!imuHealthy) {
        data.valid = false;
        return false;
    }
    
    // Check if data is available
    if (!imuSensor.wasReset()) {
        // Get timestamp first
        data.timestamp_ms = getTimestamp();
        data.valid = true;
        
        // Read sensor data if available
        if (imuSensor.getSensorEvent()) {
            // Convert quaternion to Euler angles (roll, pitch, yaw)
            float qI = imuSensor.getQuatI();
            float qJ = imuSensor.getQuatJ();
            float qK = imuSensor.getQuatK();
            float qReal = imuSensor.getQuatReal();
            
            // Calculate roll (rotation around X-axis)
            float sinr_cosp = 2.0f * (qReal * qI + qJ * qK);
            float cosr_cosp = 1.0f - 2.0f * (qI * qI + qJ * qJ);
            data.roll = atan2(sinr_cosp, cosr_cosp) * 180.0f / PI;
            
            // Calculate pitch (rotation around Y-axis)
            float sinp = 2.0f * (qReal * qJ - qK * qI);
            if (abs(sinp) >= 1.0f) {
                data.pitch = copysign(90.0f, sinp);  // Use 90 degrees if out of range
            } else {
                data.pitch = asin(sinp) * 180.0f / PI;
            }
            
            // Calculate yaw (rotation around Z-axis)
            float siny_cosp = 2.0f * (qReal * qK + qI * qJ);
            float cosy_cosp = 1.0f - 2.0f * (qJ * qJ + qK * qK);
            data.yaw = atan2(siny_cosp, cosy_cosp) * 180.0f / PI;
            
            // Read linear acceleration (gravity removed)
            data.accel_x = imuSensor.getLinAccelX();
            data.accel_y = imuSensor.getLinAccelY();
            data.accel_z = imuSensor.getLinAccelZ();
            
            // Read gyroscope (angular velocity)
            data.gyro_x = imuSensor.getGyroX();
            data.gyro_y = imuSensor.getGyroY();
            data.gyro_z = imuSensor.getGyroZ();
            
            return true;
        }
    }
    
    data.valid = false;
    return false;
}

/**
 * Check if button is pressed (with debouncing)
 * Returns true only once per button press
 */
bool SensorManager::isButtonPressed() {
    uint32_t currentTime = millis();
    bool currentState = digitalRead(FLAG_BUTTON_PIN);
    
    // Button is active LOW (pressed = LOW)
    // Check if button state changed from HIGH to LOW (press event)
    if (currentState == LOW && lastButtonState == HIGH) {
        // Check if enough time has passed since last press (debounce)
        if (currentTime - lastButtonReadTime >= BUTTON_DEBOUNCE_MS) {
            lastButtonPressTime = currentTime;
            lastButtonReadTime = currentTime;
            lastButtonState = currentState;
            return true;
        }
    }
    
    // Update state for next check
    if (currentTime - lastButtonReadTime >= BUTTON_DEBOUNCE_MS) {
        lastButtonState = currentState;
        lastButtonReadTime = currentTime;
    }
    
    return false;
}

/**
 * Get current timestamp in milliseconds since boot
 */
uint32_t SensorManager::getTimestamp() {
    return millis();
}

/**
 * Log error message to serial
 */
void SensorManager::logError(const char* component, uint8_t errorCode, const char* description) {
    Serial.print("[ERROR] ");
    Serial.print(getTimestamp());
    Serial.print(" ");
    Serial.print(component);
    Serial.print(" 0x");
    Serial.print(errorCode, HEX);
    Serial.print(" ");
    Serial.println(description);
}

/**
 * Log warning message to serial
 */
void SensorManager::logWarning(const char* component, uint8_t warningCode, const char* description) {
    Serial.print("[WARN]  ");
    Serial.print(getTimestamp());
    Serial.print(" ");
    Serial.print(component);
    Serial.print(" 0x");
    Serial.print(warningCode, HEX);
    Serial.print(" ");
    Serial.println(description);
}
