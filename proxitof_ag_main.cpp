/*
 * ProxiTof - Multi-Sensor Fusion System
 * ESP32-S3 - VL53L8CX (I2C) + BNO08x IMU (SPI)
 */

#include "BNO08x.hpp"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_log.h"
#include "esp_task_wdt.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led_strip.h"
#include "vl53l8cx_api.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

// GPIO Definitions - VL53L8CX ToF
#define I2C_SDA_PIN GPIO_NUM_8
#define I2C_SCL_PIN GPIO_NUM_9
#define SENSOR_LPN_PIN GPIO_NUM_4
#define RGB_LED_PIN GPIO_NUM_48

// GPIO Definitions - BNO08x IMU
#define BNO08X_MOSI GPIO_NUM_11
#define BNO08X_MISO GPIO_NUM_13
#define BNO08X_SCK GPIO_NUM_12
#define BNO08X_CS GPIO_NUM_10
#define BNO08X_INT GPIO_NUM_5
#define BNO08X_RST GPIO_NUM_6

// I2C Config
#define I2C_MASTER_NUM 0

// Thresholds
#define CLOSE_THRESHOLD 500
#define MEDIUM_THRESHOLD 2000

static led_strip_handle_t led_strip = NULL;
static BNO08x *imu = nullptr;
static const char *TAG = "ProxiTof";

/**
 * @brief Initialize BNO08x IMU (Optional - non-fatal)
 */
static esp_err_t imu_init(void) {
  ESP_LOGI(TAG, "");
  ESP_LOGI(TAG, "=== BNO08x IMU Initialization ===");
  ESP_LOGI(TAG, "Protocol: SPI");
  ESP_LOGI(TAG, "SPI Host: SPI2_HOST (FSPI)");
  ESP_LOGI(TAG, "Pins:");
  ESP_LOGI(TAG, "  MOSI (DI): GPIO%d", BNO08X_MOSI);
  ESP_LOGI(TAG, "  MISO (SDA): GPIO%d", BNO08X_MISO);
  ESP_LOGI(TAG, "  SCK: GPIO%d", BNO08X_SCK);
  ESP_LOGI(TAG, "  CS: GPIO%d", BNO08X_CS);
  ESP_LOGI(TAG, "  INT: GPIO%d", BNO08X_INT);
  ESP_LOGI(TAG, "  RST: GPIO%d", BNO08X_RST);
  ESP_LOGI(TAG, "");
  ESP_LOGI(TAG, "✓ Protocol Config: P0=3.3V, P1=3.3V, BT=floating");
  ESP_LOGI(TAG, "");

  // Create BNO08x config using full constructor
  bno08x_config_t imu_config(SPI2_HOST,   // SPI peripheral (FSPI on ESP32-S3)
                             BNO08X_MOSI, // MOSI pin
                             BNO08X_MISO, // MISO pin
                             BNO08X_SCK,  // SCK pin
                             BNO08X_CS,   // CS pin
                             BNO08X_INT,  // INT pin
                             BNO08X_RST,  // RST pin
                             3000000,     // 3 MHz SPI clock
                             true         // Install ISR service
  );

  ESP_LOGI(TAG, "Creating BNO08x object...");
  imu = new BNO08x(imu_config);

  if (imu == nullptr) {
    ESP_LOGE(TAG, "✗ Failed to create BNO08x object");
    return ESP_FAIL;
  }

  ESP_LOGI(TAG, "Initializing (takes ~2s)...");

  if (!imu->initialize()) {
    ESP_LOGE(TAG, "");
    ESP_LOGE(TAG, "✗ BNO08x initialization FAILED!");
    ESP_LOGE(TAG, "");
    ESP_LOGE(TAG, "Troubleshoot:");
    ESP_LOGE(TAG, "  1. Verify BOTH P0 AND P1 are at 3.3V (use multimeter)");
    ESP_LOGE(TAG, "  2. Verify BT pin is FLOATING (disconnected)");
    ESP_LOGE(TAG, "  3. Check breadboard connections");
    ESP_LOGE(TAG, "  4. Check MISO wiring (GPIO13)");
    ESP_LOGE(TAG, "");

    delete imu;
    imu = nullptr;
    return ESP_FAIL;
  }

  ESP_LOGI(TAG, "✓ BNO08x initialized successfully!");

  // Enable rotation vector reports (orientation)
  ESP_LOGI(TAG, "Enabling rotation vector @ 100Hz...");
  if (!imu->rpt.rv.enable(10000)) { // 10000us = 10ms = 100Hz
    ESP_LOGW(TAG, "Failed to enable rotation vector");
  }

  // Enable linear accelerometer
  ESP_LOGI(TAG, "Enabling linear accelerometer @ 100Hz...");
  if (!imu->rpt.linear_accelerometer.enable(10000)) {
    ESP_LOGW(TAG, "Failed to enable linear accelerometer");
  }

  ESP_LOGI(TAG, "✓ BNO08x ready!");
  return ESP_OK;
}

/**
 * @brief I2C Bus Scanner
 */
static void scan_i2c_bus(i2c_master_bus_handle_t bus_handle) {
  ESP_LOGI(TAG, "");
  ESP_LOGI(TAG, "=== I2C Bus Scan ===");
  ESP_LOGI(TAG, "Scanning addresses 0x00 - 0x7F...");

  int found_count = 0;
  for (uint8_t addr = 0; addr < 128; addr++) {
    i2c_device_config_t probe_cfg = {};
    probe_cfg.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    probe_cfg.device_address = addr;
    probe_cfg.scl_speed_hz = 100000;

    i2c_master_dev_handle_t probe_handle;
    esp_err_t ret =
        i2c_master_bus_add_device(bus_handle, &probe_cfg, &probe_handle);

    if (ret == ESP_OK) {
      uint8_t dummy;
      ret = i2c_master_receive(probe_handle, &dummy, 1, 100);
      i2c_master_bus_rm_device(probe_handle);

      if (ret == ESP_OK || ret == ESP_ERR_TIMEOUT) {
        ESP_LOGI(TAG, "  Device found at: 0x%02X", addr);
        found_count++;
      }
    }
  }

  ESP_LOGI(TAG, "Scan complete. Found %d device(s)", found_count);
  ESP_LOGI(TAG, "Expected: VL53L8CX at 0x29");
  ESP_LOGI(TAG, "");
}

/**
 * @brief Check GPIO configurations
 */
static void check_gpio_config(void) {
  ESP_LOGI(TAG, "=== GPIO Configuration Check ===");
  ESP_LOGI(TAG, "I2C SDA: GPIO%d", I2C_SDA_PIN);
  ESP_LOGI(TAG, "I2C SCL: GPIO%d", I2C_SCL_PIN);
  ESP_LOGI(TAG, "VL53L8CX LPN: GPIO%d", SENSOR_LPN_PIN);

  const int forbidden_pins[] = {26, 27, 28, 29, 30, 31, 32};
  bool i2c_ok = true;
  for (int i = 0; i < sizeof(forbidden_pins) / sizeof(int); i++) {
    if (I2C_SDA_PIN == forbidden_pins[i] || I2C_SCL_PIN == forbidden_pins[i]) {
      ESP_LOGE(TAG, "ERROR: I2C pin conflicts with flash!");
      i2c_ok = false;
    }
  }

  if (i2c_ok) {
    ESP_LOGI(TAG, "✓ GPIO pins OK for I2C");
  }
  ESP_LOGI(TAG, "");
}

/**
 * @brief Initialize RGB LED
 */
static esp_err_t rgb_led_init(void) {
  led_strip_config_t strip_config = {};
  strip_config.strip_gpio_num = RGB_LED_PIN;
  strip_config.max_leds = 1;
  strip_config.led_pixel_format = LED_PIXEL_FORMAT_GRB;
  strip_config.led_model = LED_MODEL_WS2812;
  strip_config.flags.invert_out = false;

  led_strip_rmt_config_t rmt_config = {};
  rmt_config.clk_src = RMT_CLK_SRC_DEFAULT;
  rmt_config.resolution_hz = 10 * 1000 * 1000;
  rmt_config.flags.with_dma = false;

  ESP_ERROR_CHECK(
      led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
  led_strip_clear(led_strip);

  ESP_LOGI(TAG, "RGB LED initialized on GPIO%d", RGB_LED_PIN);

  // LED test
  ESP_LOGI(TAG, "LED Test: RED");
  led_strip_set_pixel(led_strip, 0, 50, 0, 0);
  led_strip_refresh(led_strip);
  vTaskDelay(pdMS_TO_TICKS(300));

  ESP_LOGI(TAG, "LED Test: YELLOW");
  led_strip_set_pixel(led_strip, 0, 50, 50, 0);
  led_strip_refresh(led_strip);
  vTaskDelay(pdMS_TO_TICKS(300));

  ESP_LOGI(TAG, "LED Test: GREEN");
  led_strip_set_pixel(led_strip, 0, 0, 50, 0);
  led_strip_refresh(led_strip);
  vTaskDelay(pdMS_TO_TICKS(300));

  led_strip_clear(led_strip);
  led_strip_refresh(led_strip);

  ESP_LOGI(TAG, "✓ LED test complete");
  return ESP_OK;
}

/**
 * @brief Update LED based on distance
 */
static void update_led_indicator(int min_distance_mm) {
  if (min_distance_mm < CLOSE_THRESHOLD) {
    led_strip_set_pixel(led_strip, 0, 50, 0, 0); // RED
  } else if (min_distance_mm < MEDIUM_THRESHOLD) {
    led_strip_set_pixel(led_strip, 0, 50, 50, 0); // YELLOW
  } else {
    led_strip_set_pixel(led_strip, 0, 0, 50, 0); // GREEN
  }
  led_strip_refresh(led_strip);
}

/**
 * @brief Find minimum distance
 */
static int get_min_distance(VL53L8CX_ResultsData *Results) {
  int min_dist = 9999;
  for (int i = 0; i < 16; i++) {
    int dist = Results->distance_mm[VL53L8CX_NB_TARGET_PER_ZONE * i];
    if (dist > 0 && dist < min_dist) {
      min_dist = dist;
    }
  }
  return min_dist;
}

/**
 * @brief Display grid with ANSI escape codes for clean in-place updates
 */
static void print_grid(VL53L8CX_ResultsData *Results, int min_dist,
                       uint32_t frame, bool has_imu) {
  // Save cursor position, then move to (1,1) for in-place update
  printf("\0337");     // Save cursor position
  printf("\033[1;1H"); // Move cursor to row 1, column 1

  // Print header - clear each line before writing
  printf("\033["
         "2K╔═══════════════════════════════════════════════════════════╗\r\n");
  printf("\033[2K║         ProxiTof - Real-Time Sensor Fusion                "
         "║\r\n");
  printf("\033["
         "2K╠═══════════════════════════════════════════════════════════╣\r\n");
  printf(
      "\033[2K║ Frame: %-6lu | Min Dist: %4d mm | Rate: 60 Hz          ║\r\n",
      frame, min_dist);
  printf("\033["
         "2K╚═══════════════════════════════════════════════════════════╝\r\n");
  printf("\033[2K\r\n");

  // ToF Grid
  printf("\033[2K┌─────────────────────────────────────────────────┐\r\n");
  printf("\033[2K│      VL53L8CX 4x4 Distance Grid (mm)            │\r\n");
  printf("\033[2K├─────────────────────────────────────────────────┤\r\n");

  for (int row = 0; row < 4; row++) {
    printf("\033[2K│ ");
    for (int col = 0; col < 4; col++) {
      int idx = row * 4 + col;
      int dist = Results->distance_mm[VL53L8CX_NB_TARGET_PER_ZONE * idx];

      // Color code based on distance
      if (dist < CLOSE_THRESHOLD) {
        printf("\033[1;31m"); // Bold red
      } else if (dist < MEDIUM_THRESHOLD) {
        printf("\033[1;33m"); // Bold yellow
      } else {
        printf("\033[1;32m"); // Bold green
      }

      printf("%5d\033[0m ", dist); // Reset color after value
    }
    printf(" │\r\n");
  }

  printf("\033[2K└─────────────────────────────────────────────────┘\r\n");
  printf("\033[2K\r\n");

  // LED Status
  printf("\033[2K┌─────────────────────────────────────────────────┐\r\n");
  const char *led_status;
  const char *led_color;
  if (min_dist < CLOSE_THRESHOLD) {
    led_status = "CLOSE";
    led_color = "\033[1;31m🔴 RED\033[0m";
  } else if (min_dist < MEDIUM_THRESHOLD) {
    led_status = "MEDIUM";
    led_color = "\033[1;33m🟡 YELLOW\033[0m";
  } else {
    led_status = "FAR";
    led_color = "\033[1;32m🟢 GREEN\033[0m";
  }
  printf("\033[2K│ LED: %-8s %s", led_status, led_color);
  printf("                              │\r\n");
  printf("\033[2K└─────────────────────────────────────────────────┘\r\n");

  // IMU Data
  if (has_imu && imu != nullptr && imu->rpt.rv.has_new_data()) {
    bno08x_euler_angle_t euler = imu->rpt.rv.get_euler(true);

    printf("\033[2K\r\n");
    printf("\033[2K┌─────────────────────────────────────────────────┐\r\n");
    printf("\033[2K│         BNO08x IMU - Orientation (°)             │\r\n");
    printf("\033[2K├─────────────────────────────────────────────────┤\r\n");
    printf("\033[2K│ Roll:  %8.2f° │ Pitch: %8.2f° │ Yaw: %8.2f° │\r\n",
           euler.x, euler.y, euler.z);

    if (imu->rpt.linear_accelerometer.has_new_data()) {
      bno08x_accel_t accel = imu->rpt.linear_accelerometer.get();
      printf("\033[2K├─────────────────────────────────────────────────┤\r\n");
      printf("\033[2K│      BNO08x IMU - Linear Accel (m/s²)            │\r\n");
      printf("\033[2K├─────────────────────────────────────────────────┤\r\n");
      printf("\033[2K│   X: %7.2f  │  Y: %7.2f  │  Z: %7.2f      │\r\n",
             accel.x, accel.y, accel.z);
    }

    printf("\033[2K└─────────────────────────────────────────────────┘\r\n");
  } else if (has_imu) {
    printf("\033[2K\r\n");
    printf("\033[2K┌─────────────────────────────────────────────────┐\r\n");
    printf("\033[2K│ BNO08x IMU: Waiting for data...                  │\r\n");
    printf("\033[2K└─────────────────────────────────────────────────┘\r\n");
  }

  printf("\033[2K\r\n");
  printf("\033[2KPress Ctrl+] to exit monitor\r\n");

  // Clear from cursor to end of screen (removes any leftover text)
  printf("\033[J");

  // Restore cursor position and flush
  printf("\0338"); // Restore cursor position
  fflush(stdout);
}

/**
 * @brief Main application
 */
extern "C" void app_main(void) {
  ESP_LOGI(TAG, "");
  ESP_LOGI(TAG, "╔════════════════════════════════════════════╗");
  ESP_LOGI(TAG, "║   ProxiTof Multi-Sensor System            ║");
  ESP_LOGI(TAG, "║   ESP32-S3 + VL53L8CX + BNO08x            ║");
  ESP_LOGI(TAG, "╚════════════════════════════════════════════╝");
  ESP_LOGI(TAG, "");

  // Step 1: GPIO Check
  check_gpio_config();

  // Step 2: LED Test
  ESP_ERROR_CHECK(rgb_led_init());
  ESP_LOGI(TAG, "");

  // Step 3: Try IMU Init (Optional - non-fatal)
  bool imu_available = false;
  esp_err_t imu_err = imu_init();
  if (imu_err == ESP_OK) {
    imu_available = true;
    ESP_LOGI(TAG, "✓ BNO08x IMU ready");
  } else {
    ESP_LOGW(TAG, "✗ BNO08x IMU not available - continuing with ToF only");
    ESP_LOGW(TAG, "  Check wiring: P0=3.3V, P1=3.3V, BT=floating");
  }
  ESP_LOGI(TAG, "");

  // Step 4: I2C Bus Init
  ESP_LOGI(TAG, "=== I2C Bus Initialization ===");

  i2c_master_bus_config_t i2c_mst_config = {};
  i2c_mst_config.clk_source = I2C_CLK_SRC_DEFAULT;
  i2c_mst_config.i2c_port = I2C_MASTER_NUM;
  i2c_mst_config.scl_io_num = I2C_SCL_PIN;
  i2c_mst_config.sda_io_num = I2C_SDA_PIN;
  i2c_mst_config.glitch_ignore_cnt = 7;
  i2c_mst_config.flags.enable_internal_pullup = true;

  i2c_master_bus_handle_t bus_handle;
  esp_err_t ret = i2c_new_master_bus(&i2c_mst_config, &bus_handle);

  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "✗ I2C bus init failed: %s", esp_err_to_name(ret));
    ESP_LOGE(TAG, "Check: Are GPIO8/9 connected correctly?");
    return;
  }

  ESP_LOGI(TAG, "✓ I2C bus initialized at 1MHz");
  ESP_LOGI(TAG, "  SDA: GPIO%d, SCL: GPIO%d", I2C_SDA_PIN, I2C_SCL_PIN);
  ESP_LOGI(TAG, "  Internal pullups: ENABLED");
  ESP_LOGI(TAG, "");

  // Step 5: Scan I2C Bus
  scan_i2c_bus(bus_handle);

  // Step 6: VL53L8CX Initialization
  ESP_LOGI(TAG, "=== VL53L8CX Sensor Init ===");

  i2c_device_config_t dev_cfg = {};
  dev_cfg.dev_addr_length = I2C_ADDR_BIT_LEN_7;
  dev_cfg.device_address = VL53L8CX_DEFAULT_I2C_ADDRESS >> 1;
  dev_cfg.scl_speed_hz = VL53L8CX_MAX_CLK_SPEED;

  uint8_t status, isAlive, isReady;
  VL53L8CX_Configuration Dev;
  VL53L8CX_ResultsData Results;

  memset(&Dev, 0, sizeof(VL53L8CX_Configuration));
  memset(&Results, 0, sizeof(VL53L8CX_ResultsData));

  Dev.platform.bus_config = i2c_mst_config;
  ret = i2c_master_bus_add_device(bus_handle, &dev_cfg, &Dev.platform.handle);

  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "✗ Failed to add I2C device: %s", esp_err_to_name(ret));
    return;
  }

  ESP_LOGI(TAG, "Resetting sensor on GPIO%d...", SENSOR_LPN_PIN);
  Dev.platform.reset_gpio = SENSOR_LPN_PIN;
  VL53L8CX_Reset_Sensor(&(Dev.platform));
  vTaskDelay(pdMS_TO_TICKS(100));

  ESP_LOGI(TAG, "Checking if sensor is alive...");
  status = vl53l8cx_is_alive(&Dev, &isAlive);

  if (!isAlive || status) {
    ESP_LOGE(TAG, "✗ VL53L8CX NOT DETECTED!");
    ESP_LOGE(TAG, "  Status: %d, IsAlive: %d", status, isAlive);
    ESP_LOGE(TAG, "");
    ESP_LOGE(TAG, "Troubleshooting:");
    ESP_LOGE(TAG, "  1. Check I2C wiring (SDA=GPIO8, SCL=GPIO9)");
    ESP_LOGE(TAG, "  2. Check power (3.3V, GND)");
    ESP_LOGE(TAG, "  3. Check LPN pin (GPIO4)");
    ESP_LOGE(TAG, "  4. Try external 2.2k pullups on SDA/SCL");
    ESP_LOGE(TAG, "  5. Check if sensor appears in I2C scan above");
    return;
  }

  ESP_LOGI(TAG, "✓ VL53L8CX is alive!");

  ESP_LOGI(TAG, "Initializing sensor firmware...");
  status = vl53l8cx_init(&Dev);

  if (status) {
    ESP_LOGE(TAG, "✗ Init failed (status: %d)", status);
    return;
  }

  ESP_LOGI(TAG, "✓ VL53L8CX ready! (%s)", VL53L8CX_API_REVISION);

  // Configure
  ESP_LOGI(TAG, "Configuring 4x4 @ 60Hz...");

  status = vl53l8cx_set_resolution(&Dev, VL53L8CX_RESOLUTION_4X4);
  if (status) {
    ESP_LOGE(TAG, "Set resolution failed");
    return;
  }

  status = vl53l8cx_set_ranging_frequency_hz(&Dev, 60);
  if (status) {
    ESP_LOGE(TAG, "Set frequency failed");
    return;
  }

  ESP_LOGI(TAG, "✓ Configuration complete");
  ESP_LOGI(TAG, "");

  // Start ranging
  status = vl53l8cx_start_ranging(&Dev);
  if (status) {
    ESP_LOGE(TAG, "✗ Start ranging failed");
    return;
  }

  ESP_LOGI(TAG, "╔════════════════════════════════════════════╗");
  ESP_LOGI(TAG, "║   System Active - ToF Running @ 60Hz       ║");
  ESP_LOGI(TAG, "╚════════════════════════════════════════════╝");
  ESP_LOGI(TAG, "");
  ESP_LOGI(TAG, "Switching to real-time display in 2 seconds...");
  vTaskDelay(pdMS_TO_TICKS(2000));

  // Clear screen and prepare for ANSI display
  printf("\033[2J"); // Clear screen
  printf("\033[H");  // Home cursor

  uint32_t frame_count = 0;

  // Enable watchdog with 5 second timeout
  esp_task_wdt_config_t wdt_config = {
      .timeout_ms = 5000, .idle_core_mask = 0, .trigger_panic = true};
  esp_task_wdt_init(&wdt_config);
  esp_task_wdt_add(NULL); // Add current task to watchdog

  while (1) {
    // Reset watchdog
    esp_task_wdt_reset();

    // Poll IMU data (non-blocking)
    if (imu_available && imu != nullptr) {
      if (imu->data_available()) {
        // Data is fresh - will be displayed in next ToF update
      }
    }

    // Check ToF data
    status = vl53l8cx_check_data_ready(&Dev, &isReady);

    if (isReady) {
      vl53l8cx_get_ranging_data(&Dev, &Results);

      int min_dist = get_min_distance(&Results);
      update_led_indicator(min_dist);
      print_grid(&Results, min_dist, ++frame_count, imu_available);
    }

    // Small delay
    vTaskDelay(pdMS_TO_TICKS(5));
  }
}
