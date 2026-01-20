# 0.96" OLED Display (SSD1306) - SPI Integration Guide

## Overview

Adding a 0.96" OLED display (128x64 pixels, SSD1306 controller) for real-time sensor visualization at 60 FPS.

## Display Specifications

**0.96" OLED Module (SSD1306):**
- Resolution: 128x64 pixels (monochrome)
- Controller: SSD1306
- Interface: **SPI (4-wire)**
- Refresh Rate: Up to 100 FPS capable
- Power: 3.3V, ~20mA
- Viewing Angle: >160°

## Why SPI Instead of I2C?

**SPI Advantages:**
- ✅ **Much Faster**: 10-20 MHz vs 400 kHz (I2C)
- ✅ **60 FPS Capable**: Can update display smoothly
- ✅ **No Bus Conflicts**: Dedicated interface
- ✅ **Better for Graphics**: High bandwidth for animations

**I2C Would Be:**
- ❌ Too slow for 60 FPS (max ~10-15 FPS)
- ❌ Shares bus with ToF and RTC
- ❌ Limited bandwidth for graphics

## GPIO Pin Assignment

### Available GPIOs (from wiring diagram)
Looking at unused pins: GPIO 1, 7, 19-21, 35-37, 44-47

### Recommended SPI Pins for OLED

```
OLED SSD1306 (SPI)    →    ESP32-S3
──────────────────          ─────────
VCC (3.3V)            →    3.3V
GND                   →    GND
SCK (Clock)           ←    GPIO 7   (SPI Clock)
MOSI (Data)           ←    GPIO 1   (SPI Data Out)
DC (Data/Command)     ←    GPIO 19  (Register Select)
CS (Chip Select)      ←    GPIO 20  (Chip Select)
RES (Reset)           ←    GPIO 21  (Reset)
```

**Why These Pins:**
- ✅ **GPIO 1, 7**: Free, no conflicts
- ✅ **GPIO 19-21**: Consecutive, easy to wire
- ✅ **No conflicts** with existing sensors
- ✅ **Separate SPI bus** from IMU (different pins)

## Complete Updated Wiring Diagram

```
                           ESP32-S3-DevKitC-1
                    ╔══════════════════════════════════╗
                    ║                                  ║
    I2C Bus         ║  SPI Bus (IMU)      SDMMC Bus    ║  GPIO
    -------         ║  --------------     ----------   ║  -----
                    ║                                  ║
VL53L8CX ToF ──────╫── GPIO 8  (SDA)                  ║
             ──────╫── GPIO 9  (SCL)                  ║
             ──────╫── GPIO 4  (LPN)                  ║
                    ║                                  ║
DS3231 RTC  ───────╫── GPIO 8  (SDA) ← Shared I2C     ║
            ───────╫── GPIO 9  (SCL) ← Shared I2C     ║
                    ║                                  ║
                    ║              BNO08x IMU          ║
                    ║              -----------          ║
                    ║  GPIO 11 (MOSI) ──────────────── ║
                    ║  GPIO 13 (MISO) ──────────────── ║
                    ║  GPIO 12 (SCK)  ──────────────── ║
                    ║  GPIO 10 (CS)   ──────────────── ║
                    ║  GPIO 5  (INT)  ──────────────── ║
                    ║  GPIO 6  (RST)  ──────────────── ║
                    ║                                  ║
                    ║                      SD Card     ║
                    ║                      --------    ║
                    ║              GPIO 14 (CLK) ───── ║
                    ║              GPIO 15 (CMD) ───── ║
                    ║              GPIO 2  (D0)  ───── ║
                    ║              GPIO 16 (D1)  ───── ║
                    ║              GPIO 17 (D2)  ───── ║
                    ║              GPIO 18 (D3)  ───── ║
                    ║                                  ║
                    ║              OLED Display (NEW!) ║
                    ║              ----------------    ║
                    ║  GPIO 7  (SCK)  ──────────────── ║
                    ║  GPIO 1  (MOSI) ──────────────── ║
                    ║  GPIO 19 (DC)   ──────────────── ║
                    ║  GPIO 20 (CS)   ──────────────── ║
                    ║  GPIO 21 (RES)  ──────────────── ║
                    ║                                  ║
                    ║  GPIO 3  ──────────────── Button ║
                    ║  GPIO 48 ──────────────── LED    ║ (Built-in)
                    ║                                  ║
                    ╚══════════════════════════════════╝
```

## Detailed OLED Wiring

### Physical Connections

```
ESP32-S3 Pin    →    OLED Module Pin    Function
────────────         ───────────────    ────────
3.3V            →    VCC                Power (3.3V)
GND             →    GND                Ground
GPIO 7          →    SCK (CLK/D0)       SPI Clock
GPIO 1          →    MOSI (SDA/D1)      SPI Data
GPIO 19         →    DC (D/C)           Data/Command Select
GPIO 20         →    CS                 Chip Select
GPIO 21         →    RES (RST)          Reset
```

### Pin Functions Explained

- **SCK (Clock)**: SPI clock signal, up to 20 MHz
- **MOSI (Data)**: Serial data from ESP32 to display
- **DC (Data/Command)**: LOW = command, HIGH = data
- **CS (Chip Select)**: LOW = active, HIGH = inactive
- **RES (Reset)**: LOW = reset, HIGH = normal operation

## Display Layout Design

### 128x64 Pixel Layout

```
┌────────────────────────────────────────┐
│ ProxiTof v1.0          15:30:45  [USB] │ ← Header (8px)
├────────────────────────────────────────┤
│                                        │
│  ToF Grid (4x4):      IMU:             │
│  ┌──┬──┬──┬──┐       Roll:  +12.3°    │
│  │45│32│28│35│       Pitch: -5.2°     │
│  ├──┼──┼──┼──┤       Yaw:   +180.0°   │
│  │52│18│15│42│                         │
│  ├──┼──┼──┼──┤       Accel:            │
│  │48│12│10│38│       X: +0.98 g       │
│  ├──┼──┼──┼──┤       Y: -0.05 g       │
│  │55│25│22│40│       Z: +0.15 g       │
│  └──┴──┴──┴──┘                         │
│                                        │
│  Min: 10cm  Avg: 28cm  Max: 55cm      │
│  Logging: 3600 entries  [45% buffer]  │
└────────────────────────────────────────┘
```

### Display Modes

**Mode 1: Full Dashboard (Default)**
- ToF 4x4 grid visualization
- IMU orientation (roll, pitch, yaw)
- Acceleration values
- Statistics (min, avg, max distance)
- Logging status

**Mode 2: ToF Focus**
- Large 4x4 grid with bigger numbers
- Color-coded zones (simulated with patterns)
- Distance bar graph
- Min distance highlighted

**Mode 3: IMU Focus**
- Large orientation display
- Gyroscope values
- Acceleration graph
- Motion indicators

**Mode 4: System Status**
- SD card status
- Buffer fill percentage
- Time source (DS3231/NTP/USB)
- USB MSC status
- Battery level (if added)

## Software Implementation

### ESP-IDF Component

We'll use the `esp_lcd` component with SSD1306 driver:

```c
// Display configuration
#define OLED_WIDTH  128
#define OLED_HEIGHT 64
#define OLED_SCK    GPIO_NUM_7
#define OLED_MOSI   GPIO_NUM_1
#define OLED_DC     GPIO_NUM_19
#define OLED_CS     GPIO_NUM_20
#define OLED_RST    GPIO_NUM_21

// SPI bus configuration
spi_bus_config_t buscfg = {
    .mosi_io_num = OLED_MOSI,
    .miso_io_num = -1,  // Not used
    .sclk_io_num = OLED_SCK,
    .quadwp_io_num = -1,
    .quadhd_io_num = -1,
    .max_transfer_sz = OLED_WIDTH * OLED_HEIGHT / 8 + 8,
};

// SSD1306 device configuration
esp_lcd_panel_io_spi_config_t io_config = {
    .dc_gpio_num = OLED_DC,
    .cs_gpio_num = OLED_CS,
    .pclk_hz = 10 * 1000 * 1000,  // 10 MHz
    .lcd_cmd_bits = 8,
    .lcd_param_bits = 8,
    .spi_mode = 0,
};
```

### Graphics Library Options

**Option 1: U8g2 (Recommended)**
- ✅ Excellent font support
- ✅ Easy to use API
- ✅ Optimized for SSD1306
- ✅ Many examples available

**Option 2: LovyanGFX**
- ✅ Very fast
- ✅ Advanced graphics
- ✅ Good for animations
- ❌ Larger code size

**Option 3: ESP-IDF LCD**
- ✅ Native ESP-IDF
- ✅ Good performance
- ❌ Limited graphics primitives

### Update Rate Strategy

**60 FPS Display Update:**
```c
// Main loop (200 Hz)
while (1) {
    // Read sensors at full speed
    read_tof_sensor();   // 60 Hz
    read_imu_sensor();   // 100 Hz
    
    // Update display at 60 Hz (every 16.67ms)
    static uint32_t last_display_update = 0;
    uint32_t now = esp_timer_get_time() / 1000;
    if (now - last_display_update >= 16) {
        update_oled_display();
        last_display_update = now;
    }
    
    vTaskDelay(pdMS_TO_TICKS(5));  // 200 Hz loop
}
```

**Optimizations for 60 FPS:**
1. **Partial Updates**: Only redraw changed areas
2. **Double Buffering**: Prepare frame in RAM, then send
3. **DMA Transfer**: Use SPI DMA for non-blocking updates
4. **Simplified Graphics**: Avoid complex shapes

## Power Consumption

```
Component Power Budget:
─────────────────────────
ESP32-S3:        80-240 mA
VL53L8CX ToF:    20-100 mA
BNO08x IMU:      2.5-10 mA
DS3231 RTC:      0.1-1 mA
SD Card:         50-200 mA
WS2812 LED:      1-60 mA
OLED Display:    15-25 mA  ← NEW
─────────────────────────
Total:           ~170-636 mA
```

**Recommendation:** USB power (5V/1A) is sufficient.

## Configuration in config.txt

Add OLED settings:

```
# OLED Display Configuration
OLED_ENABLED=true
OLED_REFRESH_HZ=60
OLED_MODE=0  # 0=Dashboard, 1=ToF, 2=IMU, 3=Status
OLED_BRIGHTNESS=128  # 0-255
OLED_FLIP=false  # Rotate 180 degrees
```

## Implementation Steps

### 1. Hardware Setup
- [ ] Connect OLED module to ESP32-S3 (GPIO 1, 7, 19-21)
- [ ] Verify 3.3V power connection
- [ ] Check all SPI connections

### 2. Software Setup
- [ ] Add U8g2 library to project
- [ ] Create OLED display component
- [ ] Implement display update function
- [ ] Add configuration options

### 3. Testing
- [ ] Test basic display initialization
- [ ] Test text rendering
- [ ] Test graphics (lines, rectangles)
- [ ] Test 60 FPS update rate
- [ ] Verify no interference with other sensors

### 4. Integration
- [ ] Add ToF grid visualization
- [ ] Add IMU data display
- [ ] Add system status
- [ ] Implement display modes
- [ ] Add button to switch modes

## Troubleshooting

### Display Not Initializing

**Check:**
- 3.3V power (NOT 5V!)
- All SPI connections (SCK, MOSI, DC, CS, RES)
- Reset pin (should be HIGH after init)
- SPI bus configuration

**Test:**
```c
// Simple test pattern
esp_lcd_panel_draw_bitmap(panel, 0, 0, 128, 64, test_pattern);
```

### Display Flickering

**Solutions:**
- Increase SPI clock speed (up to 20 MHz)
- Use double buffering
- Enable DMA transfers
- Reduce update frequency

### Slow Update Rate

**Optimizations:**
- Use partial updates (only changed areas)
- Simplify graphics (fewer pixels to update)
- Increase SPI clock speed
- Use DMA for transfers

### Interference with Other Sensors

**Check:**
- OLED uses separate SPI bus (different pins from IMU)
- No GPIO conflicts
- Adequate power supply
- Proper grounding

## Expected Performance

**With Optimizations:**
- ✅ 60 FPS display update
- ✅ Smooth animations
- ✅ No sensor interference
- ✅ Low CPU usage (~10-15%)
- ✅ Responsive UI

**Display Latency:**
- Sensor read: ~1ms
- Display update: ~10-15ms
- Total latency: ~16ms (60 FPS)

## Benefits of OLED Display

1. **Real-Time Feedback**: See sensor data instantly
2. **No Computer Needed**: Standalone operation
3. **Field Testing**: Easy to use outdoors
4. **Debugging**: Visual confirmation of sensor operation
5. **User Interface**: Mode selection, status display
6. **Professional Look**: Complete embedded system

## Next Steps

1. **Order OLED Module**: 0.96" SSD1306 SPI (4-wire)
2. **Wire According to Diagram**: GPIO 1, 7, 19-21
3. **Install U8g2 Library**: Add to project dependencies
4. **Implement Display Code**: Create display component
5. **Test and Optimize**: Achieve 60 FPS update rate

Your system will have a professional real-time display! 🎉