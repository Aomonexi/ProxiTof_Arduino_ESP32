# Wiring Clarifications - Common Questions Answered

**Last Updated**: January 20, 2026

---

## ❓ Question 1: Why does GPS TX connect to ESP32 RX (and vice versa)?

### ✅ Answer: This is CORRECT - UART is Always Crossover!

**The Rule**: 
- **Transmitter (TX) always connects to Receiver (RX)**
- **Receiver (RX) always connects to Transmitter (TX)**

**Visual Explanation**:
```
Device A                          Device B
--------                          --------
TX (sends data)  ──────────────>  RX (receives data)
RX (receives)    <──────────────  TX (sends data)
```

**For GPS Connection**:
```
ESP32-S3                          Neo-M9N GPS
--------                          -----------
GPIO 44 (TX)     ──data──────>    RX (receives ESP32's commands)
GPIO 43 (RX)     <──data──────    TX (sends GPS NMEA data)
```

**Why?**
- ESP32 **transmits** commands → GPS **receives** commands
- GPS **transmits** NMEA data → ESP32 **receives** NMEA data
- If you connect TX to TX, both devices try to transmit at the same time = conflict!
- If you connect RX to RX, both devices try to receive but nobody transmits = no data!

**Analogy**:
Think of it like two people talking:
- Person A's **mouth** (TX) talks to Person B's **ear** (RX)
- Person B's **mouth** (TX) talks to Person A's **ear** (RX)
- You wouldn't connect mouth-to-mouth or ear-to-ear!

### ❌ Common Mistake:
```
WRONG! Don't do this:
ESP32 TX → GPS TX  ❌ (both trying to transmit)
ESP32 RX → GPS RX  ❌ (both trying to receive)
```

### ✅ Correct Wiring:
```
CORRECT:
ESP32 TX (GPIO 44) → GPS RX  ✅
ESP32 RX (GPIO 43) → GPS TX  ✅
```

---

## ❓ Question 2: Why can't I use GPIO 4 for the potentiometer?

### ✅ Answer: GPIO 4 is Already Used by ToF Sensor!

**The Conflict**:
```
GPIO 4 = ToF LPN (Low Power/Reset) pin
       = ALREADY OCCUPIED!
```

**Current GPIO 4 Usage**:
```
ESP32-S3 GPIO 4  →  VL53L8CX ToF Sensor LPN pin
```

The ToF sensor uses GPIO 4 to control its low-power mode and reset functionality. This is a **critical pin** for the ToF sensor to work properly.

**What Happens if You Use GPIO 4 for Potentiometer?**
1. ❌ ToF sensor loses control signal
2. ❌ ToF sensor may not initialize properly
3. ❌ Potentiometer readings will interfere with ToF control
4. ❌ System instability and crashes

### ✅ Solution: Use GPIO 46 Instead!

**Correct Potentiometer Wiring**:
```
Potentiometer              ESP32-S3
-------------              --------
Wiper (middle pin)    →    GPIO 46 (ADC2_CH5)  ✅ CORRECT!
One end               →    3.3V
Other end             →    GND
```

**Why GPIO 46?**
- ✅ GPIO 46 is **FREE** (not used by any other component)
- ✅ GPIO 46 has **ADC capability** (ADC2_CH5)
- ✅ GPIO 46 supports **12-bit resolution** (0-4095 range)
- ✅ GPIO 46 can read **0-3.3V** range (perfect for potentiometer)

**GPIO 46 ADC Configuration**:
```c
// ADC2 Channel 5 (GPIO 46)
adc_oneshot_unit_init_cfg_t adc_config = {
    .unit_id = ADC_UNIT_2,  // GPIO 46 is on ADC2
};

adc_oneshot_chan_cfg_t chan_config = {
    .atten = ADC_ATTEN_DB_12,      // 0-3.3V range
    .bitwidth = ADC_BITWIDTH_12,   // 12-bit (0-4095)
};

// GPIO 46 = ADC2_CH5
adc_oneshot_config_channel(adc_handle, ADC_CHANNEL_5, &chan_config);
```

---

## 📊 Complete GPIO Usage Table

| GPIO | Component | Function | Status |
|------|-----------|----------|--------|
| 1 | OLED Display | SPI MOSI | ✅ Used |
| 2 | SD Card | SDMMC D0 | ✅ Used |
| 3 | Button | Input | ✅ Used |
| **4** | **ToF Sensor** | **LPN (Low Power)** | **✅ OCCUPIED** |
| 5 | IMU | INT | ✅ Used |
| 6 | IMU | RST | ✅ Used |
| 7 | OLED Display | SPI SCK | ✅ Used |
| 8 | ToF + RTC | I2C SDA | ✅ Used |
| 9 | ToF + RTC | I2C SCL | ✅ Used |
| 10 | IMU | SPI CS | ✅ Used |
| 11 | IMU | SPI MOSI | ✅ Used |
| 12 | IMU | SPI SCK | ✅ Used |
| 13 | IMU | SPI MISO | ✅ Used |
| 14 | SD Card | SDMMC CLK | ✅ Used |
| 15 | SD Card | SDMMC CMD | ✅ Used |
| 16 | SD Card | SDMMC D1 | ✅ Used |
| 17 | SD Card | SDMMC D2 | ✅ Used |
| 18 | SD Card | SDMMC D3 | ✅ Used |
| 19-20 | - | - | ⭕ Available |
| 21 | OLED Display | RST | ✅ Used |
| 35 | OLED Display | DC | ✅ Used |
| 36-42 | - | - | ⭕ Available |
| 43 | GPS | UART RX | ✅ Used |
| 44 | GPS | UART TX | ✅ Used |
| 45 | Buzzer | PWM | ✅ Used |
| **46** | **Potentiometer** | **ADC** | **✅ CORRECT PIN** |
| 47 | - | - | ⭕ Available |
| 48 | LED | WS2812 Data | ✅ Used |

**Summary**:
- ✅ Used: 26 pins
- ⭕ Available: 12+ pins
- ❌ GPIO 4: OCCUPIED by ToF sensor
- ✅ GPIO 46: FREE for potentiometer

---

## 🔧 Quick Reference

### GPS Wiring (UART Crossover)
```
ESP32-S3          Neo-M9N GPS
--------          -----------
GPIO 44 (TX)  →   RX          ✅ Crossover is correct!
GPIO 43 (RX)  ←   TX          ✅ Crossover is correct!
5V            →   VCC         ✅ GPS needs 5V
GND           →   GND         ✅
```

### Buzzer Wiring
```
ESP32-S3          Buzzer
--------          ------
GPIO 45 (PWM) →   IN          ✅
5V            →   VCC         ✅
GND           →   GND         ✅
```

### Potentiometer Wiring (Optional)
```
ESP32-S3          Potentiometer
--------          -------------
GPIO 46 (ADC) →   Wiper         ✅ NOT GPIO 4!
3.3V          →   One end       ✅
GND           →   Other end     ✅
```

---

## ⚠️ Common Mistakes to Avoid

### ❌ Mistake 1: Connecting UART TX to TX
```
WRONG:
ESP32 TX (GPIO 44) → GPS TX  ❌
ESP32 RX (GPIO 43) → GPS RX  ❌

CORRECT:
ESP32 TX (GPIO 44) → GPS RX  ✅
ESP32 RX (GPIO 43) → GPS TX  ✅
```

### ❌ Mistake 2: Using GPIO 4 for Potentiometer
```
WRONG:
Potentiometer → GPIO 4  ❌ (conflicts with ToF LPN)

CORRECT:
Potentiometer → GPIO 46 ✅ (free ADC pin)
```

### ❌ Mistake 3: Powering GPS with 3.3V
```
WRONG:
GPS VCC → 3.3V  ❌ (GPS needs 5V)

CORRECT:
GPS VCC → 5V    ✅
```

### ❌ Mistake 4: Forgetting Ground Connection
```
WRONG:
Only connecting VCC, TX, RX  ❌

CORRECT:
Connect VCC, GND, TX, RX     ✅
```

---

## 🧪 Testing Your Wiring

### GPS Test
1. Connect GPS with crossover wiring (TX→RX, RX→TX)
2. Power on system
3. Check serial output for: `GPS: Initializing...`
4. Wait 26 seconds for first fix
5. Should see: `GPS: ✓ First fix acquired`
6. Should see coordinates: `GPS: 37.123456, -122.987654`

**If GPS doesn't work**:
- ✅ Check TX/RX crossover (not TX→TX)
- ✅ Check 5V power (not 3.3V)
- ✅ Check GND connection
- ✅ Move to open sky (GPS doesn't work indoors)

### Buzzer Test
1. Connect buzzer to GPIO 45
2. Power on system
3. Should hear startup melody (3 ascending tones)
4. Press button → should hear single beep
5. Trigger near-miss → should hear 3 short beeps

**If buzzer doesn't work**:
- ✅ Check GPIO 45 connection
- ✅ Check 5V power
- ✅ Check buzzer type (active vs passive)
- ✅ Try adjusting volume in code

### Potentiometer Test (Optional)
1. Connect potentiometer to GPIO 46 (NOT GPIO 4!)
2. Turn potentiometer knob
3. Volume should change (0-100%)
4. Check serial output for ADC readings

**If potentiometer doesn't work**:
- ✅ Check GPIO 46 connection (NOT GPIO 4!)
- ✅ Check 3.3V and GND connections
- ✅ Check wiper is connected to GPIO 46
- ✅ Enable potentiometer in code: `use_potentiometer = true`

---

## 📚 Additional Resources

- **Complete Wiring Diagram**: `docs/complete-wiring-diagram.md`
- **Integration Guide**: `docs/gps-buzzer-integration-guide.md`
- **Quick Start**: `docs/GPS-BUZZER-QUICK-START.md`
- **GPIO Pin Reference**: ESP32-S3 datasheet

---

## 💡 Key Takeaways

1. ✅ **UART is always crossover**: TX→RX, RX→TX (this is normal and correct!)
2. ✅ **GPIO 4 is occupied**: Used by ToF sensor, use GPIO 46 for potentiometer
3. ✅ **GPS needs 5V**: Not 3.3V (but logic levels are 3.3V compatible)
4. ✅ **Always connect GND**: Power and ground must both be connected
5. ✅ **Test each component**: Verify wiring before integrating into main code

---

## ❓ Question 3: How does the potentiometer control buzzer volume when they're not connected?

### ✅ Answer: The ESP32 is the "Brain" That Connects Them!

The potentiometer and buzzer are **NOT directly connected**. The ESP32 reads the potentiometer and controls the buzzer through **software**:

```
┌─────────────────────────────────────────────────────┐
│                    ESP32-S3                         │
│                  (The Brain)                        │
│                                                     │
│  GPIO 46 (ADC) ←── Reads voltage from potentiometer│
│       ↓                                             │
│   [Software]                                        │
│   Converts voltage to volume (0-100%)               │
│       ↓                                             │
│  GPIO 45 (PWM) ──→ Controls buzzer duty cycle      │
│                                                     │
└─────────────────────────────────────────────────────┘
         ↑                              ↓
         │                              │
    Potentiometer                    Buzzer
    (Input Device)                (Output Device)
```

### How It Works (Step-by-Step):

**Step 1: Potentiometer Creates Voltage**
```
User turns knob → Voltage changes (0V to 3.3V)
```

**Step 2: ESP32 Reads Voltage (ADC)**
```c
int adc_value = adc_read(GPIO_46);  // 0 to 4095
```

**Step 3: Software Converts to Volume**
```c
int volume = (adc_value * 100) / 4095;  // 0% to 100%
```

**Step 4: Software Converts to PWM**
```c
int pwm_duty = (volume * 1023) / 100;  // 0 to 1023
```

**Step 5: ESP32 Controls Buzzer (PWM)**
```c
set_pwm(GPIO_45, pwm_duty);  // Buzzer volume changes
```

### Real-World Example:

```
1. You turn knob to 75% position
   ↓
2. Potentiometer outputs 2.475V (75% of 3.3V)
   ↓
3. ESP32 ADC reads 3071 (75% of 4095)
   ↓
4. Software calculates 75% volume
   ↓
5. Software sets PWM to 767 duty (75% of 1023)
   ↓
6. Buzzer plays at 75% volume
```

**All of this happens inside the ESP32 - no direct wire needed!**

### Analogy: TV Remote Control

```
Remote Control          TV
(Potentiometer)         (Buzzer)
      ↓                   ↑
      └─→ [You] ←─────────┘
        (ESP32 Brain)
```

- Remote sends signal to **you** (ESP32)
- **You** interpret and control TV (buzzer)
- Remote and TV are NOT directly connected
- **You** are the intermediary

**For detailed explanation, see**: `docs/HOW-POTENTIOMETER-CONTROLS-BUZZER.md`

---

**Questions Answered!**  
**Your wiring is now clear and correct.**
