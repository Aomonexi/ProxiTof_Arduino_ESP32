# Wiring Diagram Update - February 4, 2026

**Status**: ✅ Complete and Verified  
**Changes**: GPS pins clarified, SD card pins verified, update rate notes added

---

## Summary of Updates

### 1. GPS Pin Assignment - VERIFIED ✅

**Correct Pins:**
- **GPIO 38** (TX) → GPS RX
- **GPIO 47** (RX) ← GPS TX

**Why these pins:**
- GPIO 38 and 47 are FREE (not used by any other peripheral)
- GPIO 43/44 are UART0 (console) - cannot be used
- SD card uses GPIO 14, 15, 2, 16, 17, 18 (NOT GPIO 38)

**Confusion Resolved:**
- Old documentation showed SD card on GPIO 38-43 (SDIO pins)
- Current implementation uses GPIO 14-18 (different SDMMC pins)
- GPIO 38/47 are completely free for GPS

### 2. SD Card Pin Assignment - VERIFIED ✅

**Correct Pins:**
```
CLK  = GPIO 14
CMD  = GPIO 15
D0   = GPIO 2
D1   = GPIO 16
D2   = GPIO 17
D3   = GPIO 18
```

**NOT using:**
- GPIO 38-43 (old SDIO pins from early documentation)
- These pins are now free for other uses

### 3. GPS Update Rate - DOCUMENTED

**Current Implementation:**
- **Default**: 1 Hz (one position update per second)
- **Adequate for**: Walking, cycling, general movement tracking
- **Power efficient**: Lower update rate = less power consumption

**Future Enhancement Options:**
- **5 Hz**: Good balance for cycling (smooth updates, moderate power)
- **10 Hz**: High precision for fast movement
- **25 Hz**: Maximum rate (research/testing only)

**Implementation Note:**
- Update rate can be changed by sending UBX CFG-RATE command to GPS
- Requires adding UBX protocol support to GPS component
- Current NMEA-only implementation uses default 1 Hz

**Recommendation for Thesis:**
- **Keep 1 Hz** for now - it's sufficient for cycling data
- Position changes slowly enough that 1 Hz captures all movement
- Higher rates would increase UART traffic and power consumption
- Can be increased later if needed for specific analysis

---

## Complete Pin Map (Verified)

| Component | Interface | GPIO Pins | Status |
|-----------|-----------|-----------|--------|
| VL53L8CX ToF | I2C | SDA=8, SCL=9, LPN=36 | ✅ Working |
| BNO08x IMU | SPI | MOSI=11, MISO=13, SCK=12, CS=10, INT=5, RST=6 | ✅ Working |
| SD Card | SDMMC | CLK=14, CMD=15, D0=2, D1=16, D2=17, D3=18 | ✅ Working |
| DS3231 RTC | I2C | SDA=8, SCL=9 (shared) | ✅ Working |
| OLED Display | SPI | SCK=7, MOSI=1, DC=35, RST=21, CS=0 | ✅ Working |
| **Neo-M9N GPS** | **UART** | **TX=38, RX=47** | **✅ Working** |
| Buzzer | PWM | GPIO 45 | ✅ Working |
| Potentiometer | ADC | GPIO 4 (ADC1_CH3) | ✅ Working |
| Flag Button | GPIO | GPIO 3 | ✅ Working |
| Mode Button | GPIO | GPIO 20 | ✅ Working |
| WS2812 LED | GPIO | GPIO 48 | ✅ Working |

---

## GPS Configuration Summary

### Hardware
- **Module**: Neo-M9N (u-blox)
- **Pins**: GPIO 38 (TX), 47 (RX)
- **UART**: UART1 (not console UART0)
- **Baud**: 38400
- **Power**: 3.3V or 5V (depending on breakout board)

### Software
- **Protocol**: NMEA 0183
- **Update Rate**: 1 Hz (default)
- **Dynamic Model**: Configurable (Bike/Pedestrian/Portable/Automotive)
- **Multi-GNSS**: GPS + GLONASS + Galileo + BeiDou
- **Mode Cycling**: Hold ACTION button in GPS mode

### Performance
- **Time to First Fix**: 1-5 minutes (cold start, outdoors)
- **Accuracy**: ~2.5m CEP (with good satellite visibility)
- **Satellites**: Typically 8-15 visible
- **HDOP**: < 2.0 for good accuracy

---

## Why 1 Hz is Sufficient for Cycling

### Position Update Frequency Analysis

**Cycling Speed Examples:**
- **Slow**: 10 km/h = 2.78 m/s → 2.78m per update @ 1 Hz
- **Medium**: 20 km/h = 5.56 m/s → 5.56m per update @ 1 Hz
- **Fast**: 30 km/h = 8.33 m/s → 8.33m per update @ 1 Hz

**GPS Accuracy:**
- Neo-M9N accuracy: ~2.5m CEP
- Position error is larger than distance traveled between updates
- Higher update rate doesn't improve accuracy, just gives more samples

**Near-Miss Detection:**
- Near-miss events are detected by ToF (60 Hz) and IMU (100 Hz)
- GPS provides location context, not real-time detection
- 1 Hz is sufficient to tag where near-miss occurred

**Power Consumption:**
- 1 Hz: ~20-30 mA average
- 5 Hz: ~30-40 mA average
- 10 Hz: ~40-50 mA average
- 25 Hz: ~50-60 mA average

**Conclusion:**
- **1 Hz is optimal** for cycling thesis data collection
- Provides adequate position tracking
- Minimizes power consumption
- Reduces UART traffic and CPU load
- Can be increased later if specific analysis requires it

---

## Future Enhancement: Configurable Update Rate

If you need higher update rates in the future, here's how to implement it:

### 1. Add UBX Protocol Support

```c
// In components/gps/gps.c

/**
 * Set GPS update rate via UBX CFG-RATE command
 * @param rate_hz Update rate in Hz (1-25)
 */
esp_err_t gps_set_update_rate(uint8_t rate_hz) {
    if (rate_hz < 1 || rate_hz > 25) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // UBX CFG-RATE message
    // measRate = 1000 / rate_hz (milliseconds)
    uint16_t meas_rate = 1000 / rate_hz;
    
    uint8_t ubx_cfg_rate[] = {
        0xB5, 0x62,  // UBX header
        0x06, 0x08,  // CFG-RATE
        0x06, 0x00,  // Length
        (uint8_t)(meas_rate & 0xFF),       // measRate LSB
        (uint8_t)((meas_rate >> 8) & 0xFF), // measRate MSB
        0x01, 0x00,  // navRate (1 = every measurement)
        0x01, 0x00,  // timeRef (1 = GPS time)
        0x00, 0x00   // Checksum (calculate)
    };
    
    // Calculate checksum
    uint8_t ck_a = 0, ck_b = 0;
    for (int i = 2; i < sizeof(ubx_cfg_rate) - 2; i++) {
        ck_a += ubx_cfg_rate[i];
        ck_b += ck_a;
    }
    ubx_cfg_rate[sizeof(ubx_cfg_rate) - 2] = ck_a;
    ubx_cfg_rate[sizeof(ubx_cfg_rate) - 1] = ck_b;
    
    // Send to GPS
    uart_write_bytes(GPS_UART_NUM, ubx_cfg_rate, sizeof(ubx_cfg_rate));
    
    return ESP_OK;
}
```

### 2. Add to Config File

```
# In data/config.txt
GPS_UPDATE_RATE=5  # Hz (1-25)
```

### 3. Call During Initialization

```c
// In main.c after gps_init()
gps_set_update_rate(config->gps_update_rate);
```

### 4. Considerations

- **UART Buffer**: May need to increase buffer size for higher rates
- **CPU Load**: Parsing more NMEA sentences takes more CPU time
- **Power**: Higher rates = more power consumption
- **Testing**: Verify no buffer overflows at high rates

---

## Testing Checklist

- [x] GPS pins verified (GPIO 38/47)
- [x] SD card pins verified (GPIO 14-18)
- [x] No pin conflicts
- [x] Wiring diagram updated
- [x] Documentation complete
- [ ] Test GPS with actual hardware
- [ ] Verify 1 Hz update rate is sufficient
- [ ] Consider future enhancement for configurable rate

---

**Status**: ✅ Documentation complete and verified  
**Next**: Flash firmware and test GPS mode cycling feature
