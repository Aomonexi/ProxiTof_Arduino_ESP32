/**
 * SystemConfig.cpp
 * 
 * Implementation of SystemConfig methods for ProxiTof system
 */

#include "../include/DataModels.h"
#include <SD.h>

bool SystemConfig::loadFromSD(const char* filename) {
    // First apply defaults in case of partial failure
    applyDefaults();
    
    // Check if SD card is available
    if (!SD.begin()) {
        Serial.println("[WARN] ConfigParser: SD card not available, using defaults");
        return false;
    }
    
    // Check if config file exists
    if (!SD.exists(filename)) {
        Serial.print("[WARN] ConfigParser: Config file not found: ");
        Serial.println(filename);
        Serial.println("[INFO] ConfigParser: Using default configuration");
        return false;
    }
    
    // Open config file
    File configFile = SD.open(filename, FILE_READ);
    if (!configFile) {
        Serial.print("[ERROR] ConfigParser: Failed to open config file: ");
        Serial.println(filename);
        return false;
    }
    
    // Read entire file content
    String content = "";
    while (configFile.available()) {
        content += (char)configFile.read();
    }
    configFile.close();
    
    Serial.print("[INFO] ConfigParser: Loaded config file: ");
    Serial.println(filename);
    
    // Parse the content
    bool success = parseFromString(content);
    
    if (success) {
        Serial.println("[INFO] ConfigParser: Configuration parsed successfully");
    } else {
        Serial.println("[WARN] ConfigParser: Some configuration parameters were invalid, using defaults for those");
    }
    
    return success;
}

bool SystemConfig::parseFromString(const String& content) {
    bool allValid = true;
    int lineNumber = 1;
    int startIndex = 0;
    
    // Parse line by line
    while (startIndex < content.length()) {
        int endIndex = content.indexOf('\n', startIndex);
        if (endIndex == -1) {
            endIndex = content.length();
        }
        
        String line = content.substring(startIndex, endIndex);
        line = trim(line);
        
        // Skip empty lines and comments
        if (line.length() > 0 && !line.startsWith("#")) {
            // Find the equals sign
            int equalsIndex = line.indexOf('=');
            if (equalsIndex > 0 && equalsIndex < line.length() - 1) {
                String key = trim(line.substring(0, equalsIndex));
                String value = trim(line.substring(equalsIndex + 1));
                
                if (!setParameter(key, value)) {
                    Serial.print("[WARN] ConfigParser: Invalid parameter on line ");
                    Serial.print(lineNumber);
                    Serial.print(": ");
                    Serial.println(line);
                    allValid = false;
                }
            } else if (line.length() > 0) {
                Serial.print("[WARN] ConfigParser: Malformed line ");
                Serial.print(lineNumber);
                Serial.print(": ");
                Serial.println(line);
                allValid = false;
            }
        }
        
        startIndex = endIndex + 1;
        lineNumber++;
    }
    
    // Validate final configuration
    if (!validateParameters()) {
        Serial.println("[ERROR] ConfigParser: Configuration validation failed");
        allValid = false;
    }
    
    return allValid;
}

bool SystemConfig::setParameter(const String& key, const String& value) {
    // Parameter validation ranges
    const uint16_t MIN_LOG_INTERVAL_MS = 10;      // Minimum 10ms (100Hz)
    const uint16_t MAX_LOG_INTERVAL_MS = 1000;    // Maximum 1000ms (1Hz)
    const uint16_t MIN_FLAG_WINDOW_MS = 500;      // Minimum 0.5 seconds
    const uint16_t MAX_FLAG_WINDOW_MS = 10000;    // Maximum 10 seconds
    const float MIN_ML_THRESHOLD = 0.1f;          // Minimum 10% confidence
    const float MAX_ML_THRESHOLD = 1.0f;          // Maximum 100% confidence
    const float MIN_APPROACH_THRESHOLD = 50.0f;   // Minimum 50 mm/s
    const float MAX_APPROACH_THRESHOLD = 2000.0f; // Maximum 2000 mm/s
    const float MIN_JERK_THRESHOLD = 0.5f;        // Minimum 0.5 m/s³
    const float MAX_JERK_THRESHOLD = 20.0f;       // Maximum 20 m/s³
    
    if (key == "log_interval_ms") {
        long val = value.toInt();
        if (val >= MIN_LOG_INTERVAL_MS && val <= MAX_LOG_INTERVAL_MS) {
            log_interval_ms = (uint16_t)val;
            return true;
        } else {
            Serial.print("[WARN] ConfigParser: log_interval_ms out of range (");
            Serial.print(MIN_LOG_INTERVAL_MS);
            Serial.print("-");
            Serial.print(MAX_LOG_INTERVAL_MS);
            Serial.print("): ");
            Serial.println(val);
        }
    }
    else if (key == "flag_window_ms") {
        long val = value.toInt();
        if (val >= MIN_FLAG_WINDOW_MS && val <= MAX_FLAG_WINDOW_MS) {
            flag_window_ms = (uint16_t)val;
            return true;
        } else {
            Serial.print("[WARN] ConfigParser: flag_window_ms out of range (");
            Serial.print(MIN_FLAG_WINDOW_MS);
            Serial.print("-");
            Serial.print(MAX_FLAG_WINDOW_MS);
            Serial.print("): ");
            Serial.println(val);
        }
    }
    else if (key == "ml_threshold") {
        float val = value.toFloat();
        if (val >= MIN_ML_THRESHOLD && val <= MAX_ML_THRESHOLD) {
            ml_threshold = val;
            return true;
        } else {
            Serial.print("[WARN] ConfigParser: ml_threshold out of range (");
            Serial.print(MIN_ML_THRESHOLD, 2);
            Serial.print("-");
            Serial.print(MAX_ML_THRESHOLD, 2);
            Serial.print("): ");
            Serial.println(val, 2);
        }
    }
    else if (key == "approach_threshold_mm_s") {
        float val = value.toFloat();
        if (val >= MIN_APPROACH_THRESHOLD && val <= MAX_APPROACH_THRESHOLD) {
            approach_threshold_mm_s = val;
            return true;
        } else {
            Serial.print("[WARN] ConfigParser: approach_threshold_mm_s out of range (");
            Serial.print(MIN_APPROACH_THRESHOLD, 1);
            Serial.print("-");
            Serial.print(MAX_APPROACH_THRESHOLD, 1);
            Serial.print("): ");
            Serial.println(val, 1);
        }
    }
    else if (key == "jerk_threshold_m_s3") {
        float val = value.toFloat();
        if (val >= MIN_JERK_THRESHOLD && val <= MAX_JERK_THRESHOLD) {
            jerk_threshold_m_s3 = val;
            return true;
        } else {
            Serial.print("[WARN] ConfigParser: jerk_threshold_m_s3 out of range (");
            Serial.print(MIN_JERK_THRESHOLD, 1);
            Serial.print("-");
            Serial.print(MAX_JERK_THRESHOLD, 1);
            Serial.print("): ");
            Serial.println(val, 1);
        }
    }
    else {
        Serial.print("[WARN] ConfigParser: Unknown parameter: ");
        Serial.println(key);
    }
    
    return false;
}

bool SystemConfig::validateParameters() const {
    // Same validation ranges as in setParameter
    const uint16_t MIN_LOG_INTERVAL_MS = 10;
    const uint16_t MAX_LOG_INTERVAL_MS = 1000;
    const uint16_t MIN_FLAG_WINDOW_MS = 500;
    const uint16_t MAX_FLAG_WINDOW_MS = 10000;
    const float MIN_ML_THRESHOLD = 0.1f;
    const float MAX_ML_THRESHOLD = 1.0f;
    const float MIN_APPROACH_THRESHOLD = 50.0f;
    const float MAX_APPROACH_THRESHOLD = 2000.0f;
    const float MIN_JERK_THRESHOLD = 0.5f;
    const float MAX_JERK_THRESHOLD = 20.0f;
    
    bool valid = true;
    
    if (log_interval_ms < MIN_LOG_INTERVAL_MS || log_interval_ms > MAX_LOG_INTERVAL_MS) {
        Serial.println("[ERROR] ConfigParser: log_interval_ms validation failed");
        valid = false;
    }
    
    if (flag_window_ms < MIN_FLAG_WINDOW_MS || flag_window_ms > MAX_FLAG_WINDOW_MS) {
        Serial.println("[ERROR] ConfigParser: flag_window_ms validation failed");
        valid = false;
    }
    
    if (ml_threshold < MIN_ML_THRESHOLD || ml_threshold > MAX_ML_THRESHOLD) {
        Serial.println("[ERROR] ConfigParser: ml_threshold validation failed");
        valid = false;
    }
    
    if (approach_threshold_mm_s < MIN_APPROACH_THRESHOLD || approach_threshold_mm_s > MAX_APPROACH_THRESHOLD) {
        Serial.println("[ERROR] ConfigParser: approach_threshold_mm_s validation failed");
        valid = false;
    }
    
    if (jerk_threshold_m_s3 < MIN_JERK_THRESHOLD || jerk_threshold_m_s3 > MAX_JERK_THRESHOLD) {
        Serial.println("[ERROR] ConfigParser: jerk_threshold_m_s3 validation failed");
        valid = false;
    }
    
    return valid;
}

String SystemConfig::trim(const String& str) const {
    int start = 0;
    int end = str.length() - 1;
    
    // Find first non-whitespace character
    while (start <= end && (str.charAt(start) == ' ' || str.charAt(start) == '\t' || str.charAt(start) == '\r')) {
        start++;
    }
    
    // Find last non-whitespace character
    while (end >= start && (str.charAt(end) == ' ' || str.charAt(end) == '\t' || str.charAt(end) == '\r')) {
        end--;
    }
    
    if (start > end) {
        return "";
    }
    
    return str.substring(start, end + 1);
}