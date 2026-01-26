# GPIO Pin Reference - Quick Lookup

**Date:** January 26, 2026  
**Board:** ESP32-S3-DevKitC-1  
**Status:** ✅ Verified from complete wiring diagram

---

## Button GPIO Pins

| Button | GPIO | Function | Notes |
|--------|------|----------|-------|
| **Button 1** | **GPIO 20** | Mode/Navigation | Cycles through display modes |
| **Button 2** | **GPIO 3** | Action (context-aware) | Flag event / WiFi sync |

---

## Complete GPIO Assignment

### Sensors

| Component | Interface | GPIO Pins | Notes |
|-----------|-----------|-----------|-------|
| **VL53L8CX ToF** | I2C | SDA=8, SCL=9, LPN=4 | 4×4 distance grid @ 60Hz |
| **BNO08x IMU** | SPI | MOSI=11, MISO=13, SCK=12, CS=10, INT=5, RST=6 | 9-DOF @ 100Hz |
| **DS3231 RTC** | I2C | SDA=8, SCL=9 (shared with ToF) | Battery-backed time |
| **Neo-M9N GPS** | UART | TX=44, RX=43 | Location @ 1-5Hz |

### Display & Storage

| Component | Interface | GPIO Pins | Notes |
|-----------|-----------|-----------|-------|
| **OLED Display** | SPI | SCK=7, MOSI=1, DC=35, RST=21, CS=0 | 128×64 @ 60Hz |
| **SD Card** | SDMMC | CLK=14, CMD=15, D0=2, D1=16, D2=17, D3=18 | 4-bit mode |

### User Interface

| Component | Type | GPIO | Notes |
|-----------|------|------|-------|
| **Mode Button** | Input | GPIO 20 | Pull-up, active LOW |
| **Flag Button** | Input | GPIO 3 | Pull-up, active LOW |
| **WS2812 LED** | Output | GPIO 48 | Built-in RGB LED |
| **Buzzer** | PWM | GPIO 45 | Audio alerts |
| **Potentiometer** | ADC | GPIO 46 | Volume control (optional) |

---

## Button Wiring

### Button 1 - Mode Switch (GPIO 20)

```
Push Button         →    ESP32-S3
───────────              ─────────
Terminal 1          →    GPIO 20
Terminal 2          →    GND
```

**Configuration:**
- Internal pull-up enabled
- Active LOW (pressed = 0V)
- Debounce: 200ms
- Function: Cycle display modes

---

### Button 2 - Action Button (GPIO 3)

```
Push Button         →    ESP32-S3
───────────              ─────────
Terminal 1          →    GPIO 3
Terminal 2          →    GND
```

**Configuration:**
- Internal pull-up enabled
- Active LOW (pressed = 0V)
- Debounce: Handled by sensor_manager
- Function: Context-aware (flag event / WiFi sync)

---

## Button Behavior Matrix

| Display Mode | Button 1 (GPIO 20) | Button 2 (GPIO 3) |
|--------------|-------------------|-------------------|
| Dashboard | Next mode | **Flag event** |
| ToF Focus | Next mode | No action |
| IMU Focus | Next mode | No action |
| Status | Next mode | No action |
| Near Miss | Next mode | **Flag event** |
| WiFi Settings | Next mode | **WiFi sync** |

---

## Common Mistakes to Avoid

### ❌ Wrong GPIO Assignments

**WRONG:**
```c
#define MODE_BUTTON_PIN GPIO_NUM_19  // ❌ Wrong!
#define FLAG_BUTTON_PIN GPIO_NUM_20  // ❌ Wrong!
```

**CORRECT:**
```c
#define MODE_BUTTON_PIN GPIO_NUM_20  // ✅ Correct
#define FLAG_BUTTON_PIN GPIO_NUM_3   // ✅ Correct
```

### ❌ Swapped Button Functions

**WRONG:**
- GPIO 3 for mode switching ❌
- GPIO 20 for event flagging ❌

**CORRECT:**
- GPIO 20 for mode switching ✅
- GPIO 3 for event flagging ✅

---

## Code Reference

### Button Initialization

**Mode Button (GPIO 20):**
```c
// In main.c
#define MODE_BUTTON_PIN GPIO_NUM_20

gpio_config_t io_conf = {
    .pin_bit_mask = (1ULL << MODE_BUTTON_PIN),
    .mode = GPIO_MODE_INPUT,
    .pull_up_en = GPIO_PULLUP_ENABLE,
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .intr_type = GPIO_INTR_DISABLE,
};
gpio_config(&io_conf);
```

**Flag Button (GPIO 3):**
```c
// In sensor_manager component
#define BUTTON_PIN GPIO_NUM_3

gpio_config_t io_conf = {
    .pin_bit_mask = (1ULL << BUTTON_PIN),
    .mode = GPIO_MODE_INPUT,
    .pull_up_en = GPIO_PULLUP_ENABLE,
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .intr_type = GPIO_INTR_DISABLE,
};
gpio_config(&io_conf);
```

### Button Reading

**Mode Button:**
```c
bool button_pressed = (gpio_get_level(MODE_BUTTON_PIN) == 0);
```

**Flag Button:**
```c
bool button_pressed = sensor_manager_is_button_pressed();
bool button_held = sensor_manager_is_button_held();
uint32_t duration = sensor_manager_get_button_duration();
```

---

## Troubleshooting

### Problem: Button doesn't respond

**Check:**
1. GPIO pin number is correct (20 or 3)
2. Button is wired to correct GPIO
3. Other terminal of button is connected to GND
4. Pull-up is enabled in code
5. Debounce delay (wait 200ms between presses)

### Problem: Wrong button does wrong action

**Solution:**
- GPIO 20 should cycle modes
- GPIO 3 should flag events (in Dashboard/Near-Miss) or sync WiFi (in WiFi Settings)
- Check wiring matches code

### Problem: Button triggers multiple times

**Solution:**
- Debouncing is working correctly
- This is normal for mechanical buttons
- Software debounce handles this (200ms)

---

## Physical Layout Suggestion

```
┌─────────────────────────────────────┐
│         ProxiTof Device             │
│                                     │
│  ┌─────────┐         ┌─────────┐   │
│  │ Button 1│         │ Button 2│   │
│  │ GPIO 20 │         │ GPIO 3  │   │
│  │  MODE   │         │ ACTION  │   │
│  └─────────┘         └─────────┘   │
│                                     │
│  ┌─────────────────────────────┐   │
│  │     OLED Display            │   │
│  │     128×64 pixels           │   │
│  └─────────────────────────────┘   │
│                                     │
│         [WS2812 LED]                │
└─────────────────────────────────────┘
```

**Recommended Button Placement:**
- Button 1 (Mode) on the LEFT
- Button 2 (Action) on the RIGHT
- Both buttons above the display
- Easy thumb access when holding device

---

## Quick Reference Card

```
╔═══════════════════════════════════════════════════╗
║           ProxiTof GPIO Quick Reference           ║
╠═══════════════════════════════════════════════════╣
║                                                   ║
║  BUTTONS:                                         ║
║  ├─ GPIO 20: Mode Switch (cycles display)        ║
║  └─ GPIO 3:  Action Button (flag/sync)           ║
║                                                   ║
║  SENSORS:                                         ║
║  ├─ GPIO 8,9:    I2C (ToF + RTC)                 ║
║  ├─ GPIO 11-13:  SPI (IMU)                       ║
║  └─ GPIO 14-18:  SDMMC (SD Card)                 ║
║                                                   ║
║  DISPLAY:                                         ║
║  └─ GPIO 0,1,7,21,35: SPI OLED                   ║
║                                                   ║
║  OTHER:                                           ║
║  ├─ GPIO 43,44: UART (GPS)                       ║
║  ├─ GPIO 45:    PWM (Buzzer)                     ║
║  ├─ GPIO 46:    ADC (Potentiometer)              ║
║  └─ GPIO 48:    LED (WS2812)                     ║
║                                                   ║
╚═══════════════════════════════════════════════════╝
```

---

**Status:** ✅ Verified and correct  
**Source:** `docs/complete-wiring-diagram.md`  
**Last Updated:** January 26, 2026

