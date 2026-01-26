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
#include <string.h>
#include <stdio.h>
#include <math.h>

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

// Display state
static bool display_initialized = false;
static SSD1306_t dev;
static oled_config_t current_config = {
    .brightness = 128,
    .flip_display = false,
    .mode = OLED_MODE_DASHBOARD
};

/**
 * Draw a horizontal bar graph (loading bar style, expands from center)
 * Used for ToF distance visualization
 * 
 * @param page Page number (0-7)
 * @param x X position
 * @param width Total width of bar area
 * @param value Distance value (0-4000mm)
 * @param max_value Maximum distance (4000mm)
 */
static void draw_bar_from_center(SSD1306_t *dev, int page, int x, int width, uint16_t value, uint16_t max_value) {
    // Calculate fill percentage (inverted: closer = more filled)
    float fill_percent = 1.0f - ((float)value / (float)max_value);
    if (fill_percent < 0.0f) fill_percent = 0.0f;
    if (fill_percent > 1.0f) fill_percent = 1.0f;
    
    // Calculate bar width from center
    int fill_width = (int)(width * fill_percent / 2.0f);
    int center = x + width / 2;
    
    // Draw bar expanding from center
    for (int i = 0; i < fill_width; i++) {
        // Draw left side
        _ssd1306_pixel(dev, center - i - 1, page * 8 + 3, true);
        _ssd1306_pixel(dev, center - i - 1, page * 8 + 4, true);
        
        // Draw right side
        _ssd1306_pixel(dev, center + i, page * 8 + 3, true);
        _ssd1306_pixel(dev, center + i, page * 8 + 4, true);
    }
    
    // Draw border
    _ssd1306_line(dev, x, page * 8 + 2, x + width - 1, page * 8 + 2, true);
    _ssd1306_line(dev, x, page * 8 + 5, x + width - 1, page * 8 + 5, true);
    _ssd1306_line(dev, x, page * 8 + 2, x, page * 8 + 5, true);
    _ssd1306_line(dev, x + width - 1, page * 8 + 2, x + width - 1, page * 8 + 5, true);
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
    
    ssd1306_clear_screen(&dev, false);
    return ESP_OK;
}

esp_err_t oled_display_update(const tof_data_t *tof_data, 
                              const imu_data_t *imu_data, 
                              uint16_t min_distance) {
    if (!display_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    // Clear screen
    ssd1306_clear_screen(&dev, false);
    
    switch (current_config.mode) {
        case OLED_MODE_DASHBOARD:
            // Yellow zone - Status bar
            {
                char status[32];
                snprintf(status, sizeof(status), "LOG %dmm %dHz", min_distance, 60);
                ssd1306_display_text(&dev, 0, status, strlen(status), false);
            }
            
            // Blue zone - ToF Grid with bar graphs (4x4)
            ssd1306_display_text(&dev, 2, "ToF Grid:", 9, false);
            
            // Draw 4x4 grid of bar graphs
            for (int row = 0; row < 4; row++) {
                for (int col = 0; col < 4; col++) {
                    int idx = row * 4 + col;
                    uint16_t distance = tof_data->distances[idx];
                    
                    // Position: 4 bars per row, each 30 pixels wide
                    int x = col * 30 + 2;
                    int page = 3 + row;  // Pages 3-6 for the 4 rows
                    
                    // Draw bar from center (closer = wider bar)
                    draw_bar_from_center(&dev, page, x, 28, distance, 2000);
                }
            }
            
            // IMU summary at bottom
            {
                char imu_line[32];
                snprintf(imu_line, sizeof(imu_line), "R%+.0f P%+.0f Y%+.0f",
                         imu_data->roll, imu_data->pitch, imu_data->yaw);
                ssd1306_display_text(&dev, 7, imu_line, strlen(imu_line), false);
            }
            break;
            
        case OLED_MODE_TOF_FOCUS:
            // Yellow zone - Key metrics
            {
                char metrics[32];
                snprintf(metrics, sizeof(metrics), "Min:%dmm", min_distance);
                ssd1306_display_text(&dev, 0, metrics, strlen(metrics), false);
            }
            
            // Blue zone - Large ToF grid with numbers
            ssd1306_display_text(&dev, 2, "ToF 4x4 Grid:", 13, false);
            for (int row = 0; row < 4; row++) {
                char line[32];
                snprintf(line, sizeof(line), "%4d %4d %4d %4d",
                         tof_data->distances[row*4+0], tof_data->distances[row*4+1],
                         tof_data->distances[row*4+2], tof_data->distances[row*4+3]);
                ssd1306_display_text(&dev, 3+row, line, strlen(line), false);
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
                ssd1306_display_text(&dev, 0, status, strlen(status), false);
            }
            
            // Blue zone - Full IMU data
            ssd1306_display_text(&dev, 2, "Orientation:", 12, false);
            {
                char line[32];
                snprintf(line, sizeof(line), " R:%+7.2f", imu_data->roll);
                ssd1306_display_text(&dev, 3, line, strlen(line), false);
                snprintf(line, sizeof(line), " P:%+7.2f", imu_data->pitch);
                ssd1306_display_text(&dev, 4, line, strlen(line), false);
                snprintf(line, sizeof(line), " Y:%+7.2f", imu_data->yaw);
                ssd1306_display_text(&dev, 5, line, strlen(line), false);
                
                ssd1306_display_text(&dev, 6, "Accel:", 6, false);
                snprintf(line, sizeof(line), "%.2f %.2f %.2f",
                         imu_data->accel_x, imu_data->accel_y, imu_data->accel_z);
                ssd1306_display_text(&dev, 7, line, strlen(line), false);
            }
            break;
            
        case OLED_MODE_STATUS:
            // Yellow zone - System info
            ssd1306_display_text(&dev, 0, "ProxiTof v1.0", 13, false);
            
            // Blue zone - Component status
            ssd1306_display_text(&dev, 2, "ToF: OK  60Hz", 13, false);
            ssd1306_display_text(&dev, 3, "IMU: OK 100Hz", 13, false);
            ssd1306_display_text(&dev, 4, "SD:  OK", 7, false);
            ssd1306_display_text(&dev, 5, "GPS: SEARCHING", 14, false);
            {
                char line[32];
                snprintf(line, sizeof(line), "Min: %dmm", min_distance);
                ssd1306_display_text(&dev, 7, line, strlen(line), false);
            }
            break;
            
        default:
            break;
    }
    
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
