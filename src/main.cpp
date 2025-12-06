/**
 * ProxiTof Near-Miss Detection System
 * ESP32-S3 based multi-sensor proximity detection with ML inference
 * 
 * Main application entry point
 */

#include <Arduino.h>
#include "DataModels.h"

void setup() {
    // Initialize serial communication
    Serial.begin(115200);
    while (!Serial && millis() < 3000) {
        delay(10);
    }
    
    Serial.println("\n=== ProxiTof Near-Miss Detection System ===");
    Serial.println("Initializing...");
    
    // Test configuration parser
    SystemConfig config;
    Serial.println("\n--- Testing Configuration Parser ---");
    
    // Test with default values
    Serial.println("Default configuration:");
    config.printConfig();
    
    // Test parsing from string
    String testConfig = "log_interval_ms=20\nflag_window_ms=3000\nml_threshold=0.8\napproach_threshold_mm_s=600\njerk_threshold_m_s3=2.5\n";
    Serial.println("Parsing test configuration string...");
    bool success = config.parseFromString(testConfig);
    Serial.print("Parse result: ");
    Serial.println(success ? "SUCCESS" : "FAILED");
    
    Serial.println("Updated configuration:");
    config.printConfig();
    
    // Test parameter validation
    Serial.println("Testing parameter validation...");
    bool valid = config.validateParameters();
    Serial.print("Validation result: ");
    Serial.println(valid ? "VALID" : "INVALID");
    
    // TODO: Initialize components in subsequent tasks
}

void loop() {
    // TODO: Implement main loop in subsequent tasks
    delay(100);
}
