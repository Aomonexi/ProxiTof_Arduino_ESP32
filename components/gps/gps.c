/**
 * gps.c
 * 
 * Neo-M9N GPS Module Driver Implementation
 * Parses NMEA sentences: GPGGA, GPRMC, GPVTG
 */

#include "gps.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>

static const char *TAG = "GPS";

#define GPS_UART_BUF_SIZE 1024
#define GPS_SENTENCE_MAX_LEN 128
#define GPS_TASK_STACK_SIZE 4096
#define GPS_TASK_PRIORITY 5

// GPS module state
static struct {
    int uart_num;
    bool initialized;
    TaskHandle_t task_handle;
    gps_data_t current_data;
    gps_stats_t stats;
    uint32_t init_time_ms;
} gps_state = {0};

/**
 * Calculate NMEA checksum
 */
static uint8_t nmea_checksum(const char *sentence, int len)
{
    uint8_t checksum = 0;
    for (int i = 1; i < len && sentence[i] != '*'; i++) {
        checksum ^= sentence[i];
    }
    return checksum;
}

/**
 * Verify NMEA sentence checksum
 */
static bool nmea_verify_checksum(const char *sentence, int len)
{
    // Find checksum position
    const char *checksum_pos = strchr(sentence, '*');
    if (!checksum_pos || checksum_pos >= sentence + len - 2) {
        return false;
    }
    
    // Calculate expected checksum
    uint8_t calculated = nmea_checksum(sentence, checksum_pos - sentence);
    
    // Parse received checksum
    uint8_t received = 0;
    sscanf(checksum_pos + 1, "%02hhx", &received);
    
    return calculated == received;
}

/**
 * Convert NMEA coordinate to decimal degrees
 * Format: DDMM.MMMM (latitude) or DDDMM.MMMM (longitude)
 */
static double nmea_to_decimal(const char *coord, char direction, bool is_longitude)
{
    if (!coord || strlen(coord) < 4) {
        return 0.0;
    }
    
    // Extract degrees
    int degrees_len = is_longitude ? 3 : 2;
    char degrees_str[4] = {0};
    strncpy(degrees_str, coord, degrees_len);
    int degrees = atoi(degrees_str);
    
    // Extract minutes
    double minutes = atof(coord + degrees_len);
    
    // Convert to decimal degrees
    double decimal = degrees + (minutes / 60.0);
    
    // Apply direction (S and W are negative)
    if (direction == 'S' || direction == 'W') {
        decimal = -decimal;
    }
    
    return decimal;
}

/**
 * Parse GPGGA sentence (Position fix data)
 * $GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47
 */
static void parse_gpgga(const char *sentence)
{
    char time_str[16] = {0};
    char lat_str[16] = {0};
    char lat_dir = 0;
    char lon_str[16] = {0};
    char lon_dir = 0;
    int fix_quality = 0;
    int satellites = 0;
    float hdop = 0.0f;
    float altitude = 0.0f;
    
    int parsed = sscanf(sentence, "$GPGGA,%[^,],%[^,],%c,%[^,],%c,%d,%d,%f,%f,M",
                       time_str, lat_str, &lat_dir, lon_str, &lon_dir,
                       &fix_quality, &satellites, &hdop, &altitude);
    
    if (parsed >= 9) {
        // Parse time (HHMMSS.sss)
        if (strlen(time_str) >= 6) {
            gps_state.current_data.hour = (time_str[0] - '0') * 10 + (time_str[1] - '0');
            gps_state.current_data.minute = (time_str[2] - '0') * 10 + (time_str[3] - '0');
            gps_state.current_data.second = (time_str[4] - '0') * 10 + (time_str[5] - '0');
            
            // Parse milliseconds if present
            if (strlen(time_str) > 7) {
                gps_state.current_data.millisecond = (uint16_t)(atof(time_str + 6) * 1000);
            }
        }
        
        // Parse position
        gps_state.current_data.latitude = nmea_to_decimal(lat_str, lat_dir, false);
        gps_state.current_data.longitude = nmea_to_decimal(lon_str, lon_dir, true);
        gps_state.current_data.altitude_m = altitude;
        
        // Parse fix quality
        gps_state.current_data.fix_quality = (gps_fix_quality_t)fix_quality;
        gps_state.current_data.satellites_used = satellites;
        gps_state.current_data.hdop = hdop;
        
        // Update validity
        gps_state.current_data.valid = (fix_quality > 0);
        
        if (gps_state.current_data.valid) {
            gps_state.stats.valid_fixes++;
            
            // Record first fix time
            if (!gps_state.stats.first_fix_acquired) {
                gps_state.stats.first_fix_acquired = true;
                gps_state.stats.time_to_first_fix_s = 
                    (xTaskGetTickCount() * portTICK_PERIOD_MS - gps_state.init_time_ms) / 1000;
                ESP_LOGI(TAG, "✓ First GPS fix acquired in %lu seconds", 
                         gps_state.stats.time_to_first_fix_s);
            }
        }
    }
}

/**
 * Parse GPRMC sentence (Recommended minimum data)
 * $GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A
 */
static void parse_gprmc(const char *sentence)
{
    char time_str[16] = {0};
    char status = 0;
    char lat_str[16] = {0};
    char lat_dir = 0;
    char lon_str[16] = {0};
    char lon_dir = 0;
    float speed_knots = 0.0f;
    float heading = 0.0f;
    char date_str[16] = {0};
    
    int parsed = sscanf(sentence, "$GPRMC,%[^,],%c,%[^,],%c,%[^,],%c,%f,%f,%[^,]",
                       time_str, &status, lat_str, &lat_dir, lon_str, &lon_dir,
                       &speed_knots, &heading, date_str);
    
    if (parsed >= 9 && status == 'A') {  // 'A' = Active/Valid
        // Parse date (DDMMYY)
        if (strlen(date_str) >= 6) {
            gps_state.current_data.day = (date_str[0] - '0') * 10 + (date_str[1] - '0');
            gps_state.current_data.month = (date_str[2] - '0') * 10 + (date_str[3] - '0');
            gps_state.current_data.year = 2000 + (date_str[4] - '0') * 10 + (date_str[5] - '0');
        }
        
        // Parse speed (knots to km/h and m/s)
        gps_state.current_data.speed_kmh = speed_knots * 1.852f;  // 1 knot = 1.852 km/h
        gps_state.current_data.speed_ms = speed_knots * 0.514444f; // 1 knot = 0.514444 m/s
        
        // Parse heading
        gps_state.current_data.heading_deg = heading;
    }
}

/**
 * Parse GPVTG sentence (Track made good and ground speed)
 * $GPVTG,054.7,T,034.4,M,005.5,N,010.2,K*48
 */
static void parse_gpvtg(const char *sentence)
{
    float heading_true = 0.0f;
    float speed_knots = 0.0f;
    float speed_kmh = 0.0f;
    
    int parsed = sscanf(sentence, "$GPVTG,%f,T,%*f,M,%f,N,%f,K",
                       &heading_true, &speed_knots, &speed_kmh);
    
    if (parsed >= 3) {
        gps_state.current_data.heading_deg = heading_true;
        gps_state.current_data.speed_kmh = speed_kmh;
        gps_state.current_data.speed_ms = speed_kmh / 3.6f;  // km/h to m/s
    }
}

/**
 * Parse NMEA sentence
 */
static void parse_nmea_sentence(const char *sentence, int len)
{
    // Verify checksum
    if (!nmea_verify_checksum(sentence, len)) {
        gps_state.stats.checksum_errors++;
        return;
    }
    
    gps_state.stats.sentences_parsed++;
    
    // Parse based on sentence type
    if (strncmp(sentence, "$GPGGA", 6) == 0 || strncmp(sentence, "$GNGGA", 6) == 0) {
        parse_gpgga(sentence);
    } else if (strncmp(sentence, "$GPRMC", 6) == 0 || strncmp(sentence, "$GNRMC", 6) == 0) {
        parse_gprmc(sentence);
    } else if (strncmp(sentence, "$GPVTG", 6) == 0 || strncmp(sentence, "$GNVTG", 6) == 0) {
        parse_gpvtg(sentence);
    }
    
    // Update timestamp
    gps_state.current_data.last_update_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
}

/**
 * GPS UART reading task
 */
static void gps_task(void *arg)
{
    uint8_t *data = malloc(GPS_UART_BUF_SIZE);
    if (!data) {
        ESP_LOGE(TAG, "Failed to allocate UART buffer");
        vTaskDelete(NULL);
        return;
    }
    
    char sentence[GPS_SENTENCE_MAX_LEN] = {0};
    int sentence_pos = 0;
    
    ESP_LOGI(TAG, "GPS task started, waiting for NMEA sentences...");
    
    uint32_t last_debug_time = 0;
    uint32_t bytes_received_total = 0;
    uint32_t no_data_count = 0;
    
    while (1) {
        // Read data from UART
        int len = uart_read_bytes(gps_state.uart_num, data, GPS_UART_BUF_SIZE - 1, 
                                  pdMS_TO_TICKS(100));
        
        if (len > 0) {
            data[len] = '\0';
            bytes_received_total += len;
            no_data_count = 0;  // Reset no-data counter
            
            // Debug: Print raw NMEA data every 5 seconds
            uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
            if (now - last_debug_time > 5000) {
                ESP_LOGI(TAG, "📡 GPS UART active: %d bytes received (total: %lu)", len, bytes_received_total);
                
                // Print first 50 bytes as hex to see what we're actually receiving
                ESP_LOG_BUFFER_HEXDUMP(TAG, data, len > 50 ? 50 : len, ESP_LOG_INFO);
                
                last_debug_time = now;
            }
            
            // Process byte by byte
            for (int i = 0; i < len; i++) {
                char c = data[i];
                
                // Start of sentence
                if (c == '$') {
                    sentence_pos = 0;
                    sentence[sentence_pos++] = c;
                }
                // End of sentence
                else if (c == '\n' || c == '\r') {
                    if (sentence_pos > 0) {
                        sentence[sentence_pos] = '\0';
                        parse_nmea_sentence(sentence, sentence_pos);
                        sentence_pos = 0;
                    }
                }
                // Middle of sentence
                else if (sentence_pos > 0 && sentence_pos < GPS_SENTENCE_MAX_LEN - 1) {
                    sentence[sentence_pos++] = c;
                }
            }
        } else {
            // No data received
            no_data_count++;
            
            // Warn if no data for 5 seconds (50 iterations × 100ms)
            if (no_data_count == 50) {
                ESP_LOGW(TAG, "⚠️  No GPS data received for 5 seconds");
                ESP_LOGW(TAG, "💡 Check: 1) GPS powered? 2) TX/RX wiring? 3) Baud rate (115200)?");
            }
            
            // Periodic warning every 30 seconds
            if (no_data_count % 300 == 0 && no_data_count > 0) {
                ESP_LOGW(TAG, "⚠️  Still no GPS data after %lu seconds", no_data_count / 10);
            }
        }
        
        // Small delay to prevent task starvation
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    
    free(data);
    vTaskDelete(NULL);
}

/**
 * Initialize GPS module
 */
esp_err_t gps_init(int uart_num, int tx_pin, int rx_pin, uint32_t baud_rate)
{
    if (gps_state.initialized) {
        ESP_LOGW(TAG, "GPS already initialized");
        return ESP_OK;
    }
    
    ESP_LOGI(TAG, "Initializing GPS on UART%d (TX=%d, RX=%d, baud=%lu)", 
             uart_num, tx_pin, rx_pin, baud_rate);
    
    // Configure UART
    uart_config_t uart_config = {
        .baud_rate = baud_rate,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    
    esp_err_t err = uart_driver_install(uart_num, GPS_UART_BUF_SIZE * 2, 0, 0, NULL, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to install UART driver: %s", esp_err_to_name(err));
        return err;
    }
    
    err = uart_param_config(uart_num, &uart_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure UART: %s", esp_err_to_name(err));
        uart_driver_delete(uart_num);
        return err;
    }
    
    err = uart_set_pin(uart_num, tx_pin, rx_pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set UART pins: %s", esp_err_to_name(err));
        uart_driver_delete(uart_num);
        return err;
    }
    
    // Initialize state
    gps_state.uart_num = uart_num;
    gps_state.initialized = true;
    gps_state.init_time_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
    memset(&gps_state.current_data, 0, sizeof(gps_data_t));
    memset(&gps_state.stats, 0, sizeof(gps_stats_t));
    
    // Create GPS reading task
    BaseType_t task_created = xTaskCreate(
        gps_task,
        "gps_task",
        GPS_TASK_STACK_SIZE,
        NULL,
        GPS_TASK_PRIORITY,
        &gps_state.task_handle
    );
    
    if (task_created != pdPASS) {
        ESP_LOGE(TAG, "Failed to create GPS task");
        uart_driver_delete(uart_num);
        gps_state.initialized = false;
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "✓ GPS initialized successfully");
    ESP_LOGI(TAG, "💡 Waiting for GPS fix (cold start: ~26s, hot start: ~1s)");
    
    return ESP_OK;
}

/**
 * Read latest GPS data
 */
esp_err_t gps_read(gps_data_t *data)
{
    if (!gps_state.initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    if (!data) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // Copy current data
    memcpy(data, &gps_state.current_data, sizeof(gps_data_t));
    
    // Return error if no valid fix yet
    if (!gps_state.current_data.valid) {
        return ESP_ERR_NOT_FOUND;
    }
    
    return ESP_OK;
}

/**
 * Check if GPS has valid fix
 */
bool gps_has_fix(void)
{
    return gps_state.initialized && gps_state.current_data.valid;
}

/**
 * Get GPS statistics
 */
void gps_get_stats(gps_stats_t *stats)
{
    if (stats) {
        memcpy(stats, &gps_state.stats, sizeof(gps_stats_t));
    }
}

/**
 * Get time since last GPS update
 */
uint32_t gps_get_time_since_update(void)
{
    if (!gps_state.initialized) {
        return UINT32_MAX;
    }
    
    uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
    return now - gps_state.current_data.last_update_ms;
}

/**
 * Deinitialize GPS module
 */
void gps_deinit(void)
{
    if (!gps_state.initialized) {
        return;
    }
    
    // Delete task
    if (gps_state.task_handle) {
        vTaskDelete(gps_state.task_handle);
        gps_state.task_handle = NULL;
    }
    
    // Delete UART driver
    uart_driver_delete(gps_state.uart_num);
    
    gps_state.initialized = false;
    ESP_LOGI(TAG, "GPS deinitialized");
}

/**
 * Pause GPS processing (suspend task)
 * Use during USB transfer mode to free CPU resources
 */
void gps_pause(void)
{
    if (!gps_state.initialized || !gps_state.task_handle) {
        return;
    }
    
    vTaskSuspend(gps_state.task_handle);
    ESP_LOGI(TAG, "⏸️  GPS task suspended");
}

/**
 * Resume GPS processing (resume task)
 * Use after exiting USB transfer mode
 */
void gps_resume(void)
{
    if (!gps_state.initialized || !gps_state.task_handle) {
        return;
    }
    
    vTaskResume(gps_state.task_handle);
    ESP_LOGI(TAG, "▶️  GPS task resumed");
}
