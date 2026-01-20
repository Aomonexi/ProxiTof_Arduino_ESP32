/**
 * gps.h
 * 
 * Neo-M9N GPS Module Driver
 * UART interface for NMEA sentence parsing
 */

#ifndef GPS_H
#define GPS_H

#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * GPS fix quality indicators
 */
typedef enum {
    GPS_FIX_NONE = 0,      // No fix
    GPS_FIX_GPS = 1,       // GPS fix
    GPS_FIX_DGPS = 2,      // Differential GPS fix
    GPS_FIX_PPS = 3,       // PPS fix
    GPS_FIX_RTK = 4,       // Real-Time Kinematic
    GPS_FIX_FLOAT_RTK = 5, // Float RTK
    GPS_FIX_ESTIMATED = 6, // Estimated/Dead reckoning
    GPS_FIX_MANUAL = 7,    // Manual input mode
    GPS_FIX_SIMULATION = 8 // Simulation mode
} gps_fix_quality_t;

/**
 * GPS data structure
 */
typedef struct {
    // Position
    double latitude;           // Degrees (positive = North, negative = South)
    double longitude;          // Degrees (positive = East, negative = West)
    float altitude_m;          // Meters above sea level
    
    // Velocity
    float speed_kmh;           // Speed over ground (km/h)
    float speed_ms;            // Speed over ground (m/s)
    float heading_deg;         // Course over ground (degrees, 0-360)
    
    // Time (UTC from satellites)
    uint8_t hour;              // 0-23
    uint8_t minute;            // 0-59
    uint8_t second;            // 0-59
    uint16_t millisecond;      // 0-999
    
    // Date (UTC from satellites)
    uint8_t day;               // 1-31
    uint8_t month;             // 1-12
    uint16_t year;             // Full year (e.g., 2026)
    
    // Fix quality
    gps_fix_quality_t fix_quality;
    uint8_t satellites_used;   // Number of satellites in use
    float hdop;                // Horizontal dilution of precision
    
    // Status
    bool valid;                // True if GPS has valid fix
    uint32_t last_update_ms;   // Timestamp of last valid update
    
} gps_data_t;

/**
 * GPS statistics
 */
typedef struct {
    uint32_t sentences_parsed;    // Total NMEA sentences parsed
    uint32_t parse_errors;        // Number of parsing errors
    uint32_t checksum_errors;     // Number of checksum failures
    uint32_t valid_fixes;         // Number of valid GPS fixes
    uint32_t time_to_first_fix_s; // Seconds to first fix (0 if not yet)
    bool first_fix_acquired;      // True after first fix
} gps_stats_t;

/**
 * Initialize GPS module
 * 
 * @param uart_num UART port number (e.g., UART_NUM_1)
 * @param tx_pin GPIO pin for TX (ESP32 → GPS RX)
 * @param rx_pin GPIO pin for RX (ESP32 ← GPS TX)
 * @param baud_rate Baud rate (default: 9600)
 * @return ESP_OK on success
 */
esp_err_t gps_init(int uart_num, int tx_pin, int rx_pin, uint32_t baud_rate);

/**
 * Read latest GPS data
 * Non-blocking - returns immediately with latest parsed data
 * 
 * @param data Pointer to GPS data structure to fill
 * @return ESP_OK if data is valid, ESP_ERR_NOT_FOUND if no fix yet
 */
esp_err_t gps_read(gps_data_t *data);

/**
 * Check if GPS has valid fix
 * 
 * @return true if GPS has valid position fix
 */
bool gps_has_fix(void);

/**
 * Get GPS statistics
 * 
 * @param stats Pointer to statistics structure to fill
 */
void gps_get_stats(gps_stats_t *stats);

/**
 * Get time since last GPS update
 * 
 * @return Milliseconds since last valid GPS data
 */
uint32_t gps_get_time_since_update(void);

/**
 * Deinitialize GPS module
 */
void gps_deinit(void);

#ifdef __cplusplus
}
#endif

#endif // GPS_H
