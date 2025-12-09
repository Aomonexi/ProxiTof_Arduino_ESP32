/**
 * ConfigParser.h
 * 
 * Configuration file parser for ProxiTof system
 */

#ifndef CONFIG_PARSER_H
#define CONFIG_PARSER_H

#include <Arduino.h>
#include "DataModels.h"

// Configuration validation constants
#define MIN_LOG_INTERVAL_MS 10
#define MAX_LOG_INTERVAL_MS 10000
#define MIN_FLAG_WINDOW_MS 100
#define MAX_FLAG_WINDOW_MS 10000
#define MIN_ML_THRESHOLD 0.0f
#define MAX_ML_THRESHOLD 1.0f
#define MIN_APPROACH_THRESHOLD 0.0f
#define MAX_APPROACH_THRESHOLD 10000.0f
#define MIN_JERK_THRESHOLD 0.0f
#define MAX_JERK_THRESHOLD 100.0f

class ConfigParser {
public:
    /**
     * Parse configuration file from SD card
     * @param config SystemConfig struct to populate
     * @param filename Path to config file on SD card
     * @return true if parsing succeeded, false otherwise
     */
    static bool parseConfigFile(SystemConfig& config, const char* filename);
    
    /**
     * Parse configuration from string content
     * @param config SystemConfig struct to populate
     * @param content Configuration file content as string
     * @return true if all parameters valid, false if any invalid
     */
    static bool parseConfigString(SystemConfig& config, const String& content);
    
private:
    /**
     * Set a single configuration parameter
     * @param config SystemConfig struct to update
     * @param key Parameter name
     * @param value Parameter value as string
     * @return true if parameter valid and set, false otherwise
     */
    static bool setConfigParameter(SystemConfig& config, const String& key, const String& value);
    
    /**
     * Validate entire configuration
     * @param config SystemConfig struct to validate
     * @return true if all parameters within valid ranges
     */
    static bool validateConfig(const SystemConfig& config);
    
    /**
     * Trim whitespace from string
     * @param str String to trim
     * @return Trimmed string
     */
    static String trim(const String& str);
};

#endif // CONFIG_PARSER_H
