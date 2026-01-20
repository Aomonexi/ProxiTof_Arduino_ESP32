/**
 * buzzer.c
 * 
 * PWM Buzzer Driver Implementation
 * Uses LEDC peripheral for PWM generation
 */

#include "buzzer.h"
#include "driver/ledc.h"
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include <string.h>

static const char *TAG = "BUZZER";

#define LEDC_TIMER              LEDC_TIMER_0
#define LEDC_MODE               LEDC_LOW_SPEED_MODE
#define LEDC_CHANNEL            LEDC_CHANNEL_0
#define LEDC_DUTY_RES           LEDC_TIMER_10_BIT  // 10-bit resolution (0-1023)
#define LEDC_MAX_DUTY           1023

// Buzzer state
static struct {
    bool initialized;
    int gpio_pin;
    int adc_pin;
    bool use_potentiometer;
    adc_oneshot_unit_handle_t adc_handle;
    adc_channel_t adc_channel;
    uint8_t volume_percent;
    uint32_t current_frequency_hz;
    bool is_playing;
    TimerHandle_t pattern_timer;
    buzzer_pattern_t current_pattern;
    int pattern_step;
} buzzer_state = {0};

/**
 * Set PWM duty cycle based on volume
 */
static void buzzer_set_duty(uint8_t volume_percent)
{
    if (volume_percent > 100) {
        volume_percent = 100;
    }
    
    // Calculate duty cycle (0-1023)
    uint32_t duty = (LEDC_MAX_DUTY * volume_percent) / 100;
    
    // Set duty cycle
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, duty);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
    
    buzzer_state.is_playing = (volume_percent > 0);
}

/**
 * Pattern timer callback
 */
static void pattern_timer_callback(TimerHandle_t xTimer)
{
    buzzer_pattern_t pattern = buzzer_state.current_pattern;
    int step = buzzer_state.pattern_step;
    
    switch (pattern) {
        case BUZZER_PATTERN_NEAR_MISS:
            // 3 short beeps: ON(200ms) OFF(100ms) ON(200ms) OFF(100ms) ON(200ms) OFF
            if (step == 0 || step == 2 || step == 4) {
                // Beep ON
                ledc_set_freq(LEDC_MODE, LEDC_TIMER, 2000);  // 2 kHz
                buzzer_set_duty(buzzer_state.volume_percent);
                xTimerChangePeriod(xTimer, pdMS_TO_TICKS(200), 0);
            } else if (step == 1 || step == 3) {
                // Beep OFF
                buzzer_set_duty(0);
                xTimerChangePeriod(xTimer, pdMS_TO_TICKS(100), 0);
            } else {
                // Pattern complete
                buzzer_stop();
                return;
            }
            buzzer_state.pattern_step++;
            break;
            
        case BUZZER_PATTERN_CRITICAL:
            // Continuous tone (no timer needed, just play once)
            ledc_set_freq(LEDC_MODE, LEDC_TIMER, 1000);  // 1 kHz
            buzzer_set_duty(100);  // Maximum volume for critical alert
            xTimerStop(xTimer, 0);
            break;
            
        case BUZZER_PATTERN_BUTTON:
            // Single short beep
            if (step == 0) {
                ledc_set_freq(LEDC_MODE, LEDC_TIMER, 3000);  // 3 kHz
                buzzer_set_duty(buzzer_state.volume_percent);
                xTimerChangePeriod(xTimer, pdMS_TO_TICKS(100), 0);
                buzzer_state.pattern_step++;
            } else {
                buzzer_stop();
            }
            break;
            
        case BUZZER_PATTERN_STARTUP:
            // Startup melody: 3 ascending tones
            if (step == 0) {
                ledc_set_freq(LEDC_MODE, LEDC_TIMER, 1000);
                buzzer_set_duty(buzzer_state.volume_percent);
                xTimerChangePeriod(xTimer, pdMS_TO_TICKS(150), 0);
            } else if (step == 1) {
                ledc_set_freq(LEDC_MODE, LEDC_TIMER, 1500);
                buzzer_set_duty(buzzer_state.volume_percent);
                xTimerChangePeriod(xTimer, pdMS_TO_TICKS(150), 0);
            } else if (step == 2) {
                ledc_set_freq(LEDC_MODE, LEDC_TIMER, 2000);
                buzzer_set_duty(buzzer_state.volume_percent);
                xTimerChangePeriod(xTimer, pdMS_TO_TICKS(150), 0);
            } else {
                buzzer_stop();
                return;
            }
            buzzer_state.pattern_step++;
            break;
            
        case BUZZER_PATTERN_ERROR:
            // Error: 2 low beeps
            if (step == 0 || step == 2) {
                ledc_set_freq(LEDC_MODE, LEDC_TIMER, 500);  // Low frequency
                buzzer_set_duty(buzzer_state.volume_percent);
                xTimerChangePeriod(xTimer, pdMS_TO_TICKS(200), 0);
            } else if (step == 1) {
                buzzer_set_duty(0);
                xTimerChangePeriod(xTimer, pdMS_TO_TICKS(100), 0);
            } else {
                buzzer_stop();
                return;
            }
            buzzer_state.pattern_step++;
            break;
            
        default:
            buzzer_stop();
            break;
    }
}

/**
 * Initialize buzzer
 */
esp_err_t buzzer_init(const buzzer_config_t *config)
{
    if (buzzer_state.initialized) {
        ESP_LOGW(TAG, "Buzzer already initialized");
        return ESP_OK;
    }
    
    if (!config) {
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGI(TAG, "Initializing buzzer on GPIO%d", config->gpio_pin);
    
    // Configure LEDC timer
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_MODE,
        .duty_resolution  = LEDC_DUTY_RES,
        .timer_num        = LEDC_TIMER,
        .freq_hz          = config->default_frequency_hz,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    
    esp_err_t err = ledc_timer_config(&ledc_timer);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure LEDC timer: %s", esp_err_to_name(err));
        return err;
    }
    
    // Configure LEDC channel
    ledc_channel_config_t ledc_channel = {
        .gpio_num       = config->gpio_pin,
        .speed_mode     = LEDC_MODE,
        .channel        = LEDC_CHANNEL,
        .timer_sel      = LEDC_TIMER,
        .duty           = 0,  // Start silent
        .hpoint         = 0
    };
    
    err = ledc_channel_config(&ledc_channel);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure LEDC channel: %s", esp_err_to_name(err));
        return err;
    }
    
    // Initialize ADC for potentiometer (if enabled)
    if (config->use_potentiometer && config->adc_pin >= 0) {
        ESP_LOGI(TAG, "Configuring volume control via potentiometer on GPIO%d", config->adc_pin);
        
        // Configure ADC
        adc_oneshot_unit_init_cfg_t adc_config = {
            .unit_id = ADC_UNIT_2,  // GPIO 46 is on ADC2
        };
        
        err = adc_oneshot_new_unit(&adc_config, &buzzer_state.adc_handle);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "Failed to initialize ADC: %s", esp_err_to_name(err));
            ESP_LOGW(TAG, "Continuing without potentiometer support");
            buzzer_state.use_potentiometer = false;
        } else {
            // Configure ADC channel (GPIO 46 = ADC2_CH5)
            adc_oneshot_chan_cfg_t chan_config = {
                .atten = ADC_ATTEN_DB_12,  // 0-3.3V range
                .bitwidth = ADC_BITWIDTH_12,  // 12-bit resolution (0-4095)
            };
            
            buzzer_state.adc_channel = ADC_CHANNEL_5;  // GPIO 46
            err = adc_oneshot_config_channel(buzzer_state.adc_handle, 
                                             buzzer_state.adc_channel, &chan_config);
            if (err != ESP_OK) {
                ESP_LOGW(TAG, "Failed to configure ADC channel: %s", esp_err_to_name(err));
                adc_oneshot_del_unit(buzzer_state.adc_handle);
                buzzer_state.use_potentiometer = false;
            } else {
                buzzer_state.use_potentiometer = true;
                buzzer_state.adc_pin = config->adc_pin;
                ESP_LOGI(TAG, "✓ Potentiometer volume control enabled");
            }
        }
    }
    
    // Create pattern timer
    buzzer_state.pattern_timer = xTimerCreate(
        "buzzer_pattern",
        pdMS_TO_TICKS(100),
        pdTRUE,  // Auto-reload
        NULL,
        pattern_timer_callback
    );
    
    if (!buzzer_state.pattern_timer) {
        ESP_LOGE(TAG, "Failed to create pattern timer");
        if (buzzer_state.adc_handle) {
            adc_oneshot_del_unit(buzzer_state.adc_handle);
        }
        return ESP_FAIL;
    }
    
    // Initialize state
    buzzer_state.initialized = true;
    buzzer_state.gpio_pin = config->gpio_pin;
    buzzer_state.volume_percent = config->default_volume_percent;
    buzzer_state.current_frequency_hz = config->default_frequency_hz;
    buzzer_state.is_playing = false;
    
    ESP_LOGI(TAG, "✓ Buzzer initialized (freq=%lu Hz, volume=%d%%)", 
             config->default_frequency_hz, config->default_volume_percent);
    
    return ESP_OK;
}

/**
 * Play alert pattern
 */
esp_err_t buzzer_play_pattern(buzzer_pattern_t pattern)
{
    if (!buzzer_state.initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    // Stop current pattern
    buzzer_stop();
    
    // Start new pattern
    buzzer_state.current_pattern = pattern;
    buzzer_state.pattern_step = 0;
    
    // Start pattern timer
    xTimerStart(buzzer_state.pattern_timer, 0);
    
    // Trigger first step immediately
    pattern_timer_callback(buzzer_state.pattern_timer);
    
    return ESP_OK;
}

/**
 * Play tone at specific frequency
 */
esp_err_t buzzer_play_tone(uint32_t frequency_hz, uint32_t duration_ms)
{
    if (!buzzer_state.initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    // Stop current pattern
    buzzer_stop();
    
    // Set frequency
    ledc_set_freq(LEDC_MODE, LEDC_TIMER, frequency_hz);
    buzzer_state.current_frequency_hz = frequency_hz;
    
    // Start tone
    buzzer_set_duty(buzzer_state.volume_percent);
    
    // Schedule stop if duration specified
    if (duration_ms > 0) {
        buzzer_state.current_pattern = BUZZER_PATTERN_NONE;
        buzzer_state.pattern_step = 0;
        xTimerChangePeriod(buzzer_state.pattern_timer, pdMS_TO_TICKS(duration_ms), 0);
        xTimerStart(buzzer_state.pattern_timer, 0);
    }
    
    return ESP_OK;
}

/**
 * Stop buzzer immediately
 */
void buzzer_stop(void)
{
    if (!buzzer_state.initialized) {
        return;
    }
    
    // Stop timer
    xTimerStop(buzzer_state.pattern_timer, 0);
    
    // Silence buzzer
    buzzer_set_duty(0);
    
    buzzer_state.is_playing = false;
    buzzer_state.current_pattern = BUZZER_PATTERN_NONE;
}

/**
 * Set volume
 */
esp_err_t buzzer_set_volume(uint8_t volume_percent)
{
    if (!buzzer_state.initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    if (volume_percent > 100) {
        volume_percent = 100;
    }
    
    buzzer_state.volume_percent = volume_percent;
    
    // Update duty cycle if currently playing
    if (buzzer_state.is_playing) {
        buzzer_set_duty(volume_percent);
    }
    
    return ESP_OK;
}

/**
 * Get current volume
 */
uint8_t buzzer_get_volume(void)
{
    return buzzer_state.volume_percent;
}

/**
 * Read potentiometer and update volume
 */
esp_err_t buzzer_update_volume_from_pot(void)
{
    if (!buzzer_state.initialized || !buzzer_state.use_potentiometer) {
        return ESP_ERR_INVALID_STATE;
    }
    
    // Read ADC value
    int adc_value = 0;
    esp_err_t err = adc_oneshot_read(buzzer_state.adc_handle, 
                                     buzzer_state.adc_channel, &adc_value);
    if (err != ESP_OK) {
        return err;
    }
    
    // Convert to volume percentage (0-4095 → 0-100%)
    uint8_t new_volume = (adc_value * 100) / 4095;
    
    // Update volume if changed significantly (>5% difference)
    if (abs(new_volume - buzzer_state.volume_percent) > 5) {
        buzzer_set_volume(new_volume);
    }
    
    return ESP_OK;
}

/**
 * Check if buzzer is currently playing
 */
bool buzzer_is_playing(void)
{
    return buzzer_state.is_playing;
}

/**
 * Deinitialize buzzer
 */
void buzzer_deinit(void)
{
    if (!buzzer_state.initialized) {
        return;
    }
    
    // Stop buzzer
    buzzer_stop();
    
    // Delete timer
    if (buzzer_state.pattern_timer) {
        xTimerDelete(buzzer_state.pattern_timer, 0);
        buzzer_state.pattern_timer = NULL;
    }
    
    // Delete ADC
    if (buzzer_state.adc_handle) {
        adc_oneshot_del_unit(buzzer_state.adc_handle);
        buzzer_state.adc_handle = NULL;
    }
    
    // Stop LEDC
    ledc_stop(LEDC_MODE, LEDC_CHANNEL, 0);
    
    buzzer_state.initialized = false;
    ESP_LOGI(TAG, "Buzzer deinitialized");
}
