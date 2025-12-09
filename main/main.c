/**
 * main.c
 * 
 * ProxiTof Near-Miss Detection System - Main Application
 * ESP-IDF Framework
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "sensor_manager.h"

static const char *TAG = "MAIN";

void app_main(void)
{
    ESP_LOGI(TAG, "ProxiTof Near-Miss Detection System Starting...");
    ESP_LOGI(TAG, "Framework: ESP-IDF");
    ESP_LOGI(TAG, "Board: ESP32-S3-DevKitC-1");
    
    // Initialize sensor manager
    esp_err_t err = sensor_manager_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Sensor manager initialization failed: %s", esp_err_to_name(err));
        ESP_LOGE(TAG, "System cannot operate without sensors");
        return;
    }
    
    ESP_LOGI(TAG, "Sensor manager initialized successfully");
    
    // Get sensor health status
    sensor_health_t health;
    sensor_manager_get_health(&health);
    ESP_LOGI(TAG, "Sensor Health:");
    ESP_LOGI(TAG, "  ToF: %s (errors: %lu)", 
             health.tof_healthy ? "HEALTHY" : "FAILED", health.tof_error_count);
    ESP_LOGI(TAG, "  IMU: %s (errors: %lu)", 
             health.imu_healthy ? "HEALTHY" : "FAILED", health.imu_error_count);
    
    // Main loop - test sensor reading
    ESP_LOGI(TAG, "Starting sensor read loop...");
    
    tof_data_t tof_data;
    imu_data_t imu_data;
    uint32_t loop_count = 0;
    
    while (1) {
        // Try to read ToF sensor
        err = sensor_manager_read_tof(&tof_data);
        if (err == ESP_OK) {
            // Print first distance value as a test
            ESP_LOGI(TAG, "[%lu] ToF: %u mm (zone 0)", 
                     tof_data.timestamp_ms, tof_data.distances[0]);
            loop_count++;
        } else if (err != ESP_ERR_NOT_FOUND) {
            // Only log if it's not just "no data ready"
            ESP_LOGW(TAG, "ToF read failed: %s", esp_err_to_name(err));
        }
        
        // Try to read IMU sensor
        err = sensor_manager_read_imu(&imu_data);
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "[%lu] IMU: roll=%.2f pitch=%.2f yaw=%.2f", 
                     imu_data.timestamp_ms, imu_data.roll, imu_data.pitch, imu_data.yaw);
        } else if (err != ESP_ERR_NOT_SUPPORTED && err != ESP_ERR_NOT_FOUND) {
            ESP_LOGW(TAG, "IMU read failed: %s", esp_err_to_name(err));
        }
        
        // Check button
        if (sensor_manager_is_button_pressed()) {
            ESP_LOGI(TAG, "*** BUTTON PRESSED - FLAG EVENT ***");
        }
        
        // Print status every 60 loops (~1 second at 60Hz)
        if (loop_count % 60 == 0 && loop_count > 0) {
            ESP_LOGI(TAG, "Status: %lu ToF readings captured", loop_count);
        }
        
        // Delay to maintain ~60Hz loop rate
        vTaskDelay(pdMS_TO_TICKS(16));  // ~60Hz
    }
}
