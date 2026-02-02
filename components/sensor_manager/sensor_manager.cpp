/**
 * sensor_manager.cpp
 * 
 * ESP-IDF Sensor Manager Implementation
 * C++ implementation for BNO08x IMU support
 */

extern "C" {
#include "sensor_manager.h"
#include "driver/i2c_master.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"
}

#include <string.h>

// VL53L8CX library (RJRP44)
#include "vl53l8cx_api.h"

// BNO08x IMU library
#include "BNO08x.hpp"

static const char *TAG = "SENSOR_MGR";

// =============================================================================
// Private State
// =============================================================================

// VL53L8CX ToF sensor
static VL53L8CX_Configuration tof_dev;
static bool tof_initialized = false;

// BNO08x IMU sensor
static BNO08x *imu = nullptr;
static bool imu_initialized = false;

// Button state
static uint32_t last_button_press_time = 0;
static uint32_t last_button_read_time = 0;
static bool last_button_state = false;
static uint32_t button_press_start_time = 0;  // When button was first pressed

// Health tracking
static sensor_health_t health = {
    .tof_healthy = false,
    .imu_healthy = false,
    .tof_error_count = 0,
    .imu_error_count = 0
};

// =============================================================================
// Private Helper Functions
// =============================================================================

// I2C master bus handle (new ESP-IDF 5.x API)
static i2c_master_bus_handle_t i2c_bus_handle = NULL;
static i2c_master_bus_config_t i2c_bus_config;  // Store for platform structure

/**
 * Initialize I2C bus for ToF sensor (ESP-IDF 5.x API)
 */
static esp_err_t init_i2c_bus(void)
{
    i2c_bus_config.i2c_port = I2C_NUM_0;
    i2c_bus_config.sda_io_num = TOF_SDA_PIN;
    i2c_bus_config.scl_io_num = TOF_SCL_PIN;
    i2c_bus_config.clk_source = I2C_CLK_SRC_DEFAULT;
    i2c_bus_config.glitch_ignore_cnt = 7;
    i2c_bus_config.flags.enable_internal_pullup = true;
    
    esp_err_t err = i2c_new_master_bus(&i2c_bus_config, &i2c_bus_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2C master bus init failed: %s", esp_err_to_name(err));
        return err;
    }
    
    ESP_LOGI(TAG, "I2C bus initialized (SDA=%d, SCL=%d)", 
             TOF_SDA_PIN, TOF_SCL_PIN);
    ESP_LOGI(TAG, "  Internal pullups: ENABLED");
    return ESP_OK;
}

/**
 * Initialize GPIO for button input
 */
static esp_err_t init_button_gpio(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << FLAG_BUTTON_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    
    esp_err_t err = gpio_config(&io_conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Button GPIO config failed: %s", esp_err_to_name(err));
        return err;
    }
    
    ESP_LOGI(TAG, "Button GPIO initialized (pin=%d)", FLAG_BUTTON_PIN);
    return ESP_OK;
}

/**
 * Scan I2C bus for devices (debugging helper)
 */
static void scan_i2c_bus(void)
{
    ESP_LOGI(TAG, "Scanning I2C bus...");
    int found = 0;
    
    for (uint8_t addr = 1; addr < 127; addr++) {
        i2c_device_config_t dev_cfg = {
            .dev_addr_length = I2C_ADDR_BIT_LEN_7,
            .device_address = addr,
            .scl_speed_hz = I2C_MASTER_FREQ_HZ,
            .scl_wait_us = 0,
            .flags = {0}
        };
        
        i2c_master_dev_handle_t dev_handle;
        esp_err_t ret = i2c_master_bus_add_device(i2c_bus_handle, &dev_cfg, &dev_handle);
        if (ret == ESP_OK) {
            // Try to probe the device
            uint8_t dummy;
            ret = i2c_master_receive(dev_handle, &dummy, 1, 50);
            if (ret == ESP_OK || ret == ESP_ERR_TIMEOUT) {
                ESP_LOGI(TAG, "  Device found at 0x%02X", addr);
                if (addr == 0x29) {
                    ESP_LOGI(TAG, "    ^ VL53L8CX ToF Sensor!");
                }
                found++;
            }
            i2c_master_bus_rm_device(dev_handle);
        }
    }
    
    if (found == 0) {
        ESP_LOGW(TAG, "No I2C devices found!");
        ESP_LOGW(TAG, "Check wiring:");
        ESP_LOGW(TAG, "  - SDA: GPIO %d", TOF_SDA_PIN);
        ESP_LOGW(TAG, "  - SCL: GPIO %d", TOF_SCL_PIN);
        ESP_LOGW(TAG, "  - LPN: GPIO %d", TOF_LPN_PIN);
        ESP_LOGW(TAG, "  - VCC: 3.3V");
        ESP_LOGW(TAG, "  - GND: Connected");
    } else {
        ESP_LOGI(TAG, "Found %d device(s)", found);
    }
}

/**
 * Try to recover ToF sensor if it becomes unresponsive
 */
static esp_err_t recover_tof_sensor(void) {
    ESP_LOGW(TAG, "Attempting ToF sensor recovery...");
    
    // Reset sensor using library function
    VL53L8CX_Reset_Sensor(&(tof_dev.platform));
    vTaskDelay(pdMS_TO_TICKS(200));  // Wait longer for sensor to power up
    
    // Check if sensor is alive
    uint8_t is_alive = 0;
    uint8_t status = vl53l8cx_is_alive(&tof_dev, &is_alive);
    if (status != VL53L8CX_STATUS_OK || !is_alive) {
        ESP_LOGE(TAG, "ToF sensor recovery failed (status=%d, alive=%d)", status, is_alive);
        return ESP_FAIL;
    }
    
    // Restart ranging
    status = vl53l8cx_start_ranging(&tof_dev);
    if (status != VL53L8CX_STATUS_OK) {
        ESP_LOGE(TAG, "Failed to restart ranging after recovery (status=%d)", status);
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "✓ ToF sensor recovery successful");
    health.tof_healthy = true;
    return ESP_OK;
}

/**
 * Initialize VL53L8CX ToF sensor (ESP-IDF 5.x API)
 * Based on working implementation from proxitof_ag_main.cpp
 */
static esp_err_t init_tof_sensor(void)
{
    ESP_LOGI(TAG, "Initializing VL53L8CX ToF sensor...");
    
    // Clear the device structure
    memset(&tof_dev, 0, sizeof(VL53L8CX_Configuration));
    
    // Store bus config in platform structure (CRITICAL for library to work)
    tof_dev.platform.bus_config = i2c_bus_config;
    
    // Configure I2C device with 7-bit address
    // VL53L8CX_DEFAULT_I2C_ADDRESS is 0x52 (8-bit), shift right to get 0x29 (7-bit)
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = VL53L8CX_DEFAULT_I2C_ADDRESS >> 1,  // Convert 8-bit to 7-bit address
        .scl_speed_hz = VL53L8CX_MAX_CLK_SPEED,
        .scl_wait_us = 0,
        .flags = {0}
    };
    
    esp_err_t err = i2c_master_bus_add_device(i2c_bus_handle, &dev_cfg, &tof_dev.platform.handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add I2C device: %s", esp_err_to_name(err));
        health.tof_error_count++;
        return err;
    }
    
    // Set platform parameters
    tof_dev.platform.reset_gpio = TOF_LPN_PIN;
    
    // Reset sensor using library function
    ESP_LOGI(TAG, "Resetting sensor on GPIO%d...", TOF_LPN_PIN);
    VL53L8CX_Reset_Sensor(&(tof_dev.platform));
    vTaskDelay(pdMS_TO_TICKS(100));  // Wait for sensor to power up
    
    // Check if sensor is alive
    ESP_LOGI(TAG, "Checking if sensor is alive...");
    uint8_t is_alive = 0;
    uint8_t status = vl53l8cx_is_alive(&tof_dev, &is_alive);
    if (status != VL53L8CX_STATUS_OK || !is_alive) {
        ESP_LOGE(TAG, "VL53L8CX NOT DETECTED!");
        ESP_LOGE(TAG, "  Status: %d, IsAlive: %d", status, is_alive);
        ESP_LOGE(TAG, "");
        ESP_LOGE(TAG, "Troubleshooting:");
        ESP_LOGE(TAG, "  1. Check I2C wiring (SDA=GPIO%d, SCL=GPIO%d)", TOF_SDA_PIN, TOF_SCL_PIN);
        ESP_LOGE(TAG, "  2. Check power (3.3V, GND)");
        ESP_LOGE(TAG, "  3. Check LPN pin (GPIO%d)", TOF_LPN_PIN);
        ESP_LOGE(TAG, "  4. Try external 2.2k pullups on SDA/SCL");
        ESP_LOGE(TAG, "  5. Check if sensor appears in I2C scan above");
        health.tof_error_count++;
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "✓ VL53L8CX is alive!");
    
    // Initialize sensor firmware
    ESP_LOGI(TAG, "Initializing sensor firmware...");
    status = vl53l8cx_init(&tof_dev);
    if (status != VL53L8CX_STATUS_OK) {
        ESP_LOGE(TAG, "✗ Init failed (status: %d)", status);
        health.tof_error_count++;
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "✓ VL53L8CX ready! (%s)", VL53L8CX_API_REVISION);
    
    // Configure for 4x4 mode at 60Hz
    ESP_LOGI(TAG, "Configuring 4x4 @ 60Hz...");
    
    status = vl53l8cx_set_resolution(&tof_dev, VL53L8CX_RESOLUTION_4X4);
    if (status != VL53L8CX_STATUS_OK) {
        ESP_LOGE(TAG, "Set resolution failed (status=%d)", status);
        health.tof_error_count++;
        return ESP_FAIL;
    }
    
    status = vl53l8cx_set_ranging_frequency_hz(&tof_dev, 60);
    if (status != VL53L8CX_STATUS_OK) {
        ESP_LOGE(TAG, "Set frequency failed (status=%d)", status);
        health.tof_error_count++;
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "✓ Configuration complete");
    
    // Wait for sensor to stabilize after configuration
    ESP_LOGI(TAG, "Waiting for sensor to stabilize...");
    vTaskDelay(pdMS_TO_TICKS(200));  // Increased delay
    
    // Verify sensor is still alive before starting ranging
    ESP_LOGI(TAG, "Verifying sensor before starting ranging...");
    is_alive = 0;
    status = vl53l8cx_is_alive(&tof_dev, &is_alive);
    if (status != VL53L8CX_STATUS_OK || !is_alive) {
        ESP_LOGE(TAG, "Sensor not responding after configuration (status=%d, alive=%d)", status, is_alive);
        health.tof_error_count++;
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "✓ Sensor still alive");
    
    // Start ranging with retry logic
    ESP_LOGI(TAG, "Starting ranging...");
    for (int retry = 0; retry < 3; retry++) {
        status = vl53l8cx_start_ranging(&tof_dev);
        if (status == VL53L8CX_STATUS_OK) {
            break;
        }
        ESP_LOGW(TAG, "Start ranging attempt %d/3 failed (status=%d)", retry + 1, status);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
    if (status != VL53L8CX_STATUS_OK) {
        ESP_LOGE(TAG, "✗ Start ranging failed after 3 attempts (status=%d)", status);
        ESP_LOGE(TAG, "This usually indicates:");
        ESP_LOGE(TAG, "  - I2C communication timeout");
        ESP_LOGE(TAG, "  - Power supply issue (check 3.3V stability with multimeter)");
        ESP_LOGE(TAG, "  - Insufficient current (ToF needs ~20mA during ranging)");
        ESP_LOGE(TAG, "  - Try adding 10uF capacitor near VCC pin");
        health.tof_error_count++;
        return ESP_FAIL;
    }
    
    tof_initialized = true;
    health.tof_healthy = true;
    ESP_LOGI(TAG, "✓ VL53L8CX initialized successfully (4x4 mode, 60Hz)");
    return ESP_OK;
}

/**
 * Initialize BNO08x IMU sensor (Optional - non-fatal)
 * Based on working implementation from ProxiTof_AG
 */
static esp_err_t init_imu_sensor(void)
{
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "=== BNO08x IMU Initialization ===");
    ESP_LOGI(TAG, "Protocol: SPI");
    ESP_LOGI(TAG, "SPI Host: SPI2_HOST (FSPI)");
    ESP_LOGI(TAG, "Pins:");
    ESP_LOGI(TAG, "  MOSI (DI): GPIO%d", IMU_MOSI_PIN);
    ESP_LOGI(TAG, "  MISO (SDA): GPIO%d", IMU_MISO_PIN);
    ESP_LOGI(TAG, "  SCK: GPIO%d", IMU_SCK_PIN);
    ESP_LOGI(TAG, "  CS: GPIO%d", IMU_CS_PIN);
    ESP_LOGI(TAG, "  INT: GPIO%d", IMU_INT_PIN);
    ESP_LOGI(TAG, "  RST: GPIO%d", IMU_RST_PIN);
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "✓ Protocol Config: P0=3.3V, P1=3.3V, BT=floating");
    ESP_LOGI(TAG, "");

    // Create BNO08x config using full constructor
    bno08x_config_t imu_config(SPI2_HOST,     // SPI peripheral (FSPI on ESP32-S3)
                               IMU_MOSI_PIN,  // MOSI pin
                               IMU_MISO_PIN,  // MISO pin
                               IMU_SCK_PIN,   // SCK pin
                               IMU_CS_PIN,    // CS pin
                               IMU_INT_PIN,   // INT pin
                               IMU_RST_PIN,   // RST pin
                               3000000,       // 3 MHz SPI clock
                               true           // Install ISR service
    );

    ESP_LOGI(TAG, "Creating BNO08x object...");
    imu = new BNO08x(imu_config);

    if (imu == nullptr) {
        ESP_LOGE(TAG, "✗ Failed to create BNO08x object");
        health.imu_healthy = false;
        health.imu_error_count++;
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Initializing (takes ~2s, timeout 5s)...");
    
    // Create a flag to track initialization completion
    volatile bool init_complete = false;
    volatile bool init_success = false;
    
    // Create a task to run initialization with timeout protection
    auto init_task = [](void* arg) {
        struct init_params {
            BNO08x* imu;
            volatile bool* complete;
            volatile bool* success;
        };
        init_params* params = (init_params*)arg;
        
        *params->success = params->imu->initialize();
        *params->complete = true;
        vTaskDelete(NULL);
    };
    
    struct init_params {
        BNO08x* imu;
        volatile bool* complete;
        volatile bool* success;
    } params = {imu, &init_complete, &init_success};
    
    TaskHandle_t init_task_handle;
    xTaskCreate(init_task, "imu_init", 4096, &params, 5, &init_task_handle);
    
    // Wait up to 5 seconds for initialization
    uint32_t start_time = sensor_manager_get_timestamp();
    while (!init_complete && (sensor_manager_get_timestamp() - start_time) < 5000) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
    // Check if initialization timed out
    if (!init_complete) {
        ESP_LOGE(TAG, "");
        ESP_LOGE(TAG, "✗ BNO08x initialization TIMEOUT (5 seconds)!");
        ESP_LOGE(TAG, "");
        ESP_LOGE(TAG, "This usually means:");
        ESP_LOGE(TAG, "  1. Sensor is not responding on SPI bus");
        ESP_LOGE(TAG, "  2. MISO line not connected or wrong pin (GPIO%d)", IMU_MISO_PIN);
        ESP_LOGE(TAG, "  3. Protocol pins misconfigured (P0=3.3V, P1=3.3V, BT=float)");
        ESP_LOGE(TAG, "  4. Power supply issue (check 3.3V with multimeter)");
        ESP_LOGE(TAG, "");
        ESP_LOGE(TAG, "💡 Try pressing RESET button to retry initialization");
        ESP_LOGE(TAG, "");
        
        // Kill the stuck task
        vTaskDelete(init_task_handle);
        
        delete imu;
        imu = nullptr;
        health.imu_healthy = false;
        health.imu_error_count++;
        return ESP_FAIL;
    }
    
    // Check if initialization failed
    if (!init_success) {
        ESP_LOGE(TAG, "");
        ESP_LOGE(TAG, "✗ BNO08x initialization FAILED!");
        ESP_LOGE(TAG, "");
        ESP_LOGE(TAG, "Troubleshoot:");
        ESP_LOGE(TAG, "  1. Verify BOTH P0 AND P1 are at 3.3V (use multimeter)");
        ESP_LOGE(TAG, "  2. Verify BT pin is FLOATING (disconnected)");
        ESP_LOGE(TAG, "  3. Check breadboard connections");
        ESP_LOGE(TAG, "  4. Check MISO wiring (GPIO%d)", IMU_MISO_PIN);
        ESP_LOGE(TAG, "");

        delete imu;
        imu = nullptr;
        health.imu_healthy = false;
        health.imu_error_count++;
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

    // Enable calibrated gyroscope
    ESP_LOGI(TAG, "Enabling calibrated gyroscope @ 100Hz...");
    if (!imu->rpt.cal_gyro.enable(10000)) {
        ESP_LOGW(TAG, "Failed to enable calibrated gyroscope");
    }

    imu_initialized = true;
    health.imu_healthy = true;
    ESP_LOGI(TAG, "✓ BNO08x ready!");
    return ESP_OK;
}

// =============================================================================
// Public API Implementation
// =============================================================================

esp_err_t sensor_manager_init(void)
{
    ESP_LOGI(TAG, "Initializing Sensor Manager...");
    
    esp_err_t err;
    bool at_least_one_sensor_ok = false;
    
    // Initialize I2C bus
    err = init_i2c_bus();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize I2C bus");
        return err;
    }
    
    // Initialize button GPIO
    err = init_button_gpio();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize button GPIO");
        // Continue - sensors might still work
    }
    
    // Scan I2C bus to see what's connected
    scan_i2c_bus();
    
    // Initialize ToF sensor (retry up to 3 times)
    for (int retry = 0; retry < 3; retry++) {
        err = init_tof_sensor();
        if (err == ESP_OK) {
            at_least_one_sensor_ok = true;
            break;
        }
        ESP_LOGW(TAG, "ToF init failed, retry %d/3", retry + 1);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    
    // Initialize IMU sensor (retry up to 3 times)
    for (int retry = 0; retry < 3; retry++) {
        err = init_imu_sensor();
        if (err == ESP_OK) {
            at_least_one_sensor_ok = true;
            break;
        }
        if (err == ESP_ERR_NOT_SUPPORTED) {
            // Not implemented yet, don't retry
            break;
        }
        ESP_LOGW(TAG, "IMU init failed, retry %d/3", retry + 1);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    
    if (!at_least_one_sensor_ok) {
        ESP_LOGE(TAG, "No sensors initialized successfully");
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "Sensor Manager initialized (ToF: %s, IMU: %s)",
             health.tof_healthy ? "OK" : "FAIL",
             health.imu_healthy ? "OK" : "FAIL");
    
    return ESP_OK;
}

esp_err_t sensor_manager_read_tof(tof_data_t *data)
{
    if (!tof_initialized || !health.tof_healthy) {
        return ESP_ERR_INVALID_STATE;
    }
    
    if (data == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // Static variables for error tracking
    static uint32_t last_error_log = 0;
    static uint32_t consecutive_errors = 0;
    
    // Check if data is ready
    uint8_t is_ready = 0;
    uint8_t status = vl53l8cx_check_data_ready(&tof_dev, &is_ready);
    if (status != VL53L8CX_STATUS_OK) {
        // Log error but don't increment error count for every failed check
        // Status 133 might indicate I2C communication issues
        uint32_t now = sensor_manager_get_timestamp();
        
        consecutive_errors++;
        
        if (now - last_error_log > 1000) {  // Log error once per second
            ESP_LOGE(TAG, "Failed to check ToF data ready (status=%d, consecutive_errors=%lu)", status, consecutive_errors);
            last_error_log = now;
            
            // Try recovery if we have many consecutive errors
            if (consecutive_errors > 50) {  // About 1 second of errors at high poll rate
                ESP_LOGW(TAG, "Too many consecutive ToF errors, attempting recovery...");
                if (recover_tof_sensor() == ESP_OK) {
                    consecutive_errors = 0;  // Reset error count on successful recovery
                } else {
                    health.tof_healthy = false;
                    health.tof_error_count++;
                }
            }
        }
        return ESP_FAIL;
    }
    
    // Reset consecutive error count on successful status check
    consecutive_errors = 0;
    
    if (!is_ready) {
        return ESP_ERR_NOT_FOUND;  // No new data available (normal at high polling rates)
    }
    
    // Get ranging data
    VL53L8CX_ResultsData results;
    status = vl53l8cx_get_ranging_data(&tof_dev, &results);
    if (status != VL53L8CX_STATUS_OK) {
        ESP_LOGE(TAG, "Failed to get ToF data (status=%d)", status);
        health.tof_error_count++;
        return ESP_FAIL;
    }
    
    // Fill output structure
    data->timestamp_ms = sensor_manager_get_timestamp();
    data->valid = true;
    
    // Copy distance data (4x4 grid in row-major order)
    // Use VL53L8CX_NB_TARGET_PER_ZONE to access correct data index
    // 
    // IMPORTANT: Sensor is mounted backwards, so we flip the matrix:
    // - Flip vertically (up-down): row 0 ↔ row 3, row 1 ↔ row 2
    // - Flip horizontally (left-right): col 0 ↔ col 3, col 1 ↔ col 2
    //
    // Original sensor layout:        Flipped layout (what we want):
    //   0  1  2  3                      15 14 13 12
    //   4  5  6  7                      11 10  9  8
    //   8  9 10 11                       7  6  5  4
    //  12 13 14 15                       3  2  1  0
    //
    for (int row = 0; row < 4; row++) {
        for (int col = 0; col < 4; col++) {
            int sensor_index = row * 4 + col;  // Original sensor index
            
            // Flip both vertically and horizontally
            int flipped_row = 3 - row;  // Flip vertically
            int flipped_col = 3 - col;  // Flip horizontally
            int output_index = flipped_row * 4 + flipped_col;
            
            data->distances[output_index] = results.distance_mm[VL53L8CX_NB_TARGET_PER_ZONE * sensor_index];
        }
    }
    
    return ESP_OK;
}

esp_err_t sensor_manager_read_imu(imu_data_t *data)
{
    if (!imu_initialized || !health.imu_healthy || imu == nullptr) {
        return ESP_ERR_INVALID_STATE;
    }
    
    if (data == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // Check if new data is available
    if (!imu->data_available()) {
        return ESP_ERR_NOT_FOUND;  // No new data
    }
    
    // Fill timestamp
    data->timestamp_ms = sensor_manager_get_timestamp();
    data->valid = true;
    
    // Read rotation vector (orientation)
    if (imu->rpt.rv.has_new_data()) {
        bno08x_euler_angle_t euler = imu->rpt.rv.get_euler(true);
        data->roll = euler.x;
        data->pitch = euler.y;
        data->yaw = euler.z;
    }
    
    // Read linear accelerometer
    if (imu->rpt.linear_accelerometer.has_new_data()) {
        bno08x_accel_t accel = imu->rpt.linear_accelerometer.get();
        data->accel_x = accel.x;
        data->accel_y = accel.y;
        data->accel_z = accel.z;
    }
    
    // Read calibrated gyroscope
    if (imu->rpt.cal_gyro.has_new_data()) {
        bno08x_gyro_t gyro = imu->rpt.cal_gyro.get();
        data->gyro_x = gyro.x;
        data->gyro_y = gyro.y;
        data->gyro_z = gyro.z;
    } else {
        // No new gyro data, set to zero
        data->gyro_x = 0.0f;
        data->gyro_y = 0.0f;
        data->gyro_z = 0.0f;
    }
    
    return ESP_OK;
}

bool sensor_manager_is_button_pressed(void)
{
    uint32_t current_time = sensor_manager_get_timestamp();
    
    // Read button state (active low with pull-up)
    bool current_state = (gpio_get_level(FLAG_BUTTON_PIN) == 0);
    
    // Detect state changes
    if (current_state != last_button_state) {
        // State changed - update tracking
        last_button_read_time = current_time;
        
        // Detect button press (HIGH to LOW transition)
        if (current_state && !last_button_state) {
            button_press_start_time = current_time;
        }
        
        // Detect button release (LOW to HIGH transition) - THIS IS THE TRIGGER!
        if (!current_state && last_button_state) {
            uint32_t press_duration = current_time - button_press_start_time;
            
            // Only trigger if:
            // 1. Button was held for at least debounce time (prevents glitches)
            // 2. Enough time passed since last press (prevents double-triggers)
            if (press_duration >= BUTTON_DEBOUNCE_MS && 
                (current_time - last_button_press_time) >= 150) {  // 150ms between presses
                
                last_button_press_time = current_time;
                last_button_state = current_state;
                ESP_LOGI(TAG, "Button pressed (held for %lu ms)", press_duration);
                return true;
            }
        }
        
        last_button_state = current_state;
    }
    
    return false;
}

uint32_t sensor_manager_get_timestamp(void)
{
    return (uint32_t)(esp_timer_get_time() / 1000);  // Convert µs to ms
}

void sensor_manager_get_health(sensor_health_t *health_out)
{
    if (health_out != NULL) {
        memcpy(health_out, &health, sizeof(sensor_health_t));
    }
}

uint32_t sensor_manager_get_last_button_press_time(void)
{
    return last_button_press_time;
}

bool sensor_manager_is_button_held(void)
{
    // Read current button state (active low with pull-up)
    return (gpio_get_level(FLAG_BUTTON_PIN) == 0);
}

uint32_t sensor_manager_get_button_duration(void)
{
    uint32_t current_time = sensor_manager_get_timestamp();
    bool current_state = (gpio_get_level(FLAG_BUTTON_PIN) == 0);
    
    // If button just pressed and start time not set, set it now
    if (current_state && button_press_start_time == 0) {
        button_press_start_time = current_time;
    }
    
    // If button released, reset start time
    if (!current_state) {
        button_press_start_time = 0;
        return 0;
    }
    
    // Button is pressed, calculate duration
    if (button_press_start_time > 0) {
        return current_time - button_press_start_time;
    }
    
    return 0;
}

i2c_master_bus_handle_t sensor_manager_get_i2c_bus(void)
{
    return i2c_bus_handle;
}

esp_err_t sensor_manager_reinit_imu(void)
{
    ESP_LOGI(TAG, "Reinitializing IMU sensor...");
    
    // Clean up existing IMU instance
    if (imu != nullptr) {
        ESP_LOGI(TAG, "Deleting existing IMU instance...");
        delete imu;
        imu = nullptr;
        imu_initialized = false;
        health.imu_healthy = false;
    }
    
    // Wait a bit for cleanup
    vTaskDelay(pdMS_TO_TICKS(200));
    
    // Reinitialize IMU using the same init function
    esp_err_t err = init_imu_sensor();
    
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "✓ IMU reinitialized successfully");
    } else {
        ESP_LOGE(TAG, "✗ IMU reinitialization failed: %s", esp_err_to_name(err));
    }
    
    return err;
}
