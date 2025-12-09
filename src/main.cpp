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
    static uint32_t tofReadCount = 0;
    static uint32_t lastFPSCheck = 0;
    uint32_t currentTime = millis();
    
    // Read ToF sensor at 60Hz (every 16.67ms ≈ 16ms)
    if (currentTime - lastToFRead >= 16) {
        ToFData tofData;
        if (sensorManager.readToFData(tofData)) {
            tofReadCount++;
            
            // Print detailed status every second
            if (currentTime - lastStatusPrint >= 1000) {
                // Calculate actual frame rate
                float actualFPS = tofReadCount * 1000.0f / (currentTime - lastFPSCheck);
                
                Serial.println("\n╔════════════════════════════════════════════════════════╗");
                Serial.print("║ ToF 4x4 Distance Grid (mm) - Time: ");
                Serial.print(tofData.timestamp_ms);
                Serial.println(" ms");
                Serial.print("║ Actual Frame Rate: ");
                Serial.print(actualFPS, 1);
                Serial.println(" Hz");
                Serial.println("╠════════════════════════════════════════════════════════╣");
                
                // Display 4x4 grid
                for (int row = 0; row < 4; row++) {
                    Serial.print("║  ");
                    for (int col = 0; col < 4; col++) {
                        int index = row * 4 + col;
                        uint16_t dist = tofData.distances[index];
                        
                        // Format: 4 digits, right-aligned
                        if (dist < 1000) Serial.print(" ");
                        if (dist < 100) Serial.print(" ");
                        if (dist < 10) Serial.print(" ");
                        Serial.print(dist);
                        Serial.print("  ");
                    }
                    Serial.println("║");
                }
                
                // Find and display min/max
                uint16_t minDist = 4000;
                uint16_t maxDist = 0;
                for (int i = 0; i < 16; i++) {
                    if (tofData.distances[i] < minDist) minDist = tofData.distances[i];
                    if (tofData.distances[i] > maxDist) maxDist = tofData.distances[i];
                }
                
                Serial.println("╠════════════════════════════════════════════════════════╣");
                Serial.print("║ Min: ");
                Serial.print(minDist);
                Serial.print(" mm  |  Max: ");
                Serial.print(maxDist);
                Serial.println(" mm");
                Serial.println("╚════════════════════════════════════════════════════════╝");
                
                lastStatusPrint = currentTime;
                lastFPSCheck = currentTime;
                tofReadCount = 0;
            }
        }
        lastToFRead = currentTime;
    }
    
    // Read IMU sensor at 100Hz (every 10ms)
    if (currentTime - lastIMURead >= 10) {
        IMUData imuData;
        if (sensorManager.readIMUData(imuData)) {
            // Successfully read IMU data
            // Data is available for processing
            // (Not displayed to avoid cluttering serial output)
        }
        lastIMURead = currentTime;
    }
    
    // Check button press
    if (sensorManager.isButtonPressed()) {
        Serial.println("\n╔════════════════════════════════════════════════════════╗");
        Serial.print("║ [BUTTON] Pressed at ");
        Serial.print(sensorManager.getLastButtonPressTime());
        Serial.println(" ms");
        Serial.println("║ Flag event recorded!");
        Serial.println("╚════════════════════════════════════════════════════════╝\n");
    }
    
    // Small delay to prevent watchdog issues
    delay(1);
}
