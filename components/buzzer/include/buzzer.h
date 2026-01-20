/**
 * buzzer.h
 * 
 * PWM Buzzer Driver for Audio Alerts
 * Supports both active and passive buzzers
 */

#ifndef BUZZER_H
#define BUZZER_H

#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Buzzer alert patterns
 */
typedef enum {
    BUZZER_PATTERN_NONE = 0,           // Silent
    BUZZER_PATTERN_NEAR_MISS,          // 3 short beeps (near-miss detected)
    BUZZER_PATTERN_CRITICAL,           // Continuous tone (critical proximity)
    BUZZER_PATTERN_BUTTON,             // Single short beep (button press)
    BUZZER_PATTERN_STARTUP,            // Startup melody
    BUZZER_PATTERN_ERROR               // Error tone
} buzzer_pattern_t;

/**
 * Buzzer configuration
 */
typedef struct {
    int gpio_pin;                      // GPIO pin for buzzer
    int adc_pin;                       // ADC pin for potentiometer (-1 if not used)
    uint32_t default_frequency_hz;     // Default frequency (2000 Hz recommended)
    uint8_t default_volume_percent;    // Default volume 0-100% (50% recommended)
    bool use_potentiometer;            // Enable volume control via potentiometer
} buzzer_config_t;

/**
 * Initialize buzzer
 * 
 * @param config Buzzer configuration
 * @return ESP_OK on success
 */
esp_err_t buzzer_init(const buzzer_config_t *config);

/**
 * Play alert pattern
 * Non-blocking - pattern plays in background
 * 
 * @param pattern Alert pattern to play
 * @return ESP_OK on success
 */
esp_err_t buzzer_play_pattern(buzzer_pattern_t pattern);

/**
 * Play tone at specific frequency
 * 
 * @param frequency_hz Frequency in Hz (100-10000)
 * @param duration_ms Duration in milliseconds (0 = continuous)
 * @return ESP_OK on success
 */
esp_err_t buzzer_play_tone(uint32_t frequency_hz, uint32_t duration_ms);

/**
 * Stop buzzer immediately
 */
void buzzer_stop(void);

/**
 * Set volume (0-100%)
 * 
 * @param volume_percent Volume percentage (0 = silent, 100 = max)
 * @return ESP_OK on success
 */
esp_err_t buzzer_set_volume(uint8_t volume_percent);

/**
 * Get current volume
 * 
 * @return Volume percentage (0-100)
 */
uint8_t buzzer_get_volume(void);

/**
 * Read potentiometer and update volume
 * Call this periodically if using potentiometer
 * 
 * @return ESP_OK on success
 */
esp_err_t buzzer_update_volume_from_pot(void);

/**
 * Check if buzzer is currently playing
 * 
 * @return true if buzzer is active
 */
bool buzzer_is_playing(void);

/**
 * Deinitialize buzzer
 */
void buzzer_deinit(void);

#ifdef __cplusplus
}
#endif

#endif // BUZZER_H
