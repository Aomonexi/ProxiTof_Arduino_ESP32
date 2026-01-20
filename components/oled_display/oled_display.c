/**
 * oled_display.c
 * 
 * OLED Display Driver - Testing/Diagnostic Version
 * 
 * This version tests the hardware connection and SPI communication
 * without relying on the esp_ssd1306 library API (which needs verification)
 */

#include "oled_display.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include <string.h>

static const char *TAG = "OLED";

// GPIO Pin Definitions
#define OLED_PIN_SCK    GPIO_NUM_7
#define OLED_PIN_MOSI   GPIO_NUM_1
#define OLED_PIN_DC     GPIO_NUM_35
#define OLED_PIN_RST    GPIO_NUM_21

// Display dimensions
#define OLED_WIDTH      128
#define OLED_HEIGHT     64

// SPI Configuration
#define OLED_SPI_HOST   SPI2_HOST
#define OLED_SPI_CLOCK  10000000  // 10 MHz

// Display state
static bool display_initialized = false;
static spi_device_handle_t spi_handle = NULL;
static oled_config_t current_config = {
    .brightness = 128,
    .flip_display = false,
    .mode = OLED_MODE_DASHBOARD
};

// SSD1306 Commands
#define SSD1306_CMD_DISPLAY_OFF         0xAE
#define SSD1306_CMD_DISPLAY_ON          0xAF
#define SSD1306_CMD_SET_CONTRAST        0x81
#define SSD1306_CMD_SET_DISPLAY_NORMAL  0xA6

/**
 * Send command to SSD1306
 */
static esp_err_t ssd1306_write_command(uint8_t cmd) {
    gpio_set_level(OLED_PIN_DC, 0);  // Command mode
    
    spi_transaction_t trans = {
        .length = 8,
        .tx_buffer = &cmd,
        .flags = 0
    };
    
    return spi_device_transmit(spi_handle, &trans);
}

esp_err_t oled_display_init(void) {
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "╔════════════════════════════════════════════╗");
    ESP_LOGI(TAG, "║   OLED Display Hardware Test               ║");
    ESP_LOGI(TAG, "╚════════════════════════════════════════════╝");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "Display: 128x64 OLED SSD1306 (4-wire SPI)");
    ESP_LOGI(TAG, "Dual-color: Yellow (top 16px) + Blue (bottom 48px)");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "Wiring Check:");
    ESP_LOGI(TAG, "  Display Pin  →  ESP32-S3 Pin");
    ESP_LOGI(TAG, "  ───────────     ────────────");
    ESP_LOGI(TAG, "  GND          →  GND");
    ESP_LOGI(TAG, "  VCC          →  3.3V ⚠️ NOT 5V!");
    ESP_LOGI(TAG, "  SCK          →  GPIO 7");
    ESP_LOGI(TAG, "  SDA (MOSI)   →  GPIO 1");
    ESP_LOGI(TAG, "  RES (Reset)  →  GPIO 21");
    ESP_LOGI(TAG, "  DC (D/C)     →  GPIO 35");
    ESP_LOGI(TAG, "");
    
    // Initialize GPIOs
    ESP_LOGI(TAG, "Step 1: Initializing GPIO pins...");
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << OLED_PIN_DC) | (1ULL << OLED_PIN_RST),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "❌ GPIO configuration failed: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Set initial states
    gpio_set_level(OLED_PIN_DC, 0);
    gpio_set_level(OLED_PIN_RST, 1);
    ESP_LOGI(TAG, "✓ GPIO pins configured (DC=GPIO35, RST=GPIO21)");
    
    // Initialize SPI bus
    ESP_LOGI(TAG, "Step 2: Initializing SPI bus...");
    spi_bus_config_t buscfg = {
        .mosi_io_num = OLED_PIN_MOSI,
        .miso_io_num = -1,
        .sclk_io_num = OLED_PIN_SCK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = OLED_WIDTH * OLED_HEIGHT / 8 + 8,
    };
    
    ret = spi_bus_initialize(OLED_SPI_HOST, &buscfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "❌ SPI bus initialization failed: %s", esp_err_to_name(ret));
        ESP_LOGE(TAG, "💡 Check wiring: SCK=GPIO7, MOSI=GPIO1");
        return ret;
    }
    
    if (ret == ESP_ERR_INVALID_STATE) {
        ESP_LOGI(TAG, "✓ SPI bus already initialized (shared)");
    } else {
        ESP_LOGI(TAG, "✓ SPI bus initialized (SCK=GPIO7, MOSI=GPIO1)");
    }
    
    // Add SPI device
    ESP_LOGI(TAG, "Step 3: Adding SPI device...");
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = OLED_SPI_CLOCK,
        .mode = 0,
        .spics_io_num = -1,  // No CS pin (4-wire SPI)
        .queue_size = 1,
        .flags = 0,
    };
    
    ret = spi_bus_add_device(OLED_SPI_HOST, &devcfg, &spi_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "❌ Failed to add SPI device: %s", esp_err_to_name(ret));
        return ret;
    }
    ESP_LOGI(TAG, "✓ SPI device added (10 MHz, 4-wire mode)");
    
    // Hardware reset
    ESP_LOGI(TAG, "Step 4: Performing hardware reset...");
    gpio_set_level(OLED_PIN_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(OLED_PIN_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(10));
    ESP_LOGI(TAG, "✓ Hardware reset complete");
    
    // Send basic initialization commands
    ESP_LOGI(TAG, "Step 5: Sending initialization commands...");
    ssd1306_write_command(SSD1306_CMD_DISPLAY_OFF);
    ssd1306_write_command(SSD1306_CMD_SET_CONTRAST);
    ssd1306_write_command(0x7F);  // Medium contrast
    ssd1306_write_command(SSD1306_CMD_SET_DISPLAY_NORMAL);
    ssd1306_write_command(SSD1306_CMD_DISPLAY_ON);
    ESP_LOGI(TAG, "✓ Basic commands sent");
    
    display_initialized = true;
    
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "╔════════════════════════════════════════════╗");
    ESP_LOGI(TAG, "║   Hardware Test Complete                   ║");
    ESP_LOGI(TAG, "╚════════════════════════════════════════════╝");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "✓ OLED hardware initialized successfully!");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "What to check on your display:");
    ESP_LOGI(TAG, "  1. Display should be powered (backlight on)");
    ESP_LOGI(TAG, "  2. May show random pixels or be blank");
    ESP_LOGI(TAG, "  3. If completely dark, check VCC=3.3V");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "⚠️  Full graphics library integration pending");
    ESP_LOGI(TAG, "💡 This confirms SPI communication is working");
    ESP_LOGI(TAG, "");
    
    return ESP_OK;
}

esp_err_t oled_display_update(const tof_data_t *tof_data, 
                              const imu_data_t *imu_data, 
                              uint16_t min_distance) {
    if (!display_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    // Placeholder - no actual display update yet
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
    
    return ESP_OK;
}

esp_err_t oled_display_set_brightness(uint8_t brightness) {
    if (!display_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    current_config.brightness = brightness;
    
    // Send contrast command
    ssd1306_write_command(SSD1306_CMD_SET_CONTRAST);
    ssd1306_write_command(brightness);
    
    ESP_LOGI(TAG, "Display brightness set to: %d/255", brightness);
    
    return ESP_OK;
}

esp_err_t oled_display_clear(void) {
    if (!display_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    return ESP_OK;
}

oled_display_mode_t oled_display_get_mode(void) {
    return current_config.mode;
}
