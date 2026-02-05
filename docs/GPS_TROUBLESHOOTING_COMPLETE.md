# GPS Troubleshooting Guide - Neo-M9N Complete Diagnosis

**Date**: February 3, 2026  
**Module**: u-blox Neo-M9N  
**Issue**: Not acquiring satellite fix  
**Reference**: Neo-M9N Integration Manual (UBX-19014286)

---

## Quick Diagnosis Checklist

Run through this checklist first:

### 1. Power and Wiring
- [ ] GPS module powered (3.3V or 5V depending on breakout board)
- [ ] Power LED on GPS module is lit
- [ ] TX/RX wires connected (crossover: ESP32 TX → GPS RX, ESP32 RX ← GPS TX)
- [ ] Ground connected
- [ ] No loose connections

### 2. UART Communication
- [ ] Serial monitor shows "GPS UART active" messages
- [ ] Bytes received counter increasing
- [ ] Hex dump shows ASCII characters (not all zeros or 0xFF)
- [ ] NMEA sentences visible (starting with $GP or $GN)

### 3. Environment
- [ ] Device is outdoors or near window
- [ ] Clear view of sky (no metal roof, thick walls)
- [ ] Not in basement or enclosed space
- [ ] Antenna facing up (if external antenna)

### 4. Timing
- [ ] Waited at least 30 seconds (warm start)
- [ ] Waited at least 60 seconds (cold start)
- [ ] Waited up to 5 minutes (first-ever fix)

---

## Problem 1: No UART Data Received

**Symptoms:**
```
W (5000) GPS: ⚠️  No GPS data received for 5 seconds
W (30000) GPS: ⚠️  Still no GPS data after 30 seconds
```

### Diagnosis Steps:

#### Step 1: Check Power

**Test:**
```c
// In main.c, add before gps_init():
gpio_set_direction(GPS_TX_PIN, GPIO_MODE_INPUT);
gpio_set_pull_mode(GPS_TX_PIN, GPIO_PULLUP_ONLY);
vTaskDelay(pdMS_TO_TICKS(100));
int level = gpio_get_level(GPS_TX_PIN);
ESP_LOGI(TAG, "GPS TX pin level: %d (should be 1 if GPS powered)", level);
```

**Expected**: Level should be 1 (HIGH) if GPS is powered and idle  
**If 0**: GPS not powered or TX pin shorted to ground

**Fix:**
- Check GPS VCC connection (3.3V or 5V)
- Check GPS GND connection
- Verify power LED on GPS module is lit
- Measure voltage at GPS VCC pin with multimeter (should be 3.3V or 5V)

#### Step 2: Check Wiring

**Verify Connections:**
```
ESP32-S3          Neo-M9N GPS
--------          -----------
GPIO 38 (TX)  →   RX (GPS receives)
GPIO 47 (RX)  ←   TX (GPS transmits)
3.3V or 5V    →   VCC/VIN
GND           →   GND
```

**Common Mistakes:**
- ❌ TX connected to TX (should be crossover!)
- ❌ RX connected to RX (should be crossover!)
- ❌ Wrong GPIO pins (check your board's pinout)
- ❌ Loose dupont wires (try soldering)

**Test with Loopback:**
```c
// Temporarily connect ESP32 TX (GPIO 38) to ESP32 RX (GPIO 47)
// Send test data:
const char *test = "TEST\r\n";
uart_write_bytes(UART_NUM_1, test, strlen(test));
vTaskDelay(pdMS_TO_TICKS(100));

// Check if received:
uint8_t buf[32];
int len = uart_read_bytes(UART_NUM_1, buf, sizeof(buf), pdMS_TO_TICKS(100));
if (len > 0) {
    ESP_LOGI(TAG, "✓ UART loopback works, wiring issue is with GPS");
} else {
    ESP_LOGE(TAG, "✗ UART loopback failed, ESP32 UART problem");
}
```

#### Step 3: Check Baud Rate

**Neo-M9N Default**: 38400 baud (8N1)

**Try Different Baud Rates:**
```c
// In main/main.c, try these in order:
#define GPS_BAUD_RATE 38400  // Default (try first)
#define GPS_BAUD_RATE 9600   // Common alternative
#define GPS_BAUD_RATE 115200 // High speed alternative
#define GPS_BAUD_RATE 4800   // Low speed alternative
```

**How to Test:**
1. Change baud rate in main.c
2. Rebuild and flash: `idf.py build flash monitor`
3. Check if "GPS UART active" appears
4. Check if hex dump shows readable ASCII

**Readable ASCII Example:**
```
I (5000) GPS: 📡 GPS UART active: 82 bytes received
0x3fceb7c0   24 47 50 47 47 41 2c 31  32 33 35 31 39 2c ...
             $  G  P  G  G  A  ,  1   2  3  5  1  9  ,
```

**Garbled Data Example (wrong baud rate):**
```
0x3fceb7c0   ff 00 aa 55 cc 33 99 66  ...
```

#### Step 4: Check UART Port Conflict

**Verify UART1 is Free:**
```bash
# In serial monitor, check for conflicts:
grep "UART" build/bootloader/bootloader.map
grep "UART" build/proxitof.map
```

**ESP32-S3 UART Ports:**
- UART0: USB-to-Serial (console) - GPIO 43/44 - **DO NOT USE**
- UART1: Available - GPIO 38/47 - **USING THIS** ✅
- UART2: Available - Any GPIO via matrix

**If Conflict:**
```c
// Try UART2 instead:
#define GPS_UART_NUM UART_NUM_2
```

---

## Problem 2: UART Data Received, But No NMEA Sentences

**Symptoms:**
```
I (5000) GPS: 📡 GPS UART active: 82 bytes received
// Hex dump shows data, but no "$GP" or "$GN" sentences
```

### Diagnosis Steps:

#### Step 1: Check Data Format

**Look at Hex Dump:**
```
// Good NMEA data (ASCII):
0x3fceb7c0   24 47 50 47 47 41 2c 31  32 33 35 31 39 2c
             $  G  P  G  G  A  ,  1   2  3  5  1  9  ,

// Bad data (binary UBX protocol):
0x3fceb7c0   b5 62 01 07 5c 00 00 00  00 00 00 00 00 00
             µ  b  .  .  \  .  .  .   .  .  .  .  .  .
```

**If Binary Data (UBX Protocol):**
GPS is in UBX mode, not NMEA mode. Need to configure GPS to output NMEA.

**Fix - Send UBX Configuration:**
```c
// Add to gps.c after uart_set_pin():

// UBX command to enable NMEA output on UART1
// UBX-CFG-PRT: Configure UART1 for NMEA output
const uint8_t ubx_cfg_prt[] = {
    0xB5, 0x62,  // Header
    0x06, 0x00,  // Class/ID: CFG-PRT
    0x14, 0x00,  // Length: 20 bytes
    0x01,        // Port ID: UART1
    0x00,        // Reserved
    0x00, 0x00,  // TX ready (not used)
    0xD0, 0x08, 0x00, 0x00,  // Mode: 8N1
    0x00, 0x96, 0x00, 0x00,  // Baud rate: 38400
    0x07, 0x00,  // Input protocols: UBX+NMEA+RTCM
    0x03, 0x00,  // Output protocols: UBX+NMEA
    0x00, 0x00,  // Flags
    0x00, 0x00,  // Reserved
    0xB0, 0x7E   // Checksum
};

uart_write_bytes(uart_num, ubx_cfg_prt, sizeof(ubx_cfg_prt));
vTaskDelay(pdMS_TO_TICKS(100));

ESP_LOGI(TAG, "Sent UBX-CFG-PRT to enable NMEA output");
```

#### Step 2: Check Sentence Parsing

**Add Debug Logging:**
```c
// In gps.c, gps_task(), after "Start of sentence":
if (c == '$') {
    sentence_pos = 0;
    sentence[sentence_pos++] = c;
    
    // DEBUG: Log sentence start
    ESP_LOGI(TAG, "📝 NMEA sentence start detected");
}

// After "End of sentence":
else if (c == '\n' || c == '\r') {
    if (sentence_pos > 0) {
        sentence[sentence_pos] = '\0';
        
        // DEBUG: Log complete sentence
        ESP_LOGI(TAG, "📝 Complete sentence: %s", sentence);
        
        parse_nmea_sentence(sentence, sentence_pos);
        sentence_pos = 0;
    }
}
```

**Expected Output:**
```
I (5000) GPS: 📝 NMEA sentence start detected
I (5001) GPS: 📝 Complete sentence: $GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47
I (5002) GPS: 📝 NMEA sentence start detected
I (5003) GPS: 📝 Complete sentence: $GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A
```

---

## Problem 3: NMEA Sentences Received, But No Satellite Fix

**Symptoms:**
```
I (5000) GPS: 📝 Complete sentence: $GPGGA,123519,,,,,,0,00,99.99,,,,,,*48
// Empty lat/lon, fix_quality=0, satellites=0
```

### Diagnosis Steps:

#### Step 1: Check Environment

**Requirements for GPS Fix:**
- ✅ **Outdoors** or near window with clear sky view
- ✅ **No metal roof** or thick concrete above
- ✅ **Not in basement** or parking garage
- ✅ **Antenna facing up** (if external antenna)
- ✅ **Stationary** (moving makes first fix harder)

**Test Locations:**
- ✅ **Good**: Open field, rooftop, balcony, near window
- ❌ **Bad**: Basement, parking garage, inside metal building
- ⚠️ **Maybe**: Inside near window (depends on building)

#### Step 2: Check Antenna

**Internal Antenna (Ceramic Patch):**
- Should be facing up (toward sky)
- Keep away from metal objects
- No obstructions above antenna

**External Antenna (U.FL/SMA):**
- Check antenna is connected to U.FL connector
- Antenna cable not damaged
- Antenna positioned with clear sky view
- Try different antenna if available

**Test:**
```c
// Check if GPS is receiving satellite signals
// Look for GPGSV sentences (satellites in view)
```

**Expected GPGSV:**
```
$GPGSV,3,1,12,01,45,123,42,02,30,234,38,03,15,045,35,04,60,312,40*7A
       ^     ^^ satellites in view (12 total)
```

**If 0 satellites in view:**
- Antenna problem
- Environment problem (no sky view)
- GPS module hardware issue

#### Step 3: Wait Longer

**Time to First Fix (TTFF):**
- **Hot Start** (recent fix, backup battery active): 1-2 seconds
- **Warm Start** (almanac available): 26-29 seconds
- **Cold Start** (no almanac, no position): 26-60 seconds
- **Factory Reset** (first-ever fix): 60-300 seconds (1-5 minutes!)

**Your Situation:**
If this is the first time powering on the GPS, it needs to download almanac data from satellites. This can take **5 minutes or more**.

**What to Do:**
1. Go outdoors with clear sky view
2. Keep device stationary
3. Wait patiently for 5-10 minutes
4. Watch serial monitor for satellite count increasing

**Expected Progress:**
```
I (30000) GPS: 📡 Satellites in view: 0
I (60000) GPS: 📡 Satellites in view: 3
I (90000) GPS: 📡 Satellites in view: 6
I (120000) GPS: 📡 Satellites in view: 8
I (150000) GPS: ✓ First GPS fix acquired in 150 seconds
```

#### Step 4: Check GPS Configuration

**Verify Multi-GNSS is Enabled:**

Neo-M9N supports GPS, GLONASS, Galileo, and BeiDou. More constellations = faster fix.

**Send UBX Command to Enable All GNSS:**
```c
// UBX-CFG-GNSS: Enable all GNSS systems
const uint8_t ubx_cfg_gnss[] = {
    0xB5, 0x62,  // Header
    0x06, 0x3E,  // Class/ID: CFG-GNSS
    0x3C, 0x00,  // Length: 60 bytes
    0x00,        // msgVer
    0x00,        // numTrkChHw (read-only)
    0xFF,        // numTrkChUse: 255 (use all)
    0x05,        // numConfigBlocks: 5
    
    // GPS
    0x00, 0x08, 0x10, 0x00, 0x01, 0x00, 0x01, 0x01,
    // SBAS
    0x01, 0x01, 0x03, 0x00, 0x01, 0x00, 0x01, 0x01,
    // Galileo
    0x02, 0x04, 0x08, 0x00, 0x01, 0x00, 0x01, 0x01,
    // BeiDou
    0x03, 0x08, 0x10, 0x00, 0x01, 0x00, 0x01, 0x01,
    // GLONASS
    0x06, 0x08, 0x0E, 0x00, 0x01, 0x00, 0x01, 0x01,
    
    0x2C, 0x0E   // Checksum
};

uart_write_bytes(uart_num, ubx_cfg_gnss, sizeof(ubx_cfg_gnss));
vTaskDelay(pdMS_TO_TICKS(100));

ESP_LOGI(TAG, "Sent UBX-CFG-GNSS to enable all GNSS systems");
```

**Save Configuration to Flash:**
```c
// UBX-CFG-CFG: Save configuration
const uint8_t ubx_cfg_save[] = {
    0xB5, 0x62,  // Header
    0x06, 0x09,  // Class/ID: CFG-CFG
    0x0D, 0x00,  // Length: 13 bytes
    0x00, 0x00, 0x00, 0x00,  // Clear mask
    0xFF, 0xFF, 0x00, 0x00,  // Save mask (all)
    0x00, 0x00, 0x00, 0x00,  // Load mask
    0x17,        // Device mask: BBR, Flash, EEPROM
    0x31, 0xBF   // Checksum
};

uart_write_bytes(uart_num, ubx_cfg_save, sizeof(ubx_cfg_save));
vTaskDelay(pdMS_TO_TICKS(500));

ESP_LOGI(TAG, "Saved GPS configuration to flash");
```

---

## Problem 4: Intermittent Fix or Fix Lost

**Symptoms:**
- GPS gets fix, then loses it
- Fix quality fluctuates
- Satellite count drops

### Possible Causes:

1. **Movement**: GPS loses fix when moving fast or changing direction rapidly
2. **Obstructions**: Buildings, trees, tunnels block satellites
3. **Antenna**: Loose connection or poor antenna placement
4. **Power**: Voltage drops or brownouts
5. **Interference**: WiFi, Bluetooth, or other RF interference

### Solutions:

**1. Improve Antenna Placement:**
- Keep antenna facing up
- Away from metal objects
- Clear line of sight to sky

**2. Add Backup Battery:**
- Neo-M9N has backup battery input (VBCKP pin)
- Maintains hot start capability
- Faster reacquisition after power loss

**3. Increase Update Rate:**
```c
// Default: 1 Hz (1 update per second)
// Can increase to 5 Hz or 10 Hz for faster updates

// UBX-CFG-RATE: Set measurement rate to 200ms (5 Hz)
const uint8_t ubx_cfg_rate_5hz[] = {
    0xB5, 0x62,  // Header
    0x06, 0x08,  // Class/ID: CFG-RATE
    0x06, 0x00,  // Length: 6 bytes
    0xC8, 0x00,  // Measurement rate: 200ms
    0x01, 0x00,  // Navigation rate: 1 cycle
    0x01, 0x00,  // Time reference: GPS time
    0xDE, 0x6A   // Checksum
};
```

**4. Use Dynamic Platform Model:**
```c
// UBX-CFG-NAV5: Set dynamic platform model
// 0 = Portable (default)
// 3 = Pedestrian
// 4 = Automotive
// 6 = Bike

const uint8_t ubx_cfg_nav5_bike[] = {
    0xB5, 0x62,  // Header
    0x06, 0x24,  // Class/ID: CFG-NAV5
    0x24, 0x00,  // Length: 36 bytes
    0xFF, 0xFF,  // Mask: apply all
    0x06,        // Dynamic model: Bike
    0x03,        // Fix mode: Auto 2D/3D
    // ... (rest of parameters)
};
```

---

## Recommended GPS Initialization Sequence

Here's the complete initialization sequence with all fixes:

```c
esp_err_t gps_init_complete(int uart_num, int tx_pin, int rx_pin)
{
    ESP_LOGI(TAG, "=== GPS Complete Initialization ===");
    
    // Step 1: Initialize UART
    uart_config_t uart_config = {
        .baud_rate = 38400,  // Neo-M9N default
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    
    ESP_ERROR_CHECK(uart_driver_install(uart_num, 2048, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(uart_num, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(uart_num, tx_pin, rx_pin, 
                                  UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    
    ESP_LOGI(TAG, "✓ UART configured (38400 baud, 8N1)");
    
    // Step 2: Wait for GPS to boot (100ms)
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // Step 3: Configure GPS for NMEA output
    const uint8_t ubx_cfg_prt[] = {
        0xB5, 0x62, 0x06, 0x00, 0x14, 0x00,
        0x01, 0x00, 0x00, 0x00,
        0xD0, 0x08, 0x00, 0x00,
        0x00, 0x96, 0x00, 0x00,
        0x07, 0x00, 0x03, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0xB0, 0x7E
    };
    uart_write_bytes(uart_num, ubx_cfg_prt, sizeof(ubx_cfg_prt));
    vTaskDelay(pdMS_TO_TICKS(100));
    ESP_LOGI(TAG, "✓ Configured for NMEA output");
    
    // Step 4: Enable all GNSS systems
    const uint8_t ubx_cfg_gnss[] = {
        0xB5, 0x62, 0x06, 0x3E, 0x3C, 0x00,
        0x00, 0x00, 0xFF, 0x05,
        // GPS
        0x00, 0x08, 0x10, 0x00, 0x01, 0x00, 0x01, 0x01,
        // SBAS
        0x01, 0x01, 0x03, 0x00, 0x01, 0x00, 0x01, 0x01,
        // Galileo
        0x02, 0x04, 0x08, 0x00, 0x01, 0x00, 0x01, 0x01,
        // BeiDou
        0x03, 0x08, 0x10, 0x00, 0x01, 0x00, 0x01, 0x01,
        // GLONASS
        0x06, 0x08, 0x0E, 0x00, 0x01, 0x00, 0x01, 0x01,
        0x2C, 0x0E
    };
    uart_write_bytes(uart_num, ubx_cfg_gnss, sizeof(ubx_cfg_gnss));
    vTaskDelay(pdMS_TO_TICKS(100));
    ESP_LOGI(TAG, "✓ Enabled all GNSS systems (GPS+GLONASS+Galileo+BeiDou)");
    
    // Step 5: Set dynamic platform model to Bike
    const uint8_t ubx_cfg_nav5[] = {
        0xB5, 0x62, 0x06, 0x24, 0x24, 0x00,
        0xFF, 0xFF, 0x06, 0x03,
        0x00, 0x00, 0x00, 0x00,
        0x10, 0x27, 0x00, 0x00,
        0x05, 0x00, 0xFA, 0x00,
        0xFA, 0x00, 0x64, 0x00,
        0x2C, 0x01, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x16, 0xDC
    };
    uart_write_bytes(uart_num, ubx_cfg_nav5, sizeof(ubx_cfg_nav5));
    vTaskDelay(pdMS_TO_TICKS(100));
    ESP_LOGI(TAG, "✓ Set dynamic model to Bike");
    
    // Step 6: Save configuration
    const uint8_t ubx_cfg_save[] = {
        0xB5, 0x62, 0x06, 0x09, 0x0D, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0xFF, 0xFF, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x17, 0x31, 0xBF
    };
    uart_write_bytes(uart_num, ubx_cfg_save, sizeof(ubx_cfg_save));
    vTaskDelay(pdMS_TO_TICKS(500));
    ESP_LOGI(TAG, "✓ Saved configuration to flash");
    
    ESP_LOGI(TAG, "=== GPS Initialization Complete ===");
    ESP_LOGI(TAG, "💡 Waiting for satellite fix (may take 1-5 minutes)");
    ESP_LOGI(TAG, "💡 Ensure device is outdoors with clear sky view");
    
    return ESP_OK;
}
```

---

## Quick Fix Summary

**If no UART data:**
1. Check power (3.3V or 5V)
2. Check wiring (TX→RX crossover)
3. Try different baud rate (38400, 9600, 115200)

**If UART data but no NMEA:**
1. Send UBX-CFG-PRT to enable NMEA output
2. Check for binary UBX data (0xB5 0x62 header)

**If NMEA but no fix:**
1. Go outdoors with clear sky view
2. Wait 5-10 minutes for first fix
3. Enable all GNSS systems (GPS+GLONASS+Galileo+BeiDou)
4. Check antenna placement

**If intermittent fix:**
1. Improve antenna placement
2. Add backup battery
3. Use bike dynamic model
4. Increase update rate

---

## Testing Commands

Add these to your code for testing:

```c
// Test 1: Check if GPS is responding
void gps_test_ping(void)
{
    // UBX-MON-VER: Request version
    const uint8_t ubx_mon_ver[] = {0xB5, 0x62, 0x0A, 0x04, 0x00, 0x00, 0x0E, 0x34};
    uart_write_bytes(GPS_UART_NUM, ubx_mon_ver, sizeof(ubx_mon_ver));
    
    ESP_LOGI(TAG, "Sent version request, check for UBX response");
}

// Test 2: Reset GPS to factory defaults
void gps_factory_reset(void)
{
    // UBX-CFG-CFG: Reset to factory defaults
    const uint8_t ubx_cfg_reset[] = {
        0xB5, 0x62, 0x06, 0x09, 0x0D, 0x00,
        0xFF, 0xFF, 0x00, 0x00,  // Clear all
        0x00, 0x00, 0x00, 0x00,  // Don't save
        0xFF, 0xFF, 0x00, 0x00,  // Load all
        0x17, 0x2F, 0xAE
    };
    uart_write_bytes(GPS_UART_NUM, ubx_cfg_reset, sizeof(ubx_cfg_reset));
    
    ESP_LOGW(TAG, "GPS reset to factory defaults, will reboot");
    vTaskDelay(pdMS_TO_TICKS(1000));
}
```

---

**Next Steps:**
1. Check serial monitor for GPS debug messages
2. Verify UART data is being received
3. Go outdoors and wait 5-10 minutes
4. If still no fix, try UBX configuration commands above

**Document Version**: 1.0  
**Date**: February 3, 2026  
**Status**: Complete troubleshooting guide

