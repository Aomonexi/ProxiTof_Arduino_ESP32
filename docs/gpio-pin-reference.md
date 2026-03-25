# GPIO Pin Reference

**Board:** ESP32-S3-DevKitC-1
**Last Updated:** March 24, 2026
**Source:** Verified from actual source code

---

## Complete GPIO Assignment

### Sensors

| Component | Interface | GPIO Pins | Source File |
|-----------|-----------|-----------|-------------|
| VL53L8CX ToF | I2C | SDA=8, SCL=9, LPN=36 | `sensor_manager.h` |
| BNO08x IMU | SPI | MOSI=11, MISO=13, SCK=12, CS=10, INT=5, RST=6 | `sensor_manager.h` |
| DS3231 RTC | I2C | SDA=8, SCL=9 (shared with ToF) | `config_manager.c` |
| Neo-M9N GPS | UART1 | TX=38, RX=47 | `main.c` |

### Display & Storage

| Component | Interface | GPIO Pins | Source File |
|-----------|-----------|-----------|-------------|
| SSD1306 OLED | 4-wire SPI | SCK=7, MOSI=1, DC=35, RST=21 (no CS) | `oled_display.c` |
| SD Card | SDMMC 4-bit | CLK=14, CMD=15, D0=2, D1=16, D2=17, D3=18 | `storage_manager.c` |

### User Interface

| Component | Type | GPIO | Source File |
|-----------|------|------|-------------|
| Mode Button | Input (pull-up) | 20 | `main.c` |
| Action/Flag Button | Input (pull-up) | 3 | `sensor_manager.h` |
| WS2812 RGB LED | RMT output | 48 | `main.c` |
| Buzzer | PWM (LEDC) | 45 | `main.c` |
| Potentiometer | ADC1_CH3 | 4 | `main.c` |

---

## Reserved Pins (DO NOT USE)

| GPIO | Reason |
|------|--------|
| 43, 44 | UART0 console (hardwired to USB-UART chip) |
| 0 | Strapping pin (boot mode) |
| 46 | Strapping pin (was potentiometer, moved to GPIO 4) |

## Available Pins (Free for expansion)

GPIO 37, 39, 40, 41, 42

---

## Button Behavior by Mode

| Display Mode | MODE (GPIO 20) | ACTION (GPIO 3) |
|--------------|----------------|-----------------|
| Dashboard (0) | Next mode | Hold = manual flag |
| Status (1) | Next mode | Refresh status |
| ToF Focus (2) | Next mode | — |
| IMU Focus (3) | Next mode | Rescan IMU |
| GPS (4) | Next mode | Reconnect GPS / Hold = cycle model |
| Near Miss (5) | Next mode | Cycle detection mode |
| WiFi Settings (6) | Next mode | Sync time |
| USB Transfer (7) | Next mode | Toggle USB MSC on/off |
| Shutdown (8) | Next mode | Confirm shutdown |
