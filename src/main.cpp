/**
 * ProxiTof Near-Miss Detection System
 * ESP32-S3 based multi-sensor proximity detection with ML inference
 * 
 * Main application entry point
 */

#include <Arduino.h>
#include <DataModels.h>
#include <SensorManager.h>

// Global objects
SensorManager sensorManager;
SystemConfig config;

void setup() {
    // Initialize serial communication
    Serial.begin(115200);
    delay(2000);  // Give time for USB CDC to initialize
    
    Serial.println("\n=== ProxiTof Near-Miss Detection System ===");
    Serial.println("Initializing...");
    
    // Load configuration
    config.applyDefaults();
    config.printConfig();
    
    // Initialize sensor manager
    if (!sensorManager.initialize()) {
        Serial.println("[FATAL] No sensors initialized successfully!");
        Serial.println("System cannot operate. Please check hardware connections.");
        while (1) {
            delay(1000);
        }
    }
    
    Serial.println("\n=== System Ready ===");
    Serial.println("Sensor Manager initialized successfully");
    Serial.print("ToF Sensor: ");
    Serial.println(sensorManager.isToFHealthy() ? "HEALTHY" : "UNAVAILABLE");
    Serial.print("IMU Sensor: ");
    Serial.println(sensorManager.isIMUHealthy() ? "HEALTHY" : "UNAVAILABLE");
    Serial.println("\nStarting sensor data collection...");
    Serial.println("Press the button (GPIO 3) to test button functionality\n");
}

void loop() {
    static uint32_t lastToFRead = 0;
    static uint32_t lastIMURead = 0;
    static uint32_t lastStatusPrint = 0;
    uint32_t currentTime = millis();
    
    // Read ToF sensor at ~60Hz (every 16ms)
    if (currentTime - lastToFRead >= 16) {
        ToFData tofData;
        if (sensorManager.readToFData(tofData)) {
            // Successfully read ToF data
            // Find minimum distance
            uint16_t minDist = 4000;
            for (int i = 0; i < 16; i++) {
                if (tofData.distances[i] < minDist) {
                    minDist = tofData.distances[i];
                }
            }
            
            // Print status every second
            if (currentTime - lastStatusPrint >= 1000) {
                Serial.print("[ToF] Time: ");
                Serial.print(tofData.timestamp_ms);
                Serial.print(" ms, Min Distance: ");
                Serial.print(minDist);
                Serial.println(" mm");
                lastStatusPrint = currentTime;
            }
        }
        lastToFRead = currentTime;
    }
    
    // Read IMU sensor at ~100Hz (every 10ms)
    if (currentTime - lastIMURead >= 10) {
        IMUData imuData;
        if (sensorManager.readIMUData(imuData)) {
            // Successfully read IMU data
            // Data is available for processing
        }
        lastIMURead = currentTime;
    }
    
    // Check button press
    if (sensorManager.isButtonPressed()) {
        Serial.print("\n[BUTTON] Button pressed at ");
        Serial.print(sensorManager.getLastButtonPressTime());
        Serial.println(" ms");
        Serial.println("         Flag event recorded!\n");
    }
    
    // Small delay to prevent watchdog issues
    delay(1);
}
