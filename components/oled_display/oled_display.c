/**
 * oled_display.c
 * 
 * OLED Display Driver - SPI Mode with nopnop2002 library
 * Features: Dual-color zones, bar graph ToF visualization, multiple layouts
 */

#include "oled_display.h"
#include "ssd1306.h"
#include "font8x8_basic.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "smart_time.h"
#include "storage_manager.h"
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
                
                // Get GPS status (simplified - you can expand this)
                const char *gps_status = "GPS";  // TODO: Add actual GPS fix status
                
                // Line 1: Time + Logging + GPS
                char status_line1[32];
                snprintf(status_line1, sizeof(status_line1), "%s %s %s", time_str, log_status, gps_status);
                fb_write_text(0, status_line1, false);
                
                // Line 2: Min distance
                char status_line2[32];
                snprintf(status_line2, sizeof(status_line2), "Min:%dmm", min_distance);
                fb_write_text(1, status_line2, false);
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
            
            // Gaps between bars are automatically handled by the 32px spacing
            
            // IMU summary at bottom (page 6-7)
            {
                char imu_line1[32];
                snprintf(imu_line1, sizeof(imu_line1), "R%+.0f P%+.0f Y%+.0f",
                         imu_data->roll, imu_data->pitch, imu_data->yaw);
                fb_write_text(6, imu_line1, false);
                
                char imu_line2[32];
                snprintf(imu_line2, sizeof(imu_line2), "A%.2f %.2f %.2f",
                         imu_data->accel_x, imu_data->accel_y, imu_data->accel_z);
                fb_write_text(7, imu_line2, false);
            }
            break;
            
        case OLED_MODE_TOF_FOCUS:
            // Yellow zone - Key metrics
            {
                char metrics[32];
                snprintf(metrics, sizeof(metrics), "Min:%dmm", min_distance);
                fb_write_text(0, metrics, false);
            }
            
            // Blue zone - Large ToF grid with numbers
            fb_write_text(2, "ToF 4x4 Grid:", false);
            for (int row = 0; row < 4; row++) {
                char line[32];
                snprintf(line, sizeof(line), "%4d %4d %4d %4d",
                         tof_data->distances[row*4+0], tof_data->distances[row*4+1],
                         tof_data->distances[row*4+2], tof_data->distances[row*4+3]);
                fb_write_text(3+row, line, false);
            }
            break;
            
        case OLED_MODE_IMU_FOCUS:
            // Yellow zone - IMU status
            {
                char status[32];
                float jerk = sqrtf(imu_data->accel_x * imu_data->accel_x +
                                   imu_data->accel_y * imu_data->accel_y +
                                   imu_data->accel_z * imu_data->accel_z);
                snprintf(status, sizeof(status), "IMU J:%.1fm/s3", jerk);
                fb_write_text(0, status, false);
            }
            
            // Blue zone - Full IMU data
            fb_write_text(2, "Orientation:", false);
            {
                char line[32];
                snprintf(line, sizeof(line), " R:%+7.2f", imu_data->roll);
                fb_write_text(3, line, false);
                snprintf(line, sizeof(line), " P:%+7.2f", imu_data->pitch);
                fb_write_text(4, line, false);
                snprintf(line, sizeof(line), " Y:%+7.2f", imu_data->yaw);
                fb_write_text(5, line, false);
                
                fb_write_text(6, "Accel:", false);
                snprintf(line, sizeof(line), "%.2f %.2f %.2f",
                         imu_data->accel_x, imu_data->accel_y, imu_data->accel_z);
                fb_write_text(7, line, false);
            }
            break;
            
        case OLED_MODE_STATUS:
            // Yellow zone - System info
            fb_write_text(0, "ProxiTof v1.0", false);
            
            // Blue zone - Component status
            fb_write_text(2, "ToF: OK  60Hz", false);
            fb_write_text(3, "IMU: OK 100Hz", false);
            fb_write_text(4, "SD:  OK", false);
            fb_write_text(5, "GPS: SEARCHING", false);
            {
                char line[32];
                snprintf(line, sizeof(line), "Min: %dmm", min_distance);
                fb_write_text(7, line, false);
            }
            break;
            
        default:
            break;
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
    
    current_config.mode = mode;
    
    const char *mode_names[] = {"Dashboard", "ToF Focus", "IMU Focus", "Status"};
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
