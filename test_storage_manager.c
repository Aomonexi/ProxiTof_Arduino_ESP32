/**
 * test_storage_manager.c
 * 
 * Simple test for Storage Manager component
 * Add this to your main.c to test SD card functionality
 */

#include "storage_manager.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "STORAGE_TEST";

void test_storage_manager(void) {
    ESP_LOGI(TAG, "=== Storage Manager Test ===");
    
    // Initialize storage manager
    esp_err_t ret = storage_manager_init();
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "SD card not available - test skipped");
        return;
    }
    
    // Start logging
    ret = storage_manager_start_logging();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start logging");
        return;
    }
    
    ESP_LOGI(TAG, "Writing test data...");
    
    // Write some test data
    for (int i = 0; i < 10; i++) {
        log_entry_t entry = {0};
        
        // Fill with test data
        entry.timestamp_ms = i * 1000; // 1 second intervals
        
        // Test ToF data (simulate 500mm distance)
        for (int j = 0; j < 16; j++) {
            entry.tof[j] = 500 + (i * 10) + j; // Varying distances
        }
        
        // Test IMU data
        entry.roll = i * 1.0f;
        entry.pitch = i * 0.5f;
        entry.yaw = i * -0.3f;
        entry.accel_x = 0.1f * i;
        entry.accel_y = 0.2f * i;
        entry.accel_z = 9.8f + (0.1f * i);
        entry.gyro_x = 0.05f * i;
        entry.gyro_y = -0.03f * i;
        entry.gyro_z = 0.02f * i;
        
        // Flag every 5th entry
        entry.flag_manual = (i % 5 == 0) ? 1 : 0;
        
        // Write to storage
        ret = storage_manager_write(&entry);
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "Write failed for entry %d: %s", i, esp_err_to_name(ret));
        }
        
        // Small delay
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
    // Force flush
    storage_manager_flush();
    
    // Get statistics
    storage_stats_t stats;
    storage_manager_get_stats(&stats);
    
    ESP_LOGI(TAG, "=== Storage Statistics ===");
    ESP_LOGI(TAG, "Entries logged: %lu", stats.entries_logged);
    ESP_LOGI(TAG, "Entries dropped: %lu", stats.entries_dropped);
    ESP_LOGI(TAG, "Flush count: %lu", stats.flush_count);
    ESP_LOGI(TAG, "File size: %lu bytes", stats.current_file_size);
    ESP_LOGI(TAG, "Buffer fill: %.1f%%", stats.buffer_fill_percent);
    ESP_LOGI(TAG, "SD card present: %s", stats.sd_card_present ? "Yes" : "No");
    ESP_LOGI(TAG, "Logging active: %s", stats.logging_active ? "Yes" : "No");
    
    // Stop logging
    storage_manager_stop_logging();
    
    ESP_LOGI(TAG, "=== Test Complete ===");
    ESP_LOGI(TAG, "Check SD card for CSV file in /YYYY-MM-DD/ directory");
}

// Add this to your app_main() function:
void app_main(void) {
    // ... your existing initialization ...
    
    // Test storage manager
    test_storage_manager();
    
    // ... rest of your application ...
}