/**
 * storage_manager.c
 * 
 * ESP-IDF Storage Manager Implementation
 * Circular buffering + SD card logging via ESP-IDF VFS
 */

#include "storage_manager.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"
#include <string.h>
#include <sys/stat.h>
#include <time.h>

static const char *TAG = "STORAGE_MGR";

// =============================================================================
// Circular Buffer Implementation
// =============================================================================

typedef struct {
    log_entry_t data[STORAGE_BUFFER_SIZE];
    size_t head;
    size_t tail;
    size_t count;
} circular_buffer_t;

static circular_buffer_t buffer = {0};

static inline bool buffer_is_full(void) {
    return buffer.count >= STORAGE_BUFFER_SIZE;
}

static inline bool buffer_is_empty(void) {
    return buffer.count == 0;
}

static inline size_t buffer_size(void) {
    return buffer.count;
}

static inline float buffer_fill_percent(void) {
    return (buffer.count * 100.0f) / STORAGE_BUFFER_SIZE;
}

static bool buffer_push(const log_entry_t *entry) {
    if (buffer_is_full()) {
        return false;
    }
    
    memcpy(&buffer.data[buffer.head], entry, sizeof(log_entry_t));
    buffer.head = (buffer.head + 1) % STORAGE_BUFFER_SIZE;
    buffer.count++;
    
    return true;
}

static bool buffer_pop(log_entry_t *entry) {
    if (buffer_is_empty()) {
        return false;
    }
    
    memcpy(entry, &buffer.data[buffer.tail], sizeof(log_entry_t));
    buffer.tail = (buffer.tail + 1) % STORAGE_BUFFER_SIZE;
    buffer.count--;
    
    return true;
}

static void buffer_clear(void) {
    buffer.head = 0;
    buffer.tail = 0;
    buffer.count = 0;
}

// =============================================================================
// SD Card State
// =============================================================================

static sdmmc_card_t *card = NULL;
static FILE *log_file = NULL;
static bool sd_initialized = false;
static bool logging_active = false;

// Statistics
static storage_stats_t stats = {0};

// Current file info
static char current_filename[128] = {0};  // Increased size to prevent truncation
static uint32_t current_file_size = 0;

// =============================================================================
// SD Card Initialization
// =============================================================================

esp_err_t storage_manager_init(void) {
    ESP_LOGI(TAG, "Initializing SD card via SDIO (4-bit mode)...");
    ESP_LOGW(TAG, "⚠️  SDIO hardware not connected - expect initialization to fail");
    ESP_LOGW(TAG, "⚠️  This is normal if SD card breakout is not installed yet");
    
    // Options for mounting the filesystem
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };
    
    // Use SDMMC host (native SD protocol - much faster and more reliable than SPI)
    // ESP32-S3 SDMMC pins:
    //   CLK:  GPIO 39
    //   CMD:  GPIO 38
    //   D0:   GPIO 40
    //   D1:   GPIO 41 (4-bit mode)
    //   D2:   GPIO 42 (4-bit mode)
    //   D3:   GPIO 43 (4-bit mode)
    
    ESP_LOGI(TAG, "Using SDMMC host (dedicated pins, no sharing with IMU)");
    ESP_LOGI(TAG, "Pins: CLK=39, CMD=38, D0=40, D1=41, D2=42, D3=43");
    
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    
    // Reduce frequency for better compatibility
    host.max_freq_khz = SDMMC_FREQ_DEFAULT;  // 20MHz (can go up to 40MHz later)
    
    // Configure SDMMC slot (4-bit mode for maximum speed)
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    
    // Use 4-bit mode for faster transfers
    slot_config.width = 4;
    
    // Enable internal pull-ups (Adafruit board also has pull-ups, so this is safe)
    slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;
    
    ESP_LOGI(TAG, "Attempting to mount filesystem (this may take a few seconds)...");
    
    // Try to mount with timeout handling
    esp_err_t ret = esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot_config, &mount_config, &card);
    
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGW(TAG, "❌ Failed to mount filesystem - SD card not detected or not formatted as FAT32");
        } else if (ret == ESP_ERR_TIMEOUT) {
            ESP_LOGW(TAG, "❌ SD card initialization timeout - hardware not connected");
        } else {
            ESP_LOGW(TAG, "❌ Failed to initialize SD card: %s", esp_err_to_name(ret));
        }
        ESP_LOGW(TAG, "");
        ESP_LOGW(TAG, "💡 System will continue WITHOUT data logging");
        ESP_LOGW(TAG, "💡 Sensors and LED will work normally");
        ESP_LOGW(TAG, "");
        stats.sd_card_present = false;
        sd_initialized = false;
        return ret;
    }
    
    ESP_LOGI(TAG, "✓ Filesystem mounted successfully");
    
    // Print card info
    sdmmc_card_print_info(stdout, card);
    
    sd_initialized = true;
    stats.sd_card_present = true;
    
    ESP_LOGI(TAG, "✓ SD card mounted at /sdcard");
    
    // Clear buffer
    buffer_clear();
    
    return ESP_OK;
}

// =============================================================================
// File Management
// =============================================================================

static esp_err_t create_date_directory(char *dir_path, size_t max_len) {
    time_t now;
    struct tm timeinfo;
    
    time(&now);
    localtime_r(&now, &timeinfo);
    
    // Format: /sdcard/YYYY-MM-DD
    snprintf(dir_path, max_len, "/sdcard/%04d-%02d-%02d",
             timeinfo.tm_year + 1900,
             timeinfo.tm_mon + 1,
             timeinfo.tm_mday);
    
    // Create directory if it doesn't exist
    struct stat st = {0};
    if (stat(dir_path, &st) == -1) {
        if (mkdir(dir_path, 0700) != 0) {
            ESP_LOGE(TAG, "Failed to create directory: %s", dir_path);
            return ESP_FAIL;
        }
        ESP_LOGI(TAG, "Created directory: %s", dir_path);
    }
    
    return ESP_OK;
}

static esp_err_t create_log_file(void) {
    char dir_path[64];
    
    // Create date directory
    esp_err_t ret = create_date_directory(dir_path, sizeof(dir_path));
    if (ret != ESP_OK) {
        return ret;
    }
    
    // Generate filename with timestamp
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    
    snprintf(current_filename, sizeof(current_filename),
             "%s/log_%04d%02d%02d_%02d%02d%02d.csv",
             dir_path,
             timeinfo.tm_year + 1900,
             timeinfo.tm_mon + 1,
             timeinfo.tm_mday,
             timeinfo.tm_hour,
             timeinfo.tm_min,
             timeinfo.tm_sec);
    
    // Open file for writing
    log_file = fopen(current_filename, "w");
    if (log_file == NULL) {
        ESP_LOGE(TAG, "Failed to create log file: %s", current_filename);
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "Created log file: %s", current_filename);
    
    // Write CSV header
    fprintf(log_file, "timestamp_ms");
    
    // ToF columns (tof_00 to tof_33)
    for (int row = 0; row < 4; row++) {
        for (int col = 0; col < 4; col++) {
            fprintf(log_file, ",tof_%d%d", row, col);
        }
    }
    
    // IMU columns
    fprintf(log_file, ",roll,pitch,yaw");
    fprintf(log_file, ",accel_x,accel_y,accel_z");
    fprintf(log_file, ",gyro_x,gyro_y,gyro_z");
    
    // Flag column
    fprintf(log_file, ",flag_manual");
    
    fprintf(log_file, "\n");
    fflush(log_file);
    
    current_file_size = ftell(log_file);
    
    return ESP_OK;
}

// =============================================================================
// Logging Operations
// =============================================================================

esp_err_t storage_manager_start_logging(void) {
    if (!sd_initialized) {
        ESP_LOGE(TAG, "SD card not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    
    if (logging_active) {
        ESP_LOGW(TAG, "Logging already active");
        return ESP_OK;
    }
    
    // Create new log file
    esp_err_t ret = create_log_file();
    if (ret != ESP_OK) {
        return ret;
    }
    
    // Reset statistics
    stats.entries_logged = 0;
    stats.entries_dropped = 0;
    stats.flush_count = 0;
    
    logging_active = true;
    stats.logging_active = true;
    
    ESP_LOGI(TAG, "Logging started");
    
    return ESP_OK;
}

esp_err_t storage_manager_stop_logging(void) {
    if (!logging_active) {
        return ESP_OK;
    }
    
    // Flush remaining buffer
    storage_manager_flush();
    
    // Close file
    if (log_file != NULL) {
        fclose(log_file);
        log_file = NULL;
    }
    
    logging_active = false;
    stats.logging_active = false;
    
    ESP_LOGI(TAG, "Logging stopped. Total entries: %lu, Dropped: %lu",
             stats.entries_logged, stats.entries_dropped);
    
    return ESP_OK;
}

esp_err_t storage_manager_write(const log_entry_t *entry) {
    if (!logging_active) {
        return ESP_ERR_INVALID_STATE;
    }
    
    if (entry == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // Try to add to buffer
    if (!buffer_push(entry)) {
        stats.entries_dropped++;
        return ESP_ERR_NO_MEM;
    }
    
    // Check if we should flush
    if (buffer_fill_percent() >= STORAGE_FLUSH_THRESHOLD) {
        storage_manager_flush();
    }
    
    return ESP_OK;
}

esp_err_t storage_manager_flush(void) {
    if (!logging_active || log_file == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    
    uint32_t flushed = 0;
    log_entry_t entry;
    
    // Write all buffered entries
    while (buffer_pop(&entry)) {
        // Write CSV row
        fprintf(log_file, "%lu", entry.timestamp_ms);
        
        // ToF data
        for (int i = 0; i < 16; i++) {
            fprintf(log_file, ",%u", entry.tof[i]);
        }
        
        // IMU data
        fprintf(log_file, ",%.2f,%.2f,%.2f", entry.roll, entry.pitch, entry.yaw);
        fprintf(log_file, ",%.2f,%.2f,%.2f", entry.accel_x, entry.accel_y, entry.accel_z);
        fprintf(log_file, ",%.2f,%.2f,%.2f", entry.gyro_x, entry.gyro_y, entry.gyro_z);
        
        // Flag
        fprintf(log_file, ",%u", entry.flag_manual);
        
        fprintf(log_file, "\n");
        
        flushed++;
        stats.entries_logged++;
    }
    
    if (flushed > 0) {
        fflush(log_file);
        stats.flush_count++;
        
        // Update file size
        current_file_size = ftell(log_file);
        stats.current_file_size = current_file_size;
        
        // Check if file rotation needed
        if (current_file_size >= STORAGE_MAX_FILE_SIZE) {
            ESP_LOGI(TAG, "File size limit reached, rotating...");
            fclose(log_file);
            create_log_file();
        }
    }
    
    return ESP_OK;
}

void storage_manager_get_stats(storage_stats_t *out_stats) {
    if (out_stats != NULL) {
        memcpy(out_stats, &stats, sizeof(storage_stats_t));
        out_stats->buffer_fill_percent = buffer_fill_percent();
    }
}

bool storage_manager_is_logging(void) {
    return logging_active;
}
