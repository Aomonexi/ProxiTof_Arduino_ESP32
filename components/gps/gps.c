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
    uint8_t dynamic_model;  // Current dynamic platform model
    uint8_t update_rate_hz;  // Current update rate in Hz
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
    
    // DIAGNOSTIC: Log antenna status messages (GNTXT/GPTXT)
    if (strncmp(sentence, "$GNTXT", 6) == 0 || strncmp(sentence, "$GPTXT", 6) == 0) {
        // Check for antenna status
        if (strstr(sentence, "ANTSTATUS") != NULL) {
            if (strstr(sentence, "OK") != NULL) {
                ESP_LOGI(TAG, "✅ Antenna Status: OK (connected and working)");
            } else if (strstr(sentence, "OPEN") != NULL) {
                ESP_LOGW(TAG, "⚠️  Antenna Status: OPEN (disconnected or not detected)");
            } else if (strstr(sentence, "SHORT") != NULL) {
                ESP_LOGE(TAG, "❌ Antenna Status: SHORT (short circuit detected!)");
            } else {
                ESP_LOGI(TAG, "📡 Antenna Status: %s", sentence);
            }
        }
    }
    
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
esp_err_t gps_init(int uart_num, int tx_pin, int rx_pin, uint32_t baud_rate, uint8_t dynamic_model)
{
    if (gps_state.initialized) {
        ESP_LOGW(TAG, "GPS already initialized");
        return ESP_OK;
    }
    
    ESP_LOGI(TAG, "Initializing GPS on UART%d (TX=%d, RX=%d, baud=%lu, model=%s)", 
             uart_num, tx_pin, rx_pin, baud_rate, gps_get_model_name(dynamic_model));
    
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
    
    // Wait for GPS module to boot (100ms)
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // Optional: Configure GPS for optimal performance
    // Note: GPS works fine with factory defaults, this just optimizes it
    ESP_LOGI(TAG, "Configuring GPS for optimal performance...");
    
    // 0. Configure antenna (ensure IPEX/internal antenna is active)
    // NEO-M9N CFG-ANT: Select internal antenna (IPEX) over external (SMA)
    // This is critical if module has both IPEX and SMA connectors
    uint8_t ubx_cfg_ant[] = {
        0xB5, 0x62,  // UBX header
        0x06, 0x13,  // CFG-ANT
        0x04, 0x00,  // Length: 4 bytes
        0x1B, 0x00,  // flags: svcs=1, scd=1, ocd=1, pdwnOnSCD=1, recovery=1
        0xF3, 0x9F,  // pins: default pin configuration
        0x00, 0x00   // Checksum (will be calculated)
    };
    
    // Calculate checksum for CFG-ANT
    uint8_t ck_a = 0, ck_b = 0;
    for (int i = 2; i < sizeof(ubx_cfg_ant) - 2; i++) {
        ck_a += ubx_cfg_ant[i];
        ck_b += ck_a;
    }
    ubx_cfg_ant[sizeof(ubx_cfg_ant) - 2] = ck_a;
    ubx_cfg_ant[sizeof(ubx_cfg_ant) - 1] = ck_b;
    
    uart_write_bytes(uart_num, ubx_cfg_ant, sizeof(ubx_cfg_ant));
    vTaskDelay(pdMS_TO_TICKS(100));
    ESP_LOGI(TAG, "✓ Configured antenna (internal/IPEX active, SCD/OCD enabled)");
    
    // 1. Enable all GNSS systems (GPS + GLONASS + Galileo + BeiDou + QZSS + SBAS)
    // This gives faster fix acquisition (more satellites visible)
    // NEO-M9N supports: GPS L1C/A, GLONASS L1OF, Galileo E1B/C, BeiDou B1I, QZSS L1C/A, SBAS L1C/A
    const uint8_t ubx_cfg_gnss[] = {
        0xB5, 0x62,  // UBX header
        0x06, 0x3E,  // CFG-GNSS
        0x44, 0x00,  // Length: 68 bytes (6 GNSS systems × 8 bytes + 4 byte header)
        0x00, 0x00, 0xFF, 0x06,  // Version 0, numTrkChHw=255, numTrkChUse=6 config blocks
        // GPS (8 channels, enabled, L1C/A signal)
        0x00, 0x08, 0x10, 0x00, 0x01, 0x00, 0x01, 0x01,
        // SBAS (1 channel, enabled, L1C/A signal)
        0x01, 0x01, 0x03, 0x00, 0x01, 0x00, 0x01, 0x01,
        // Galileo (4 channels, enabled, E1B/C signal)
        0x02, 0x04, 0x08, 0x00, 0x01, 0x00, 0x01, 0x01,
        // BeiDou (8 channels, enabled, B1I signal)
        0x03, 0x08, 0x10, 0x00, 0x01, 0x00, 0x01, 0x01,
        // QZSS (2 channels, enabled, L1C/A signal)
        0x05, 0x02, 0x05, 0x00, 0x01, 0x00, 0x01, 0x01,
        // GLONASS (8 channels, enabled, L1OF signal)
        0x06, 0x08, 0x0E, 0x00, 0x01, 0x00, 0x01, 0x01,
        0x00, 0x00   // Checksum (will be calculated)
    };
    
    // Calculate checksum for UBX message (reuse variables)
    ck_a = 0;
    ck_b = 0;
    for (int i = 2; i < sizeof(ubx_cfg_gnss) - 2; i++) {
        ck_a += ubx_cfg_gnss[i];
        ck_b += ck_a;
    }
    
    // Create mutable copy with correct checksum
    uint8_t ubx_cfg_gnss_msg[sizeof(ubx_cfg_gnss)];
    memcpy(ubx_cfg_gnss_msg, ubx_cfg_gnss, sizeof(ubx_cfg_gnss));
    ubx_cfg_gnss_msg[sizeof(ubx_cfg_gnss) - 2] = ck_a;
    ubx_cfg_gnss_msg[sizeof(ubx_cfg_gnss) - 1] = ck_b;
    
    uart_write_bytes(uart_num, ubx_cfg_gnss_msg, sizeof(ubx_cfg_gnss_msg));
    vTaskDelay(pdMS_TO_TICKS(100));
    ESP_LOGI(TAG, "✓ Enabled all 6 GNSS systems:");
    ESP_LOGI(TAG, "  - GPS (L1C/A): 8 channels");
    ESP_LOGI(TAG, "  - GLONASS (L1OF): 8 channels");
    ESP_LOGI(TAG, "  - Galileo (E1B/C): 4 channels");
    ESP_LOGI(TAG, "  - BeiDou (B1I): 8 channels");
    ESP_LOGI(TAG, "  - QZSS (L1C/A): 2 channels");
    ESP_LOGI(TAG, "  - SBAS (L1C/A): 1 channel");
    ESP_LOGI(TAG, "  Total: 31 tracking channels for maximum satellite visibility");
    
    // 2. Set dynamic platform model from config
    // Optimizes Kalman filter for specific movement type
    ESP_LOGI(TAG, "Setting dynamic model to %s (%d)...", gps_get_model_name(dynamic_model), dynamic_model);
    
    // Build UBX CFG-NAV5 message with dynamic model from parameter
    const uint8_t ubx_cfg_nav5[] = {
        0xB5, 0x62,  // UBX header
        0x06, 0x24,  // CFG-NAV5
        0x24, 0x00,  // Length: 36 bytes
        0xFF, 0xFF,  // Mask: apply all settings
        dynamic_model,  // Dynamic model from config
        0x03,        // Fix mode: 3 = Auto 2D/3D
        0x00, 0x00, 0x00, 0x00,  // Fixed altitude
        0x10, 0x27, 0x00, 0x00,  // Fixed altitude variance
        0x05, 0x00,  // Min elevation: 5 degrees
        0xFA, 0x00,  // Reserved
        0xFA, 0x00,  // Position DOP mask
        0x64, 0x00,  // Time DOP mask
        0x2C, 0x01,  // Position accuracy: 300m
        0x00, 0x00,  // Time accuracy
        0x00, 0x00, 0x00, 0x00,  // Static hold threshold
        0x00, 0x00, 0x00, 0x00,  // DGPS timeout
        0x00, 0x00, 0x00, 0x00,  // Reserved
        0x16, 0xDC   // Checksum (will be recalculated)
    };
    uart_write_bytes(uart_num, ubx_cfg_nav5, sizeof(ubx_cfg_nav5));
    vTaskDelay(pdMS_TO_TICKS(100));
    ESP_LOGI(TAG, "✓ Set dynamic model to %s", gps_get_model_name(dynamic_model));
    
    // 3. Save configuration to flash (persists across power cycles)
    const uint8_t ubx_cfg_save[] = {
        0xB5, 0x62,  // UBX header
        0x06, 0x09,  // CFG-CFG
        0x0D, 0x00,  // Length: 13 bytes
        0x00, 0x00, 0x00, 0x00,  // Clear mask: none
        0xFF, 0xFF, 0x00, 0x00,  // Save mask: all
        0x00, 0x00, 0x00, 0x00,  // Load mask: none
        0x17,        // Device mask: BBR, Flash, EEPROM
        0x31, 0xBF   // Checksum
    };
    uart_write_bytes(uart_num, ubx_cfg_save, sizeof(ubx_cfg_save));
    vTaskDelay(pdMS_TO_TICKS(500));  // Wait for save to complete
    ESP_LOGI(TAG, "✓ Saved configuration to GPS flash memory");
    
    ESP_LOGI(TAG, "GPS configuration complete (runs once, saved to flash)");
    
    // Initialize state
    gps_state.uart_num = uart_num;
    gps_state.initialized = true;
    gps_state.init_time_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
    gps_state.dynamic_model = dynamic_model;  // Store dynamic model
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

/**
 * Calculate UBX checksum (Fletcher algorithm)
 */
static void ubx_checksum(const uint8_t *data, int len, uint8_t *ck_a, uint8_t *ck_b)
{
    *ck_a = 0;
    *ck_b = 0;
    for (int i = 0; i < len; i++) {
        *ck_a += data[i];
        *ck_b += *ck_a;
    }
}

/**
 * Set GPS dynamic platform model
 */
esp_err_t gps_set_dynamic_model(uint8_t model)
{
    if (!gps_state.initialized) {
        ESP_LOGE(TAG, "GPS not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    
    // Validate model (0-10 are valid per u-blox spec)
    if (model > 10) {
        ESP_LOGE(TAG, "Invalid dynamic model: %d (valid: 0-10)", model);
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGI(TAG, "Setting GPS dynamic model to %d (%s)...", model, gps_get_model_name(model));
    
    // Build UBX CFG-NAV5 message
    uint8_t payload[36] = {
        0xFF, 0xFF,  // Mask: apply all settings
        model,       // Dynamic model (0-10 all valid)
        0x03,        // Fix mode: 3 = Auto 2D/3D
        0x00, 0x00, 0x00, 0x00,  // Fixed altitude
        0x10, 0x27, 0x00, 0x00,  // Fixed altitude variance
        0x05, 0x00,  // Min elevation: 5 degrees
        0xFA, 0x00,  // Reserved
        0xFA, 0x00,  // Position DOP mask
        0x64, 0x00,  // Time DOP mask
        0x2C, 0x01,  // Position accuracy: 300m
        0x00, 0x00,  // Time accuracy
        0x00, 0x00, 0x00, 0x00,  // Static hold threshold
        0x00, 0x00, 0x00, 0x00,  // DGPS timeout
        0x00, 0x00, 0x00, 0x00,  // Reserved
    };
    
    // Calculate checksum
    uint8_t ck_a, ck_b;
    uint8_t header[4] = {0x06, 0x24, 0x24, 0x00};  // Class, ID, Length
    ubx_checksum(header, 4, &ck_a, &ck_b);
    uint8_t ck_a2, ck_b2;
    ubx_checksum(payload, sizeof(payload), &ck_a2, &ck_b2);
    ck_a += ck_a2;
    ck_b += ck_b2;
    
    // Build complete message
    uint8_t message[44];
    message[0] = 0xB5;  // UBX header
    message[1] = 0x62;
    message[2] = 0x06;  // CFG-NAV5
    message[3] = 0x24;
    message[4] = 0x24;  // Length LSB
    message[5] = 0x00;  // Length MSB
    memcpy(&message[6], payload, sizeof(payload));
    message[42] = ck_a;
    message[43] = ck_b;
    
    // Send to GPS
    uart_write_bytes(gps_state.uart_num, message, sizeof(message));
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // Save to GPS flash
    const uint8_t ubx_cfg_save[] = {
        0xB5, 0x62,  // UBX header
        0x06, 0x09,  // CFG-CFG
        0x0D, 0x00,  // Length: 13 bytes
        0x00, 0x00, 0x00, 0x00,  // Clear mask: none
        0xFF, 0xFF, 0x00, 0x00,  // Save mask: all
        0x00, 0x00, 0x00, 0x00,  // Load mask: none
        0x17,        // Device mask: BBR, Flash, EEPROM
        0x31, 0xBF   // Checksum
    };
    uart_write_bytes(gps_state.uart_num, ubx_cfg_save, sizeof(ubx_cfg_save));
    vTaskDelay(pdMS_TO_TICKS(500));
    
    // Update state
    gps_state.dynamic_model = model;
    
    ESP_LOGI(TAG, "✓ GPS dynamic model set to %s", gps_get_model_name(model));
    return ESP_OK;
}

/**
 * Get current GPS dynamic platform model
 */
int gps_get_dynamic_model(void)
{
    if (!gps_state.initialized) {
        return -1;
    }
    return gps_state.dynamic_model;
}

/**
 * Get name of GPS dynamic model
 */
const char* gps_get_model_name(uint8_t model)
{
    switch (model) {
        case 0: return "Portable";
        case 1: return "Reserved";
        case 2: return "Stationary";
        case 3: return "Pedestrian";
        case 4: return "Automotive";
        case 5: return "Sea";
        case 6: return "Airborne<1g";
        case 7: return "Airborne<2g";
        case 8: return "Airborne<4g";
        case 9: return "Wrist";
        case 10: return "Bike";
        default: return "Unknown";
    }
}

/**
 * Set GPS update rate (measurement rate)
 */
esp_err_t gps_set_update_rate(uint8_t rate_hz)
{
    if (!gps_state.initialized) {
        ESP_LOGE(TAG, "GPS not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    
    // Validate rate (1-25 Hz)
    if (rate_hz < 1 || rate_hz > 25) {
        ESP_LOGE(TAG, "Invalid update rate: %d Hz (valid: 1-25)", rate_hz);
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGI(TAG, "Setting GPS update rate to %d Hz...", rate_hz);
    
    // Calculate measurement rate in milliseconds
    // measurement_rate = 1000 / rate_hz
    uint16_t meas_rate_ms = 1000 / rate_hz;
    
    // Build UBX CFG-RATE message
    // Format: measRate (2 bytes), navRate (2 bytes), timeRef (2 bytes)
    // measRate: measurement period in ms (e.g., 200ms for 5Hz)
    // navRate: navigation rate cycles (1 = every measurement)
    // timeRef: time reference (0 = UTC, 1 = GPS time)
    uint8_t payload[6] = {
        (uint8_t)(meas_rate_ms & 0xFF),        // measRate LSB
        (uint8_t)((meas_rate_ms >> 8) & 0xFF), // measRate MSB
        0x01, 0x00,  // navRate = 1 (every measurement)
        0x00, 0x00   // timeRef = 0 (UTC)
    };
    
    // Calculate checksum
    uint8_t ck_a, ck_b;
    uint8_t header[4] = {0x06, 0x08, 0x06, 0x00};  // Class, ID, Length
    ubx_checksum(header, 4, &ck_a, &ck_b);
    uint8_t ck_a2, ck_b2;
    ubx_checksum(payload, sizeof(payload), &ck_a2, &ck_b2);
    ck_a += ck_a2;
    ck_b += ck_b2;
    
    // Build complete message
    uint8_t message[14];
    message[0] = 0xB5;  // UBX header
    message[1] = 0x62;
    message[2] = 0x06;  // CFG-RATE
    message[3] = 0x08;
    message[4] = 0x06;  // Length LSB
    message[5] = 0x00;  // Length MSB
    memcpy(&message[6], payload, sizeof(payload));
    message[12] = ck_a;
    message[13] = ck_b;
    
    // Send to GPS
    uart_write_bytes(gps_state.uart_num, message, sizeof(message));
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // Save to GPS flash
    const uint8_t ubx_cfg_save[] = {
        0xB5, 0x62,  // UBX header
        0x06, 0x09,  // CFG-CFG
        0x0D, 0x00,  // Length: 13 bytes
        0x00, 0x00, 0x00, 0x00,  // Clear mask: none
        0xFF, 0xFF, 0x00, 0x00,  // Save mask: all
        0x00, 0x00, 0x00, 0x00,  // Load mask: none
        0x17,        // Device mask: BBR, Flash, EEPROM
        0x31, 0xBF   // Checksum
    };
    uart_write_bytes(gps_state.uart_num, ubx_cfg_save, sizeof(ubx_cfg_save));
    vTaskDelay(pdMS_TO_TICKS(500));
    
    // Update state
    gps_state.update_rate_hz = rate_hz;
    
    ESP_LOGI(TAG, "✓ GPS update rate set to %d Hz (%d ms period)", rate_hz, meas_rate_ms);
    return ESP_OK;
}

/**
 * Get current GPS update rate
 */
uint8_t gps_get_update_rate(void)
{
    if (!gps_state.initialized) {
        return 0;
    }
    return gps_state.update_rate_hz;
}
