/**
 * test_config_properties.cpp
 * 
 * Property-based tests for configuration parsing
 * Tests Properties 50 and 51 from the design document
 * 
 * Feature: near-miss-detection-ml
 * Property 50: Configuration parameter application
 * Property 51: Configuration fallback
 * Validates: Requirements 12.2, 12.3
 */

#include <Arduino.h>
#include <unity.h>
#include "../include/DataModels.h"

// =============================================================================
// Test Utilities
// =============================================================================

/**
 * Generate random integer within range [min, max]
 */
int randomInt(int min, int max) {
    return min + (random() % (max - min + 1));
}

/**
 * Generate random float within range [min, max]
 */
float randomFloat(float min, float max) {
    float scale = random() / (float)RAND_MAX;
    return min + scale * (max - min);
}

/**
 * Create a valid config string with random valid parameters
 */
String generateValidConfigString() {
    String config = "";
    
    // Generate random valid values within acceptable ranges
    uint16_t log_interval = randomInt(10, 1000);
    uint16_t flag_window = randomInt(500, 10000);
    float ml_threshold = randomFloat(0.1f, 1.0f);
    float approach_threshold = randomFloat(50.0f, 2000.0f);
    float jerk_threshold = randomFloat(0.5f, 20.0f);
    
    config += "log_interval_ms=" + String(log_interval) + "\n";
    config += "flag_window_ms=" + String(flag_window) + "\n";
    config += "ml_threshold=" + String(ml_threshold, 2) + "\n";
    config += "approach_threshold_mm_s=" + String(approach_threshold, 1) + "\n";
    config += "jerk_threshold_m_s3=" + String(jerk_threshold, 1) + "\n";
    
    return config;
}

/**
 * Create a config string with specific values
 */
String createConfigString(uint16_t log_interval, uint16_t flag_window, 
                         float ml_threshold, float approach_threshold, 
                         float jerk_threshold) {
    String config = "";
    config += "log_interval_ms=" + String(log_interval) + "\n";
    config += "flag_window_ms=" + String(flag_window) + "\n";
    config += "ml_threshold=" + String(ml_threshold, 2) + "\n";
    config += "approach_threshold_mm_s=" + String(approach_threshold, 1) + "\n";
    config += "jerk_threshold_m_s3=" + String(jerk_threshold, 1) + "\n";
    return config;
}

/**
 * Create an invalid config string (out of range values)
 */
String generateInvalidConfigString() {
    String config = "";
    
    // Generate values outside acceptable ranges
    int choice = randomInt(0, 4);
    
    switch (choice) {
        case 0:
            // Invalid log_interval_ms (too small or too large)
            config += "log_interval_ms=" + String(randomInt(1, 5)) + "\n";
            break;
        case 1:
            // Invalid flag_window_ms (too small)
            config += "flag_window_ms=" + String(randomInt(10, 400)) + "\n";
            break;
        case 2:
            // Invalid ml_threshold (out of range)
            config += "ml_threshold=" + String(randomFloat(1.1f, 2.0f), 2) + "\n";
            break;
        case 3:
            // Invalid approach_threshold (too small)
            config += "approach_threshold_mm_s=" + String(randomFloat(1.0f, 40.0f), 1) + "\n";
            break;
        case 4:
            // Invalid jerk_threshold (too large)
            config += "jerk_threshold_m_s3=" + String(randomFloat(25.0f, 50.0f), 1) + "\n";
            break;
    }
    
    return config;
}

/**
 * Create a malformed config string (syntax errors)
 */
String generateMalformedConfigString() {
    String config = "";
    
    int choice = randomInt(0, 3);
    
    switch (choice) {
        case 0:
            // Missing equals sign
            config += "log_interval_ms 16\n";
            break;
        case 1:
            // Missing value
            config += "flag_window_ms=\n";
            break;
        case 2:
            // Invalid key
            config += "invalid_parameter=100\n";
            break;
        case 3:
            // Random text
            config += "this is not a valid config line\n";
            break;
    }
    
    return config;
}

// =============================================================================
// Property 50: Configuration parameter application
// Feature: near-miss-detection-ml, Property 50
// Validates: Requirements 12.2
// =============================================================================

/**
 * Property 50: Configuration parameter application
 * For any valid config string, all specified parameters should be applied to system settings
 */
void property_50_config_parameter_application() {
    const int NUM_ITERATIONS = 100;
    int passed = 0;
    int failed = 0;
    
    Serial.println("\n=== Property 50: Configuration parameter application ===");
    Serial.println("Testing that valid config parameters are correctly applied...");
    
    for (int i = 0; i < NUM_ITERATIONS; i++) {
        // Generate random valid configuration values
        uint16_t expected_log_interval = randomInt(10, 1000);
        uint16_t expected_flag_window = randomInt(500, 10000);
        float expected_ml_threshold = randomFloat(0.1f, 1.0f);
        float expected_approach_threshold = randomFloat(50.0f, 2000.0f);
        float expected_jerk_threshold = randomFloat(0.5f, 20.0f);
        
        // Create config string with these values
        String configString = createConfigString(
            expected_log_interval,
            expected_flag_window,
            expected_ml_threshold,
            expected_approach_threshold,
            expected_jerk_threshold
        );
        
        // Parse configuration
        SystemConfig config;
        bool parseResult = config.parseFromString(configString);
        
        // Verify parsing succeeded
        if (!parseResult) {
            Serial.print("Iteration "); Serial.print(i);
            Serial.println(": FAILED - Valid config failed to parse");
            Serial.println("Config string:");
            Serial.println(configString);
            failed++;
            continue;
        }
        
        // Verify all parameters were applied correctly
        bool allCorrect = true;
        
        if (config.log_interval_ms != expected_log_interval) {
            Serial.print("Iteration "); Serial.print(i);
            Serial.print(": FAILED - log_interval_ms mismatch. Expected: ");
            Serial.print(expected_log_interval);
            Serial.print(", Got: ");
            Serial.println(config.log_interval_ms);
            allCorrect = false;
        }
        
        if (config.flag_window_ms != expected_flag_window) {
            Serial.print("Iteration "); Serial.print(i);
            Serial.print(": FAILED - flag_window_ms mismatch. Expected: ");
            Serial.print(expected_flag_window);
            Serial.print(", Got: ");
            Serial.println(config.flag_window_ms);
            allCorrect = false;
        }
        
        if (abs(config.ml_threshold - expected_ml_threshold) > 0.01f) {
            Serial.print("Iteration "); Serial.print(i);
            Serial.print(": FAILED - ml_threshold mismatch. Expected: ");
            Serial.print(expected_ml_threshold, 2);
            Serial.print(", Got: ");
            Serial.println(config.ml_threshold, 2);
            allCorrect = false;
        }
        
        if (abs(config.approach_threshold_mm_s - expected_approach_threshold) > 0.1f) {
            Serial.print("Iteration "); Serial.print(i);
            Serial.print(": FAILED - approach_threshold_mm_s mismatch. Expected: ");
            Serial.print(expected_approach_threshold, 1);
            Serial.print(", Got: ");
            Serial.println(config.approach_threshold_mm_s, 1);
            allCorrect = false;
        }
        
        if (abs(config.jerk_threshold_m_s3 - expected_jerk_threshold) > 0.1f) {
            Serial.print("Iteration "); Serial.print(i);
            Serial.print(": FAILED - jerk_threshold_m_s3 mismatch. Expected: ");
            Serial.print(expected_jerk_threshold, 1);
            Serial.print(", Got: ");
            Serial.println(config.jerk_threshold_m_s3, 1);
            allCorrect = false;
        }
        
        if (allCorrect) {
            passed++;
        } else {
            failed++;
        }
    }
    
    Serial.print("Property 50 Results: ");
    Serial.print(passed);
    Serial.print(" passed, ");
    Serial.print(failed);
    Serial.println(" failed");
    
    // Property test passes if all iterations passed
    TEST_ASSERT_EQUAL_INT(NUM_ITERATIONS, passed);
}

// =============================================================================
// Property 51: Configuration fallback
// Feature: near-miss-detection-ml, Property 51
// Validates: Requirements 12.3
// =============================================================================

/**
 * Property 51: Configuration fallback
 * For any missing or invalid config, default values should be used and a warning should be logged
 */
void property_51_config_fallback() {
    const int NUM_ITERATIONS = 100;
    int passed = 0;
    int failed = 0;
    
    Serial.println("\n=== Property 51: Configuration fallback ===");
    Serial.println("Testing that invalid/missing config falls back to defaults...");
    
    for (int i = 0; i < NUM_ITERATIONS; i++) {
        SystemConfig config;
        
        // Test different types of invalid configurations
        int testType = randomInt(0, 2);
        String configString;
        
        switch (testType) {
            case 0:
                // Empty config string (missing config)
                configString = "";
                break;
            case 1:
                // Invalid parameter values (out of range)
                configString = generateInvalidConfigString();
                break;
            case 2:
                // Malformed config (syntax errors)
                configString = generateMalformedConfigString();
                break;
        }
        
        // Parse configuration (should handle gracefully)
        bool parseResult = config.parseFromString(configString);
        
        // For invalid/missing config, parsing may return false but defaults should be applied
        // Check that configuration has valid default values
        bool hasValidDefaults = true;
        
        // Default values from SystemConfig::applyDefaults()
        const uint16_t DEFAULT_LOG_INTERVAL = 16;
        const uint16_t DEFAULT_FLAG_WINDOW = 2000;
        const float DEFAULT_ML_THRESHOLD = 0.70f;
        const float DEFAULT_APPROACH_THRESHOLD = 500.0f;
        const float DEFAULT_JERK_THRESHOLD = 2.0f;
        
        // For empty config, all should be defaults
        if (configString.length() == 0) {
            if (config.log_interval_ms != DEFAULT_LOG_INTERVAL) {
                Serial.print("Iteration "); Serial.print(i);
                Serial.print(": FAILED - Empty config should have default log_interval_ms (");
                Serial.print(DEFAULT_LOG_INTERVAL);
                Serial.print("), got: ");
                Serial.println(config.log_interval_ms);
                hasValidDefaults = false;
            }
            
            if (config.flag_window_ms != DEFAULT_FLAG_WINDOW) {
                Serial.print("Iteration "); Serial.print(i);
                Serial.print(": FAILED - Empty config should have default flag_window_ms (");
                Serial.print(DEFAULT_FLAG_WINDOW);
                Serial.print("), got: ");
                Serial.println(config.flag_window_ms);
                hasValidDefaults = false;
            }
            
            if (abs(config.ml_threshold - DEFAULT_ML_THRESHOLD) > 0.01f) {
                Serial.print("Iteration "); Serial.print(i);
                Serial.print(": FAILED - Empty config should have default ml_threshold (");
                Serial.print(DEFAULT_ML_THRESHOLD, 2);
                Serial.print("), got: ");
                Serial.println(config.ml_threshold, 2);
                hasValidDefaults = false;
            }
            
            if (abs(config.approach_threshold_mm_s - DEFAULT_APPROACH_THRESHOLD) > 0.1f) {
                Serial.print("Iteration "); Serial.print(i);
                Serial.print(": FAILED - Empty config should have default approach_threshold_mm_s (");
                Serial.print(DEFAULT_APPROACH_THRESHOLD, 1);
                Serial.print("), got: ");
                Serial.println(config.approach_threshold_mm_s, 1);
                hasValidDefaults = false;
            }
            
            if (abs(config.jerk_threshold_m_s3 - DEFAULT_JERK_THRESHOLD) > 0.1f) {
                Serial.print("Iteration "); Serial.print(i);
                Serial.print(": FAILED - Empty config should have default jerk_threshold_m_s3 (");
                Serial.print(DEFAULT_JERK_THRESHOLD, 1);
                Serial.print("), got: ");
                Serial.println(config.jerk_threshold_m_s3, 1);
                hasValidDefaults = false;
            }
        } else {
            // For invalid/malformed config, at least verify values are within valid ranges
            // (defaults should be applied for invalid parameters)
            if (config.log_interval_ms < 10 || config.log_interval_ms > 1000) {
                Serial.print("Iteration "); Serial.print(i);
                Serial.print(": FAILED - log_interval_ms out of valid range: ");
                Serial.println(config.log_interval_ms);
                hasValidDefaults = false;
            }
            
            if (config.flag_window_ms < 500 || config.flag_window_ms > 10000) {
                Serial.print("Iteration "); Serial.print(i);
                Serial.print(": FAILED - flag_window_ms out of valid range: ");
                Serial.println(config.flag_window_ms);
                hasValidDefaults = false;
            }
            
            if (config.ml_threshold < 0.1f || config.ml_threshold > 1.0f) {
                Serial.print("Iteration "); Serial.print(i);
                Serial.print(": FAILED - ml_threshold out of valid range: ");
                Serial.println(config.ml_threshold, 2);
                hasValidDefaults = false;
            }
            
            if (config.approach_threshold_mm_s < 50.0f || config.approach_threshold_mm_s > 2000.0f) {
                Serial.print("Iteration "); Serial.print(i);
                Serial.print(": FAILED - approach_threshold_mm_s out of valid range: ");
                Serial.println(config.approach_threshold_mm_s, 1);
                hasValidDefaults = false;
            }
            
            if (config.jerk_threshold_m_s3 < 0.5f || config.jerk_threshold_m_s3 > 20.0f) {
                Serial.print("Iteration "); Serial.print(i);
                Serial.print(": FAILED - jerk_threshold_m_s3 out of valid range: ");
                Serial.println(config.jerk_threshold_m_s3, 1);
                hasValidDefaults = false;
            }
        }
        
        if (hasValidDefaults) {
            passed++;
        } else {
            failed++;
            Serial.println("Config string that failed:");
            Serial.println(configString);
        }
    }
    
    Serial.print("Property 51 Results: ");
    Serial.print(passed);
    Serial.print(" passed, ");
    Serial.print(failed);
    Serial.println(" failed");
    
    // Property test passes if all iterations passed
    TEST_ASSERT_EQUAL_INT(NUM_ITERATIONS, passed);
}

// =============================================================================
// Additional Edge Case Tests
// =============================================================================

/**
 * Test that comments and whitespace are handled correctly
 */
void test_config_comments_and_whitespace() {
    Serial.println("\n=== Testing comments and whitespace handling ===");
    
    String configString = 
        "# This is a comment\n"
        "  log_interval_ms = 20  \n"  // Leading/trailing whitespace
        "\n"  // Empty line
        "flag_window_ms=3000\n"  // No spaces around =
        "  # Another comment\n"
        "ml_threshold = 0.85\n";
    
    SystemConfig config;
    bool result = config.parseFromString(configString);
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_UINT16(20, config.log_interval_ms);
    TEST_ASSERT_EQUAL_UINT16(3000, config.flag_window_ms);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.85f, config.ml_threshold);
    
    Serial.println("Comments and whitespace test: PASSED");
}

/**
 * Test partial configuration (some parameters missing)
 */
void test_partial_config() {
    Serial.println("\n=== Testing partial configuration ===");
    
    String configString = 
        "log_interval_ms=25\n"
        "ml_threshold=0.60\n";
        // Other parameters missing - should use defaults
    
    SystemConfig config;
    bool result = config.parseFromString(configString);
    
    // Should parse successfully
    TEST_ASSERT_TRUE(result);
    
    // Specified parameters should be applied
    TEST_ASSERT_EQUAL_UINT16(25, config.log_interval_ms);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.60f, config.ml_threshold);
    
    // Missing parameters should have defaults
    TEST_ASSERT_EQUAL_UINT16(2000, config.flag_window_ms);  // Default
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 500.0f, config.approach_threshold_mm_s);  // Default
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 2.0f, config.jerk_threshold_m_s3);  // Default
    
    Serial.println("Partial configuration test: PASSED");
}

// =============================================================================
// Test Runner
// =============================================================================

void setUp(void) {
    // Set up code here (runs before each test)
}

void tearDown(void) {
    // Clean up code here (runs after each test)
}

void setup() {
    // Wait for serial connection
    delay(2000);
    
    Serial.begin(115200);
    Serial.println("\n\n==============================================");
    Serial.println("Configuration Parsing Property-Based Tests");
    Serial.println("==============================================");
    
    // Initialize random seed
    randomSeed(analogRead(0));
    
    UNITY_BEGIN();
    
    // Run property-based tests
    RUN_TEST(property_50_config_parameter_application);
    RUN_TEST(property_51_config_fallback);
    
    // Run edge case tests
    RUN_TEST(test_config_comments_and_whitespace);
    RUN_TEST(test_partial_config);
    
    UNITY_END();
}

void loop() {
    // Nothing to do here
}
