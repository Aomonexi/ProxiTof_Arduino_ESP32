/**
 * oled_display.c
 * 
 * OLED Display Driver - SPI Mode with nopnop2002 library
 * Features: Dual-color zones, bar graph ToF visualization, multiple layouts
 */

#include "oled_display.h"
#include "proxitof_logo.h"
#include "ssd1306.h"
#include "font8x8_basic.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "smart_time.h"
#include "storage_manager.h"
#include "gps.h"
#include "sensor_manager.h"
#include "config_manager.h"
#include "usb_flash_partition.h"  // For USB MSC functions
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <time.h>

static const char *TAG = "OLED_SPI";

// GPIO Pin Definitions
#define OLED_PIN_SCK    GPIO_NUM_7   // D0 on display (Clock)
#define OLED_PIN_MOSI   GPIO_NUM_1   // D1 on display (Data/MOSI)
#define OLED_PIN_DC     GPIO_NUM_35  // D/C on display (Data/Command)
#define OLED_PIN_RST    GPIO_NUM_21  // RES on display (Reset)
#define OLED_PIN_CS     -1           // No CS pin on your display

// Display dimensions
#define OLED_WIDTH      128
#define OLED_HEIGHT     64

// Color zones
#define YELLOW_ZONE_HEIGHT  16  // Top 16 pixels (pages 0-1)
#define BLUE_ZONE_HEIGHT    48  // Bottom 48 pixels (pages 2-7)

// Frame buffer for differential updates (eliminates flicker)
// 128x64 display = 8 pages × 128 bytes = 1024 bytes
#define FRAME_BUFFER_SIZE 1024
static uint8_t frame_buffer[FRAME_BUFFER_SIZE];
static uint8_t prev_frame_buffer[FRAME_BUFFER_SIZE];
static bool frame_buffer_initialized = false;

// Display state
static bool display_initialized = false;
static SSD1306_t dev;
static oled_config_t current_config = {
    .brightness = 128,
    .flip_display = false,
    .mode = OLED_MODE_DASHBOARD
};

// State for jerk calculation (need previous acceleration)
static float prev_accel_x = 0.0f;
static float prev_accel_y = 0.0f;
static float prev_accel_z = 0.0f;
static uint32_t prev_accel_time = 0;
static bool prev_accel_valid = false;

// State for approach rate calculation (need previous distance)
static uint16_t prev_min_distance = 0;
static uint32_t prev_distance_time = 0;
static bool prev_distance_valid = false;

// Near-miss detection state (improved with time-based logic)
typedef struct {
    bool rapid_approach_active;
    uint32_t rapid_approach_start_time;
    uint16_t min_distance_during_approach;
    uint32_t high_jerk_time;
    bool high_jerk_detected;
    float max_jerk_during_approach;
} near_miss_state_t;

static near_miss_state_t nm_state = {0};

// Shutdown confirmation state
static bool shutdown_confirmation_pending = false;
static uint32_t shutdown_confirmation_time = 0;
static bool shutdown_confirmed = false;
static bool near_miss_active = false;
static char near_miss_reasons[4][32] = {"", "", "", ""};
static int near_miss_reason_count = 0;

// Jerk noise filtering (moving average)
#define JERK_HISTORY_SIZE 5
static float jerk_history[JERK_HISTORY_SIZE] = {0};
static int jerk_history_index = 0;

// Volume popup state
static uint32_t volume_popup_last_update = 0;
static uint8_t volume_popup_value = 0;
static bool volume_popup_active = false;

/**
 * Calculate moving average for jerk (noise filtering)
 */
static float calculate_jerk_moving_average(float new_jerk) {
    jerk_history[jerk_history_index] = new_jerk;
    jerk_history_index = (jerk_history_index + 1) % JERK_HISTORY_SIZE;
    
    float sum = 0.0f;
    for (int i = 0; i < JERK_HISTORY_SIZE; i++) {
        sum += jerk_history[i];
    }
    return sum / JERK_HISTORY_SIZE;
}

// Forward declarations
static void draw_volume_popup(void);

/**
 * Clear frame buffer (internal buffer only, doesn't update display)
 */
static void clear_frame_buffer(void) {
    memset(frame_buffer, 0, FRAME_BUFFER_SIZE);
}

/**
 * Write text to frame buffer at specified position
 * @param page Page number (0-7)
 * @param text Text string to write
 * @param invert Invert colors (white on black vs black on white)
 */
static void fb_write_text(int page, const char *text, bool invert) {
    if (page < 0 || page >= 8) return;
    
    int len = strlen(text);
    if (len > 16) len = 16;  // Max 16 characters per line
    
    for (int i = 0; i < len; i++) {
        uint8_t c = text[i];
        if (c < 32 || c > 127) c = 32;  // Replace invalid chars with space
        
        // Get font data for this character
        const uint8_t *font_char = font8x8_basic_tr[c];
        
        // Write 8 bytes (8 pixels wide) to frame buffer
        for (int x = 0; x < 8; x++) {
            int fb_index = page * 128 + i * 8 + x;
            if (fb_index < FRAME_BUFFER_SIZE) {
                frame_buffer[fb_index] = invert ? ~font_char[x] : font_char[x];
            }
        }
    }
}

/**
 * Draw a bar in the frame buffer
 * @param page Page number (0-7)
 * @param x X position in pixels (0-127)
 * @param width Bar width in pixels
 * @param distance Distance value (0-4000mm)
 * @param max_distance Maximum distance (2000mm)
 */
static void fb_draw_bar(int page, int x, int width, uint16_t distance, uint16_t max_distance) {
    if (page < 0 || page >= 8) return;
    
    // Handle invalid ToF readings (sensor returns 8191 for out-of-range)
    if (distance > 4000) {
        distance = max_distance;  // Treat as max distance (empty bar)
    }
    
    // Calculate fill level (closer = fuller bar)
    float fill_percent = 1.0f - ((float)distance / (float)max_distance);
    if (fill_percent < 0.0f) fill_percent = 0.0f;
    if (fill_percent > 1.0f) fill_percent = 1.0f;
    
    int fill_width = (int)(fill_percent * (float)width);
    
    // Draw bar in frame buffer
    for (int i = 0; i < width && (x + i) < 128; i++) {
        int fb_index = page * 128 + x + i;
        if (fb_index < FRAME_BUFFER_SIZE) {
            if (i < fill_width) {
                // Filled portion - solid bar
                frame_buffer[fb_index] = 0xFF;
            } else {
                // Empty portion - just border (top and bottom pixels)
                frame_buffer[fb_index] = 0x81;
            }
        }
    }
}

/**
 * Update display with differential rendering
 * Only sends changed regions to the display (eliminates flicker)
 */
static void update_display_differential(void) {
    if (!frame_buffer_initialized) {
        // First update - send everything
        for (int page = 0; page < 8; page++) {
            ssd1306_display_image(&dev, page, 0, &frame_buffer[page * 128], 128);
        }
        memcpy(prev_frame_buffer, frame_buffer, FRAME_BUFFER_SIZE);
        frame_buffer_initialized = true;
        return;
    }
    
    // Differential update - only send changed pages
    for (int page = 0; page < 8; page++) {
        int page_offset = page * 128;
        bool page_changed = false;
        
        // Check if this page has any changes
        for (int x = 0; x < 128; x++) {
            if (frame_buffer[page_offset + x] != prev_frame_buffer[page_offset + x]) {
                page_changed = true;
                break;
            }
        }
        
        // Only update changed pages
        if (page_changed) {
            ssd1306_display_image(&dev, page, 0, &frame_buffer[page_offset], 128);
            memcpy(&prev_frame_buffer[page_offset], &frame_buffer[page_offset], 128);
        }
    }
}

esp_err_t oled_display_init(void) {
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "╔════════════════════════════════════════════╗");
    ESP_LOGI(TAG, "║   OLED Display Initialization (SPI)       ║");
    ESP_LOGI(TAG, "╚════════════════════════════════════════════╝");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "Display: 128x64 OLED SSD1306 (4-wire SPI)");
    ESP_LOGI(TAG, "Dual-color: Yellow (top 16px) + Blue (bottom 48px)");
    ESP_LOGI(TAG, "Library: nopnop2002/esp_idf_ssd1306");
    ESP_LOGI(TAG, "");
    
    // Initialize SPI interface
    ESP_LOGI(TAG, "Initializing SPI interface...");
    // Note: Using SPI3 (forced in ssd1306_spi.c) to avoid conflict with IMU on SPI2
    // Function signature: spi_master_init(dev, mosi, sclk, cs, dc, reset)
    spi_master_init(&dev, OLED_PIN_MOSI, OLED_PIN_SCK, OLED_PIN_CS, OLED_PIN_DC, OLED_PIN_RST);
    
    // Initialize display
    ESP_LOGI(TAG, "Initializing SSD1306 display...");
    ssd1306_init(&dev, OLED_WIDTH, OLED_HEIGHT);
    
    // Clear display
    ssd1306_clear_screen(&dev, false);
    
    // Initialize frame buffers
    clear_frame_buffer();
    memset(prev_frame_buffer, 0, FRAME_BUFFER_SIZE);
    frame_buffer_initialized = false;
    
    // Display startup message
    ssd1306_display_text(&dev, 0, "  ProxiTof v1.0", 16, false);
    ssd1306_display_text(&dev, 2, "  Initializing...", 17, false);
    
    display_initialized = true;
    
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "╔════════════════════════════════════════════╗");
    ESP_LOGI(TAG, "║   OLED Display Ready!                      ║");
    ESP_LOGI(TAG, "╚════════════════════════════════════════════╝");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "✓ Display initialized successfully");
    ESP_LOGI(TAG, "✓ Yellow zone: Top 16 pixels (alerts)");
    ESP_LOGI(TAG, "✓ Blue zone: Bottom 48 pixels (data)");
    ESP_LOGI(TAG, "✓ Bar graph visualization enabled");
    ESP_LOGI(TAG, "");
    
    return ESP_OK;
}

esp_err_t oled_display_clear(void) {
    if (!display_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    clear_frame_buffer();
    ssd1306_clear_screen(&dev, false);
    return ESP_OK;
}

esp_err_t oled_display_update(const tof_data_t *tof_data, 
                              const imu_data_t *imu_data, 
                              uint16_t min_distance) {
    if (!display_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    // Calculate derived metrics
    float jerk_magnitude = 0.0f;
    float jerk_filtered = 0.0f;
    float approach_rate = 0.0f;
    float angular_velocity = 0.0f;
    
    // Calculate jerk (derivative of acceleration)
    if (prev_accel_valid && imu_data->valid) {
        uint32_t delta_time = imu_data->timestamp_ms - prev_accel_time;
        if (delta_time > 0) {
            float delta_ax = imu_data->accel_x - prev_accel_x;
            float delta_ay = imu_data->accel_y - prev_accel_y;
            float delta_az = imu_data->accel_z - prev_accel_z;
            float delta_t_sec = delta_time / 1000.0f;
            
            float jerk_x = delta_ax / delta_t_sec;
            float jerk_y = delta_ay / delta_t_sec;
            float jerk_z = delta_az / delta_t_sec;
            
            jerk_magnitude = sqrtf(jerk_x * jerk_x + jerk_y * jerk_y + jerk_z * jerk_z);
            
            // Apply moving average filter to reduce noise
            jerk_filtered = calculate_jerk_moving_average(jerk_magnitude);
        }
    }
    
    // Update previous acceleration
    if (imu_data->valid) {
        prev_accel_x = imu_data->accel_x;
        prev_accel_y = imu_data->accel_y;
        prev_accel_z = imu_data->accel_z;
        prev_accel_time = imu_data->timestamp_ms;
        prev_accel_valid = true;
    }
    
    // Calculate approach rate (negative = approaching)
    if (prev_distance_valid && tof_data->valid) {
        uint32_t delta_time = tof_data->timestamp_ms - prev_distance_time;
        if (delta_time > 0) {
            float delta_dist = (float)min_distance - (float)prev_min_distance;
            float delta_t_sec = delta_time / 1000.0f;
            approach_rate = delta_dist / delta_t_sec;  // mm/s (negative = approaching)
        }
    }
    
    // Update previous distance
    if (tof_data->valid) {
        prev_min_distance = min_distance;
        prev_distance_time = tof_data->timestamp_ms;
        prev_distance_valid = true;
    }
    
    // Calculate angular velocity magnitude
    if (imu_data->valid) {
        angular_velocity = sqrtf(imu_data->gyro_x * imu_data->gyro_x +
                                imu_data->gyro_y * imu_data->gyro_y +
                                imu_data->gyro_z * imu_data->gyro_z);
    }
    
    // Improved near-miss detection with time-based state machine
    uint32_t now = tof_data->timestamp_ms;
    near_miss_reason_count = 0;
    
    // State 1: Detect rapid approach START
    if (approach_rate < -800.0f && !nm_state.rapid_approach_active) {
        nm_state.rapid_approach_active = true;
        nm_state.rapid_approach_start_time = now;
        nm_state.min_distance_during_approach = min_distance;
        nm_state.high_jerk_detected = false;
        nm_state.max_jerk_during_approach = 0.0f;
    }
    
    // State 2: Track minimum distance and jerk during approach
    if (nm_state.rapid_approach_active) {
        // Update minimum distance
        if (min_distance < nm_state.min_distance_during_approach) {
            nm_state.min_distance_during_approach = min_distance;
        }
        
        // Detect high jerk during approach (filtered to reduce false positives)
        if (jerk_filtered > 50.0f) {
            nm_state.high_jerk_detected = true;
            nm_state.high_jerk_time = now;
            if (jerk_filtered > nm_state.max_jerk_during_approach) {
                nm_state.max_jerk_during_approach = jerk_filtered;
            }
        }
        
        // Check approach duration
        uint32_t approach_duration = now - nm_state.rapid_approach_start_time;
        
        // State 3: Evaluate near-miss conditions
        near_miss_active = false;
        
        // Condition 1: Critical proximity reached during approach
        if (nm_state.min_distance_during_approach < 200) {
            snprintf(near_miss_reasons[near_miss_reason_count++], 32, 
                    "CRITICAL: %dmm", nm_state.min_distance_during_approach);
            near_miss_active = true;
        }
        
        // Condition 2: Rapid approach sustained (0.5-5 seconds)
        if (approach_duration > 500 && approach_duration < 5000 && 
            approach_rate < -800.0f) {
            snprintf(near_miss_reasons[near_miss_reason_count++], 32, 
                    "RAPID: %.0fmm/s", -approach_rate);
            near_miss_active = true;
        }
        
        // Condition 3: High jerk during approach (sudden stop/dodge)
        if (nm_state.high_jerk_detected) {
            uint32_t time_since_jerk = now - nm_state.high_jerk_time;
            if (time_since_jerk < 2000) {  // Within 2 seconds
                snprintf(near_miss_reasons[near_miss_reason_count++], 32, 
                        "JERK: %.1fm/s3", nm_state.max_jerk_during_approach);
                near_miss_active = true;
            }
        }
        
        // Condition 4: High rotation during approach (evasive maneuver)
        if (angular_velocity > 300.0f) {
            snprintf(near_miss_reasons[near_miss_reason_count++], 32, 
                    "SPIN: %.0fdeg/s", angular_velocity);
            near_miss_active = true;
        }
        
        // State 4: Reset if approach ended (slowed down or moving away)
        if (approach_rate > -100.0f || approach_duration > 5000) {
            nm_state.rapid_approach_active = false;
        }
    }
    
    // Standalone critical proximity check (even without approach)
    if (!nm_state.rapid_approach_active && min_distance < 150) {
        near_miss_active = true;
        near_miss_reason_count = 0;
        snprintf(near_miss_reasons[near_miss_reason_count++], 32, 
                "CRITICAL: %dmm", min_distance);
    }
    
    // Clear frame buffer (not the display - eliminates flicker!)
    clear_frame_buffer();
    
    switch (current_config.mode) {
        case OLED_MODE_DASHBOARD:
            // ===== YELLOW ZONE (Top 16 pixels) - Status Bar =====
            {
                // Get current time
                struct tm timeinfo;
                char time_str[16] = "??:??:??";
                if (smart_time_get(&timeinfo) == ESP_OK) {
                    strftime(time_str, sizeof(time_str), "%H:%M:%S", &timeinfo);
                }
                
                // Get logging status
                storage_stats_t storage_stats;
                storage_manager_get_stats(&storage_stats);
                const char *log_status = storage_stats.logging_active ? "LOG" : "---";
                
                // Get GPS status
                gps_data_t gps_data;
                const char *gps_status = "GPS";
                if (gps_read(&gps_data) == ESP_OK && gps_data.valid) {
                    if (gps_data.satellites_used >= 4) {
                        gps_status = "3D";  // 3D fix
                    } else if (gps_data.satellites_used >= 3) {
                        gps_status = "2D";  // 2D fix
                    } else {
                        gps_status = "FIX";  // Some fix
                    }
                } else {
                    gps_status = "---";  // No fix
                }
                
                // Check if button is currently pressed (for visual feedback)
                bool button_active = sensor_manager_is_button_held();
                
                // Line 1: Date + Time (INVERTED when button pressed)
                char date_str[16] = "YYYY-MM-DD";
                if (smart_time_get(&timeinfo) == ESP_OK) {
                    strftime(date_str, sizeof(date_str), "%Y-%m-%d", &timeinfo);
                }
                char status_line1[32];
                snprintf(status_line1, sizeof(status_line1), "%s %s", date_str, time_str);
                fb_write_text(0, status_line1, button_active);  // Invert entire line when button pressed
                
                // Line 2: Logging + GPS + Min distance (INVERTED when button pressed)
                char status_line2[32];
                if (fabsf(approach_rate) > 1.0f) {
                    snprintf(status_line2, sizeof(status_line2), "%s %s %dmm %+.0f", 
                            log_status, gps_status, min_distance, approach_rate);
                } else {
                    snprintf(status_line2, sizeof(status_line2), "%s %s %dmm", 
                            log_status, gps_status, min_distance);
                }
                fb_write_text(1, status_line2, button_active);  // Invert entire line when button pressed
            }
            
            // ===== BLUE ZONE (Bottom 48 pixels) - ToF Grid + IMU =====
            
            // Draw 4x4 grid of bar graphs (symmetric layout)
            // Each bar: 28 pixels wide × 8 pixels tall (1 page)
            // Spacing: 4 pixels between bars horizontally
            for (int row = 0; row < 4; row++) {
                for (int col = 0; col < 4; col++) {
                    int idx = row * 4 + col;
                    uint16_t distance = tof_data->distances[idx];
                    
                    // Symmetric positioning: 28px bar + 4px spacing = 32px per column
                    int x = col * 32;
                    int page = 2 + row;  // Pages 2-5 for the 4 rows
                    
                    // Draw bar in frame buffer
                    fb_draw_bar(page, x, 28, distance, 2000);
                }
            }
            
            // IMU summary at bottom (page 6-7)
            {
                char imu_line1[32];
                snprintf(imu_line1, sizeof(imu_line1), "R%+.0f P%+.0f Y%+.0f",
                         imu_data->roll, imu_data->pitch, imu_data->yaw);
                fb_write_text(6, imu_line1, false);
                
                char imu_line2[32];
                snprintf(imu_line2, sizeof(imu_line2), "J%.1f A%.1fg",
                         jerk_filtered,  // Use filtered jerk (less noisy)
                         sqrtf(imu_data->accel_x * imu_data->accel_x +
                               imu_data->accel_y * imu_data->accel_y +
                               imu_data->accel_z * imu_data->accel_z) / 9.81f);
                fb_write_text(7, imu_line2, false);
            }
            break;
            
        case OLED_MODE_TOF_FOCUS:
            // Yellow zone - Key metrics
            {
                char metrics[32];
                snprintf(metrics, sizeof(metrics), "Min:%dmm", min_distance);
                fb_write_text(0, metrics, false);
                
                // Line 2: Approach rate
                char rate_line[32];
                if (fabsf(approach_rate) > 1.0f) {
                    snprintf(rate_line, sizeof(rate_line), "Rate:%+.0fmm/s", approach_rate);
                } else {
                    snprintf(rate_line, sizeof(rate_line), "Rate: ---");
                }
                fb_write_text(1, rate_line, false);
            }
            
            // Blue zone - Large ToF grid with numbers
            // Smart formatting: show up to 4 digits, use "----" for invalid
            fb_write_text(2, "ToF Grid (mm):", false);
            for (int row = 0; row < 4; row++) {
                char line[32];
                char val0[5], val1[5], val2[5], val3[5];
                
                // Get distances
                uint16_t d0 = tof_data->distances[row*4+0];
                uint16_t d1 = tof_data->distances[row*4+1];
                uint16_t d2 = tof_data->distances[row*4+2];
                uint16_t d3 = tof_data->distances[row*4+3];
                
                // Format each value: show "----" for invalid (>4000), otherwise show number
                snprintf(val0, sizeof(val0), d0 > 4000 ? "----" : "%4d", d0);
                snprintf(val1, sizeof(val1), d1 > 4000 ? "----" : "%4d", d1);
                snprintf(val2, sizeof(val2), d2 > 4000 ? "----" : "%4d", d2);
                snprintf(val3, sizeof(val3), d3 > 4000 ? "----" : "%4d", d3);
                
                // Combine: 4 chars × 4 values = 16 chars (perfect fit!)
                snprintf(line, sizeof(line), "%s%s%s%s", val0, val1, val2, val3);
                fb_write_text(3+row, line, false);
            }
            
            // Page 7: Statistics
            {
                uint16_t max_dist = 0;
                uint32_t sum = 0;
                int valid_count = 0;
                for (int i = 0; i < 16; i++) {
                    uint16_t dist = tof_data->distances[i];
                    if (dist <= 4000) {  // Only count valid readings
                        if (dist > max_dist) {
                            max_dist = dist;
                        }
                        sum += dist;
                        valid_count++;
                    }
                }
                uint16_t avg_dist = valid_count > 0 ? (sum / valid_count) : 0;
                
                char stats[32];
                snprintf(stats, sizeof(stats), "Avg:%d Max:%d", avg_dist, max_dist);
                fb_write_text(7, stats, false);
            }
            break;
            
        case OLED_MODE_IMU_FOCUS:
            // Yellow zone - IMU status
            {
                char status[32];
                snprintf(status, sizeof(status), "IMU 100Hz");
                fb_write_text(0, status, false);
                
                // Line 2: Jerk magnitude (filtered)
                char jerk_line[32];
                snprintf(jerk_line, sizeof(jerk_line), "Jerk:%.1fm/s3", jerk_filtered);
                fb_write_text(1, jerk_line, false);
            }
            
            // Blue zone - Full IMU data
            fb_write_text(2, "Orient (deg):", false);
            {
                char line[32];
                snprintf(line, sizeof(line), " R:%+7.1f", imu_data->roll);
                fb_write_text(3, line, false);
                snprintf(line, sizeof(line), " P:%+7.1f", imu_data->pitch);
                fb_write_text(4, line, false);
                snprintf(line, sizeof(line), " Y:%+7.1f", imu_data->yaw);
                fb_write_text(5, line, false);
                
                fb_write_text(6, "Accel (m/s2):", false);
                snprintf(line, sizeof(line), "%.2f %.2f %.2f",
                         imu_data->accel_x, imu_data->accel_y, imu_data->accel_z);
                fb_write_text(7, line, false);
            }
            break;
            
        case OLED_MODE_STATUS:
            // Yellow zone - System info
            fb_write_text(0, "ProxiTof v1.0", false);
            
            // Line 2: Uptime
            {
                uint32_t uptime_sec = sensor_manager_get_timestamp() / 1000;
                uint32_t hours = uptime_sec / 3600;
                uint32_t minutes = (uptime_sec % 3600) / 60;
                char uptime[32];
                snprintf(uptime, sizeof(uptime), "Up:%02luh%02lum", 
                        (unsigned long)hours, (unsigned long)minutes);
                fb_write_text(1, uptime, false);
            }
            
            // Blue zone - Component status
            {
                // ToF status
                sensor_health_t health;
                sensor_manager_get_health(&health);
                char tof_status[32];
                snprintf(tof_status, sizeof(tof_status), "ToF:%s 60Hz", 
                        health.tof_healthy ? "OK " : "ERR");
                fb_write_text(2, tof_status, false);
                
                // IMU status
                char imu_status[32];
                snprintf(imu_status, sizeof(imu_status), "IMU:%s 100Hz", 
                        health.imu_healthy ? "OK " : "ERR");
                fb_write_text(3, imu_status, false);
                
                // SD card status
                storage_stats_t storage_stats;
                storage_manager_get_stats(&storage_stats);
                char sd_status[32];
                snprintf(sd_status, sizeof(sd_status), "SD: %s %luKB", 
                        storage_stats.logging_active ? "LOG" : "OK ",
                        (unsigned long)(storage_stats.current_file_size / 1024));
                fb_write_text(4, sd_status, false);
                
                // GPS status
                gps_data_t gps_data;
                char gps_status[32];
                if (gps_read(&gps_data) == ESP_OK && gps_data.valid) {
                    snprintf(gps_status, sizeof(gps_status), "GPS:FIX SAT:%d", 
                            gps_data.satellites_used);
                } else {
                    snprintf(gps_status, sizeof(gps_status), "GPS:SEARCH");
                }
                fb_write_text(5, gps_status, false);
                
                // RTC status (check if time is valid)
                struct tm timeinfo;
                char rtc_status[32];
                if (smart_time_get(&timeinfo) == ESP_OK && timeinfo.tm_year > (2024 - 1900)) {
                    snprintf(rtc_status, sizeof(rtc_status), "RTC:SYNC");
                } else {
                    snprintf(rtc_status, sizeof(rtc_status), "RTC:NOSYNC");
                }
                fb_write_text(6, rtc_status, false);
                
                // Min distance
                char line[32];
                snprintf(line, sizeof(line), "Min:%dmm", min_distance);
                fb_write_text(7, line, false);
            }
            break;
            
        case OLED_MODE_NEAR_MISS:
            // Yellow zone - ALERT! (always inverted, extra inversion when button pressed)
            {
                bool button_active = sensor_manager_is_button_held();
                // Always inverted for alert, but if button pressed, invert again (becomes normal)
                // This creates a "flashing" effect when flagging
                fb_write_text(0, "!NEAR MISS ALERT!", !button_active);  // Inverted normally, normal when button pressed
                
                // Line 2: Timestamp (also inverted when button pressed)
                struct tm timeinfo;
                char time_str[32] = "??:??:??";
                if (smart_time_get(&timeinfo) == ESP_OK) {
                    strftime(time_str, sizeof(time_str), "%H:%M:%S", &timeinfo);
                }
                fb_write_text(1, time_str, button_active);  // Invert when button pressed
            }
            
            // Blue zone - Reasons
            if (near_miss_active && near_miss_reason_count > 0) {
                fb_write_text(2, "Reasons:", false);
                for (int i = 0; i < near_miss_reason_count && i < 4; i++) {
                    fb_write_text(3 + i, near_miss_reasons[i], false);
                }
                
                // Fill remaining lines
                for (int i = 3 + near_miss_reason_count; i < 8; i++) {
                    fb_write_text(i, "", false);
                }
            } else {
                fb_write_text(2, "No active alert", false);
                fb_write_text(3, "", false);
                fb_write_text(4, "Press button to", false);
                fb_write_text(5, "return to", false);
                fb_write_text(6, "Dashboard", false);
                fb_write_text(7, "", false);
            }
            break;
            
        case OLED_MODE_WIFI_SETTINGS:
            // Yellow zone - WiFi Settings Header
            fb_write_text(0, "WiFi Settings", true);  // Inverted header
            
            // Line 2: WiFi SSID (always show from config, WiFi turns on only when syncing)
            {
                const system_config_t *config = config_manager_get_config();
                
                char ssid_line[32];
                if (strlen(config->wifi_ssid) > 0) {
                    // Display is 16 chars wide, truncate if needed
                    if (strlen(config->wifi_ssid) > 14) {
                        // Show first 12 chars + ".."
                        snprintf(ssid_line, sizeof(ssid_line), "%.12s..", config->wifi_ssid);
                    } else {
                        // Show full SSID (max 14 chars to fit in buffer safely)
                        snprintf(ssid_line, sizeof(ssid_line), "%.14s", config->wifi_ssid);
                    }
                } else {
                    snprintf(ssid_line, sizeof(ssid_line), "No SSID set");
                }
                fb_write_text(1, ssid_line, false);
            }
            
            // Blue zone - WiFi information and controls
            {
                time_status_t time_status;
                smart_time_get_status(&time_status);
                
                // Line 3: Sync status
                char status_line[32];
                if (time_status.time_synced) {
                    snprintf(status_line, sizeof(status_line), "Status: SYNCED");
                } else {
                    snprintf(status_line, sizeof(status_line), "Status: NOT SYNCED");
                }
                fb_write_text(2, status_line, false);
                
                // Line 4: Last sync time
                if (time_status.time_synced && time_status.last_sync_time > 0) {
                    time_t now = time(NULL);
                    time_t hours_ago = (now - time_status.last_sync_time) / 3600;
                    char sync_line[32];
                    if (hours_ago < 1) {
                        snprintf(sync_line, sizeof(sync_line), "Last: <1h ago");
                    } else if (hours_ago < 24) {
                        snprintf(sync_line, sizeof(sync_line), "Last: %ldh ago", (long)hours_ago);
                    } else {
                        snprintf(sync_line, sizeof(sync_line), "Last: %ldd ago", (long)(hours_ago / 24));
                    }
                    fb_write_text(3, sync_line, false);
                } else {
                    fb_write_text(3, "Last: Never", false);
                }
                
                // Line 5: Sync count
                char count_line[32];
                snprintf(count_line, sizeof(count_line), "Syncs: %lu", (unsigned long)time_status.sync_count);
                fb_write_text(4, count_line, false);
                
                // Line 6: Empty line for spacing
                fb_write_text(5, "", false);
                
                // Line 7-8: Instructions (centered on screen)
                fb_write_text(6, "Press ACTION to", false);
                fb_write_text(7, "sync time (WiFi)", false);
            }
            break;
            
        case OLED_MODE_GPS:
            // Yellow zone - GPS Status Header with Satellite Progress Bar
            {
                gps_data_t gps_data;
                esp_err_t gps_err = gps_read(&gps_data);
                
                if (gps_err == ESP_OK && gps_data.valid) {
                    // GPS has fix - show fix type and satellite count
                    char fix_line[32];
                    const char* fix_type = "FIX";
                    if (gps_data.satellites_used >= 4) {
                        fix_type = "3D FIX";
                    } else if (gps_data.satellites_used >= 3) {
                        fix_type = "2D FIX";
                    }
                    snprintf(fix_line, sizeof(fix_line), "GPS: %s (%dsat)", fix_type, gps_data.satellites_used);
                    fb_write_text(0, fix_line, false);
                    
                    // HDOP (accuracy indicator) + UTC Time
                    char hdop_time_line[32];
                    snprintf(hdop_time_line, sizeof(hdop_time_line), "HDOP:%.1f %02d:%02d:%02d", 
                            gps_data.hdop, gps_data.hour, gps_data.minute, gps_data.second);
                    fb_write_text(1, hdop_time_line, false);
                } else {
                    // No fix - show searching status with satellite progress bar
                    fb_write_text(0, "GPS: SEARCHING", false);
                    
                    // Get stats to show progress
                    gps_stats_t stats;
                    gps_get_stats(&stats);
                    
                    // Read current GPS data even if not valid (to get satellite count)
                    gps_data_t temp_data;
                    gps_read(&temp_data);
                    uint8_t sats = temp_data.satellites_used;
                    
                    // Satellite progress bar: [====    ] 4/12
                    char sat_bar[32];
                    int bar_width = 10;  // 10 characters for bar
                    int filled = (sats * bar_width) / 12;  // Assume 12 sats max for display
                    if (filled > bar_width) filled = bar_width;
                    
                    char bar[12] = {0};
                    for (int i = 0; i < bar_width; i++) {
                        bar[i] = (i < filled) ? '=' : ' ';
                    }
                    
                    snprintf(sat_bar, sizeof(sat_bar), "[%s]%d/12", bar, sats);
                    fb_write_text(1, sat_bar, false);
                }
            }
            
            // Blue zone - GPS Data
            {
                gps_data_t gps_data;
                esp_err_t gps_err = gps_read(&gps_data);
                
                if (gps_err == ESP_OK && gps_data.valid) {
                    // Line 3: Latitude
                    char lat_line[32];
                    snprintf(lat_line, sizeof(lat_line), "Lat: %.6f%c", 
                            fabs(gps_data.latitude), 
                            gps_data.latitude >= 0 ? 'N' : 'S');
                    fb_write_text(2, lat_line, false);
                    
                    // Line 4: Longitude
                    char lon_line[32];
                    snprintf(lon_line, sizeof(lon_line), "Lon: %.6f%c", 
                            fabs(gps_data.longitude), 
                            gps_data.longitude >= 0 ? 'E' : 'W');
                    fb_write_text(3, lon_line, false);
                    
                    // Line 5: Altitude + Date
                    char alt_date_line[32];
                    snprintf(alt_date_line, sizeof(alt_date_line), "Alt:%.0fm %02d/%02d/%04d", 
                            gps_data.altitude_m, gps_data.day, gps_data.month, gps_data.year);
                    fb_write_text(4, alt_date_line, false);
                    
                    // Line 6: Speed (both km/h and m/s)
                    char speed_line[32];
                    snprintf(speed_line, sizeof(speed_line), "Spd:%.1fkm/h %.1fm/s", 
                            gps_data.speed_kmh, gps_data.speed_ms);
                    fb_write_text(5, speed_line, false);
                    
                    // Line 7: Heading with compass direction
                    char heading_line[32];
                    const char* compass = "N";
                    float hdg = gps_data.heading_deg;
                    if (hdg >= 337.5 || hdg < 22.5) compass = "N";
                    else if (hdg >= 22.5 && hdg < 67.5) compass = "NE";
                    else if (hdg >= 67.5 && hdg < 112.5) compass = "E";
                    else if (hdg >= 112.5 && hdg < 157.5) compass = "SE";
                    else if (hdg >= 157.5 && hdg < 202.5) compass = "S";
                    else if (hdg >= 202.5 && hdg < 247.5) compass = "SW";
                    else if (hdg >= 247.5 && hdg < 292.5) compass = "W";
                    else if (hdg >= 292.5 && hdg < 337.5) compass = "NW";
                    snprintf(heading_line, sizeof(heading_line), "Hdg: %.0fdeg (%s)", hdg, compass);
                    fb_write_text(6, heading_line, false);
                    
                    // Line 8: Data age
                    uint32_t age_ms = gps_get_time_since_update();
                    char age_line[32];
                    if (age_ms < 1000) {
                        snprintf(age_line, sizeof(age_line), "Age: %lums", (unsigned long)age_ms);
                    } else {
                        snprintf(age_line, sizeof(age_line), "Age: %.1fs", age_ms / 1000.0f);
                    }
                    fb_write_text(7, age_line, false);
                } else {
                    // No fix - show acquisition progress
                    gps_stats_t stats;
                    gps_get_stats(&stats);
                    
                    fb_write_text(2, "Acquiring sats..", false);
                    
                    // Show parsed sentences count
                    char parsed_line[32];
                    snprintf(parsed_line, sizeof(parsed_line), "Parsed: %lu", (unsigned long)stats.sentences_parsed);
                    fb_write_text(3, parsed_line, false);
                    
                    // Show time to first fix if available
                    if (stats.first_fix_acquired) {
                        char ttff_line[32];
                        snprintf(ttff_line, sizeof(ttff_line), "TTFF: %lus", (unsigned long)stats.time_to_first_fix_s);
                        fb_write_text(4, ttff_line, false);
                    } else {
                        fb_write_text(4, "", false);
                    }
                    
                    fb_write_text(5, "This may take", false);
                    fb_write_text(6, "1-5 min outdoors", false);
                    
                    // Show PPS status hint
                    fb_write_text(7, "PPS blink=time OK", false);
                }
            }
            break;
            
        case OLED_MODE_USB_TRANSFER:
            // Yellow zone - USB Transfer Header (INVERTED for attention!)
            {
                bool usb_msc_active = usb_msc_sdcard_is_active();
                
                if (!usb_msc_active) {
                    // Not in USB mode - show activation screen
                    fb_write_text(0, "USB Transfer Mode", true);  // Inverted
                    fb_write_text(1, "Press ACTION", true);       // Inverted
                } else {
                    // In USB mode - show active status with connection indicator
                    bool usb_connected = usb_msc_sdcard_is_host_connected();
                    bool usb_ready = usb_msc_sdcard_is_ready();
                    
                    if (usb_connected && usb_ready) {
                        fb_write_text(0, "!USB CONNECTED!", true);  // Inverted
                        fb_write_text(1, "PC accessing SD", true);  // Inverted
                    } else if (usb_connected && !usb_ready) {
                        fb_write_text(0, "!USB SUSPENDED!", true);  // Inverted
                        fb_write_text(1, "PC idle/sleep", true);    // Inverted
                    } else {
                        fb_write_text(0, "!USB MODE ACTIVE!", true);  // Inverted
                        fb_write_text(1, "Waiting for PC", true);     // Inverted
                    }
                }
            }
            
            // Blue zone - Instructions and status
            {
                bool usb_msc_active = usb_msc_sdcard_is_active();
                
                if (!usb_msc_active) {
                    // Activation screen - explain what will happen
                    fb_write_text(2, "This will:", false);
                    fb_write_text(3, "1. Pause logging", false);
                    fb_write_text(4, "2. Enable USB MSC", false);
                    fb_write_text(5, "3. Expose SD card", false);
                    fb_write_text(6, "", false);
                    fb_write_text(7, "ACTION: Activate", false);
                } else {
                    // Active screen - show live status
                    bool usb_connected = usb_msc_sdcard_is_host_connected();
                    bool usb_ready = usb_msc_sdcard_is_ready();
                    
                    fb_write_text(2, "", false);
                    
                    if (usb_connected && usb_ready) {
                        // PC connected and ready - safe to transfer files
                        fb_write_text(3, "Status: READY", false);
                        fb_write_text(4, "", false);
                        fb_write_text(5, "PC can read/write", false);
                        fb_write_text(6, "", false);
                        fb_write_text(7, "1.Eject on PC", false);
                        fb_write_text(8, "2.ACTION to exit", false);
                    } else if (usb_connected && !usb_ready) {
                        // PC connected but suspended (idle/sleep)
                        fb_write_text(3, "Status: SUSPENDED", false);
                        fb_write_text(4, "", false);
                        fb_write_text(5, "PC idle or sleep", false);
                        fb_write_text(6, "Wake PC to access", false);
                        fb_write_text(7, "", false);
                        fb_write_text(8, "ACTION: Exit", false);
                    } else {
                        // Not connected - waiting for PC
                        fb_write_text(3, "Status: WAITING", false);
                        fb_write_text(4, "", false);
                        fb_write_text(5, "Connect LEFT port", false);
                        fb_write_text(6, "to PC", false);
                        fb_write_text(7, "", false);
                        fb_write_text(8, "ACTION: Exit (safe)", false);
                    }
                }
            }
            break;
            
        case OLED_MODE_SHUTDOWN:
            // Yellow zone - Shutdown Warning (INVERTED for attention!)
            if (!shutdown_confirmation_pending) {
                // First press - show warning
                fb_write_text(0, "!SHUTDOWN MODE!", true);  // Inverted
                fb_write_text(1, "Press ACTION", true);     // Inverted
            } else {
                // Second press pending - show confirmation
                fb_write_text(0, "!CONFIRM SHUTDOWN!", true);  // Inverted
                fb_write_text(1, "Press ACTION again", true);  // Inverted
            }
            
            // Blue zone - Instructions and warnings
            if (!shutdown_confirmation_pending) {
                // First screen - explain what will happen
                fb_write_text(2, "This will:", false);
                fb_write_text(3, "1. Stop logging", false);
                fb_write_text(4, "2. Save all data", false);
                fb_write_text(5, "3. Power off", false);
                fb_write_text(6, "", false);
                fb_write_text(7, "Press MODE cancel", false);
            } else {
                // Confirmation screen - final warning
                fb_write_text(2, "Are you sure?", false);
                fb_write_text(3, "", false);
                fb_write_text(4, "ACTION = Confirm", false);
                fb_write_text(5, "MODE = Cancel", false);
                fb_write_text(6, "", false);
                
                // Show countdown timer (10 seconds to confirm)
                uint32_t now = sensor_manager_get_timestamp();
                uint32_t elapsed = (now - shutdown_confirmation_time) / 1000;
                uint32_t remaining = 10 - elapsed;
                
                if (remaining > 0) {
                    char timer[32];
                    snprintf(timer, sizeof(timer), "Timeout: %lus", (unsigned long)remaining);
                    fb_write_text(7, timer, false);
                } else {
                    // Timeout - cancel confirmation
                    shutdown_confirmation_pending = false;
                    fb_write_text(7, "TIMEOUT - Cancelled", false);
                }
            }
            break;
            
        default:
            break;
    }
    
    // Draw volume popup overlay (if active)
    if (volume_popup_active) {
        draw_volume_popup();
    }
    
    // Update display with differential rendering (only changed regions)
    update_display_differential();
    
    return ESP_OK;
}

esp_err_t oled_display_set_mode(oled_display_mode_t mode) {
    if (!display_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    if (mode >= OLED_MODE_MAX) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // Cancel shutdown confirmation if leaving shutdown mode
    if (current_config.mode == OLED_MODE_SHUTDOWN && mode != OLED_MODE_SHUTDOWN) {
        shutdown_confirmation_pending = false;
        shutdown_confirmed = false;
        ESP_LOGI(TAG, "Shutdown cancelled - mode changed");
    }
    
    current_config.mode = mode;
    
    const char *mode_names[] = {"Dashboard", "Status", "ToF Focus", "IMU Focus", "GPS", "Near Miss", "WiFi Settings", "USB Transfer", "Shutdown"};
    ESP_LOGI(TAG, "Display mode changed to: %s", mode_names[mode]);
    
    // Show mode change notification
    ssd1306_clear_screen(&dev, false);
    char msg[32];
    snprintf(msg, sizeof(msg), "Mode: %s", mode_names[mode]);
    ssd1306_display_text(&dev, 3, msg, strlen(msg), false);
    vTaskDelay(pdMS_TO_TICKS(500));  // Show for 500ms
    
    return ESP_OK;
}

esp_err_t oled_display_set_brightness(uint8_t brightness) {
    if (!display_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    current_config.brightness = brightness;
    ssd1306_contrast(&dev, brightness);
    
    ESP_LOGI(TAG, "Display brightness set to: %d/255", brightness);
    
    return ESP_OK;
}

oled_display_mode_t oled_display_get_mode(void) {
    return current_config.mode;
}


esp_err_t oled_display_handle_action_button(void) {
    if (!display_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    oled_display_mode_t mode = current_config.mode;
    
    switch (mode) {
        case OLED_MODE_DASHBOARD:
        case OLED_MODE_NEAR_MISS:
            // These modes handle button in main.c for event flagging
            return ESP_ERR_NOT_SUPPORTED;
            
        case OLED_MODE_WIFI_SETTINGS:
            // Trigger WiFi sync with progress display
            ESP_LOGI(TAG, "WiFi sync requested from display");
            
            // Show initial connecting message
            clear_frame_buffer();
            fb_write_text(0, "WiFi Sync", true);
            fb_write_text(2, "Connecting WiFi..", false);
            // Draw progress bar (empty)
            for (int x = 10; x < 118; x++) {
                frame_buffer[4 * 128 + x] = 0x81;  // Top and bottom border
            }
            update_display_differential();
            vTaskDelay(pdMS_TO_TICKS(500));
            
            // Update progress: WiFi connecting (25%)
            clear_frame_buffer();
            fb_write_text(0, "WiFi Sync", true);
            fb_write_text(2, "WiFi connecting..", false);
            // Draw progress bar (25%)
            for (int x = 10; x < 118; x++) {
                if (x < 37) {
                    frame_buffer[4 * 128 + x] = 0xFF;  // Filled
                } else {
                    frame_buffer[4 * 128 + x] = 0x81;  // Empty border
                }
            }
            update_display_differential();
            
            // Trigger sync
            esp_err_t ret = smart_time_sync_now();
            
            if (ret == ESP_OK) {
                // Success - show 100% progress
                clear_frame_buffer();
                fb_write_text(0, "WiFi Sync", true);
                fb_write_text(2, "Sync SUCCESS!", false);
                fb_write_text(3, "Time updated", false);
                // Draw progress bar (100%)
                for (int x = 10; x < 118; x++) {
                    frame_buffer[4 * 128 + x] = 0xFF;  // Fully filled
                }
                update_display_differential();
            } else {
                // Failed
                clear_frame_buffer();
                fb_write_text(0, "WiFi Sync", true);
                fb_write_text(2, "Sync FAILED", false);
                if (ret == ESP_ERR_NOT_SUPPORTED) {
                    fb_write_text(3, "WiFi disabled", false);
                } else if (ret == ESP_ERR_TIMEOUT) {
                    fb_write_text(3, "Connection timeout", false);
                } else {
                    fb_write_text(3, "Check config.txt", false);
                }
                update_display_differential();
            }
            
            // Wait 2 seconds to show result
            vTaskDelay(pdMS_TO_TICKS(2000));
            
            return ret;
            
        case OLED_MODE_GPS:
            // Trigger GPS reconnect
            ESP_LOGI(TAG, "GPS reconnect requested from display");
            
            // Show reconnecting message
            clear_frame_buffer();
            fb_write_text(0, "GPS Reconnect", true);
            fb_write_text(2, "Reinitializing..", false);
            update_display_differential();
            vTaskDelay(pdMS_TO_TICKS(500));
            
            // Deinitialize and reinitialize GPS
            gps_deinit();
            vTaskDelay(pdMS_TO_TICKS(200));
            
            // Reinitialize with correct pins: GPIO 38 (TX), 47 (RX), 38400 baud
            esp_err_t gps_ret = gps_init(UART_NUM_1, 38, 47, 38400);
            
            if (gps_ret == ESP_OK) {
                // Success
                clear_frame_buffer();
                fb_write_text(0, "GPS Reconnect", true);
                fb_write_text(2, "SUCCESS!", false);
                fb_write_text(3, "Searching sats..", false);
                update_display_differential();
            } else {
                // Failed
                clear_frame_buffer();
                fb_write_text(0, "GPS Reconnect", true);
                fb_write_text(2, "FAILED", false);
                fb_write_text(3, "Check wiring", false);
                update_display_differential();
            }
            
            // Wait 2 seconds to show result
            vTaskDelay(pdMS_TO_TICKS(2000));
            
            return gps_ret;
            
        case OLED_MODE_IMU_FOCUS:
            // Trigger IMU rescan
            ESP_LOGI(TAG, "IMU rescan requested from display");
            
            // Show rescanning message
            clear_frame_buffer();
            fb_write_text(0, "IMU Rescan", true);
            fb_write_text(2, "Reinitializing..", false);
            fb_write_text(3, "Please wait", false);
            update_display_differential();
            vTaskDelay(pdMS_TO_TICKS(500));
            
            // Reinitialize IMU
            esp_err_t imu_ret = sensor_manager_reinit_imu();
            
            if (imu_ret == ESP_OK) {
                // Success
                clear_frame_buffer();
                fb_write_text(0, "IMU Rescan", true);
                fb_write_text(2, "SUCCESS!", false);
                fb_write_text(3, "IMU ready", false);
                update_display_differential();
            } else {
                // Failed
                clear_frame_buffer();
                fb_write_text(0, "IMU Rescan", true);
                fb_write_text(2, "FAILED", false);
                fb_write_text(3, "Check wiring", false);
                fb_write_text(5, "Or restart", false);
                fb_write_text(6, "device", false);
                update_display_differential();
            }
            
            // Wait 2 seconds to show result
            vTaskDelay(pdMS_TO_TICKS(2000));
            
            return imu_ret;
            
        case OLED_MODE_USB_TRANSFER:
            // Toggle USB MSC mode
            ESP_LOGI(TAG, "USB Transfer mode toggle requested from display");
            
            bool usb_msc_active = usb_msc_sdcard_is_active();
            
            if (!usb_msc_active) {
                // Activate USB MSC mode
                
                // Initialize USB MSC if not already initialized (lazy initialization)
                if (!usb_msc_sdcard_is_initialized()) {
                    ESP_LOGI(TAG, "USB MSC not initialized, initializing now...");
                    esp_err_t init_ret = usb_msc_sdcard_init();
                    if (init_ret != ESP_OK) {
                        ESP_LOGE(TAG, "USB MSC initialization failed: %s", esp_err_to_name(init_ret));
                        return init_ret;
                    }
                    ESP_LOGI(TAG, "✓ USB MSC initialized successfully");
                }
                
                // Pause logging and enable USB MSC (no blocking UI updates)
                esp_err_t ret = usb_msc_sdcard_pause_logging();
                
                if (ret != ESP_OK) {
                    ESP_LOGE(TAG, "Failed to activate USB mode: %s", esp_err_to_name(ret));
                }
                
                // Force display update to show new status immediately
                // This ensures the display shows the USB active state right away
                vTaskDelay(pdMS_TO_TICKS(50));  // Small delay to let USB settle
                
                return ret;
            } else {
                // Deactivate USB MSC mode
                ESP_LOGI(TAG, "Deactivating USB Transfer mode");
                ESP_LOGI(TAG, "💡 Ensure you ejected on PC and disconnected cable first!");
                
                // Resume logging and disable USB MSC (no blocking UI updates)
                esp_err_t ret = usb_msc_sdcard_resume_logging();
                
                if (ret != ESP_OK) {
                    ESP_LOGE(TAG, "Failed to deactivate USB mode: %s", esp_err_to_name(ret));
                }
                
                // Force display update to show new status immediately
                vTaskDelay(pdMS_TO_TICKS(50));  // Small delay to let SD card settle
                
                return ret;
            }
            
        case OLED_MODE_SHUTDOWN:
            // Handle shutdown confirmation
            if (!shutdown_confirmation_pending) {
                // First press - enter confirmation mode
                shutdown_confirmation_pending = true;
                shutdown_confirmation_time = sensor_manager_get_timestamp();
                ESP_LOGW(TAG, "Shutdown confirmation requested - press ACTION again to confirm");
                return ESP_OK;
            } else {
                // Second press - confirm shutdown
                uint32_t now = sensor_manager_get_timestamp();
                uint32_t elapsed = now - shutdown_confirmation_time;
                
                // Check if within 10 second timeout
                if (elapsed < 10000) {
                    shutdown_confirmed = true;
                    ESP_LOGW(TAG, "SHUTDOWN CONFIRMED - System will power down");
                    
                    // Show final message
                    clear_frame_buffer();
                    fb_write_text(0, "!SHUTTING DOWN!", true);
                    fb_write_text(1, "", true);
                    fb_write_text(3, "Saving data...", false);
                    fb_write_text(4, "Please wait", false);
                    update_display_differential();
                    
                    return ESP_OK;
                } else {
                    // Timeout - reset confirmation
                    shutdown_confirmation_pending = false;
                    ESP_LOGI(TAG, "Shutdown confirmation timeout - cancelled");
                    return ESP_ERR_TIMEOUT;
                }
            }
            
        default:
            // No action for other modes
            return ESP_ERR_NOT_SUPPORTED;
    }
}

bool oled_display_is_shutdown_confirmed(void) {
    return shutdown_confirmed;
}

esp_err_t oled_display_show_startup(uint8_t progress, const char *status_text) {
    if (!display_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    // Clamp progress to 0-100
    if (progress > 100) {
        progress = 100;
    }
    
    // Clear display
    ssd1306_clear_screen(&dev, false);
    
    // ===== YELLOW ZONE (Top 16 pixels, Pages 0-1) - Progress Bar =====
    // Draw progress bar in yellow zone
    uint8_t progress_bar[128];
    int fill_width = (progress * 128) / 100;
    
    // Page 0: Top border and fill
    for (int x = 0; x < 128; x++) {
        if (x < fill_width) {
            progress_bar[x] = 0xFF;  // Fully filled
        } else {
            progress_bar[x] = 0x81;  // Just borders (top and bottom pixels)
        }
    }
    ssd1306_display_image(&dev, 0, 0, progress_bar, 128);
    
    // Page 1: Bottom border and fill
    for (int x = 0; x < 128; x++) {
        if (x < fill_width) {
            progress_bar[x] = 0xFF;  // Fully filled
        } else {
            progress_bar[x] = 0x81;  // Just borders
        }
    }
    ssd1306_display_image(&dev, 1, 0, progress_bar, 128);
    
    // ===== BLUE ZONE (Bottom 48 pixels, Pages 2-7) - Logo and Status =====
    // Display Proxitof logo in blue zone (skip yellow zone pages 0-1)
    // Logo is 64 pixels tall (8 pages), but we only have 48 pixels (6 pages) in blue zone
    // Display pages 1-6 of the logo (skip first page, fits perfectly in pages 2-7)
    for (int display_page = 2; display_page <= 7; display_page++) {
        int logo_page = display_page - 1;  // Logo page 1-6 maps to display pages 2-7
        ssd1306_display_image(&dev, display_page, 0, &proxitof_logo_ssd1306[logo_page * 128], 128);
    }
    
    // Overlay status text on page 6 (if provided)
    if (status_text != NULL && strlen(status_text) > 0) {
        ssd1306_display_text(&dev, 6, status_text, strlen(status_text), false);
    }
    
    // Overlay progress percentage on page 7
    char progress_str[16];
    snprintf(progress_str, sizeof(progress_str), "%d%%", progress);
    ssd1306_display_text(&dev, 7, progress_str, strlen(progress_str), false);
    
    return ESP_OK;
}


/**
 * Show volume popup overlay with circular progress bar
 */
esp_err_t oled_display_show_volume_popup(uint8_t volume_percent)
{
    if (!display_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    if (volume_percent > 100) {
        volume_percent = 100;
    }
    
    // Update popup state
    volume_popup_value = volume_percent;
    volume_popup_last_update = xTaskGetTickCount() * portTICK_PERIOD_MS;
    volume_popup_active = true;
    
    return ESP_OK;
}

/**
 * Draw volume popup overlay (simplified - text-based)
 */
static void draw_volume_popup(void)
{
    if (!volume_popup_active) {
        return;
    }
    
    // Check if popup should auto-hide (2 seconds timeout)
    uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
    if ((current_time - volume_popup_last_update) > 2000) {
        volume_popup_active = false;
        return;
    }
    
    // Draw popup using frame buffer (centered on screen)
    // Page 3-4 (middle of screen)
    
    // Clear popup area
    for (int page = 3; page <= 4; page++) {
        for (int x = 16; x < 112; x++) {
            frame_buffer[page * 128 + x] = 0x00;
        }
    }
    
    // Draw border (top and bottom)
    for (int x = 16; x < 112; x++) {
        frame_buffer[3 * 128 + x] |= 0x01;  // Top border
        frame_buffer[4 * 128 + x] |= 0x80;  // Bottom border
    }
    
    // Draw side borders
    for (int page = 3; page <= 4; page++) {
        frame_buffer[page * 128 + 16] = 0xFF;  // Left border
        frame_buffer[page * 128 + 111] = 0xFF;  // Right border
    }
    
    // Draw volume bar (progress bar)
    int bar_width = (volume_popup_value * 80) / 100;  // 80 pixels max width
    for (int x = 0; x < bar_width; x++) {
        frame_buffer[4 * 128 + 20 + x] |= 0x3C;  // Middle bits for bar
    }
    
    // Draw volume text
    char vol_text[16];
    snprintf(vol_text, sizeof(vol_text), "VOL:%d%%", volume_popup_value);
    fb_write_text(3, vol_text, false);
}
