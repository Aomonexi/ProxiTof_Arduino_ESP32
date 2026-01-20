# ProxiTof Hardware Checklist - Pre-Assembly Guide

**Last Updated**: January 20, 2026  
**Purpose**: Verify all hardware components before final assembly  
**Goal**: No more soldering needed after this checklist!

---

## 📋 Complete Component List

### ✅ Core Components (Already Working)

| Component | Quantity | Status | Notes |
|-----------|----------|--------|-------|
| ESP32-S3-DevKitC-1 (N16R8) | 1 | ✅ Working | 16MB Flash, 8MB PSRAM |
| VL53L8CX ToF Sensor | 1 | ✅ Working | 4x4 distance grid |
| BNO08x IMU Sensor | 1 | ✅ Working | 9-DOF motion sensor |
| SD Card Module (SDMMC) | 1 | ✅ Working | 4-bit interface |
| DS3231 RTC Module | 1 | ✅ Working | Battery-backed time |
| OLED Display (SSD1306) | 1 | ✅ Working | 128x64, 4-wire SPI |
| Push Button | 1 | ✅ Working | Manual event flag |
| SD Card (Class 10) | 1 | ✅ Working | FAT32 formatted |
| CR2032 Battery | 1 | ✅ Working | For RTC backup |

### 🔧 New Components (Need to Add)

| Component | Quantity | Status | Where to Get |
|-----------|----------|--------|--------------|
| **Neo-M9N GPS Module** | 1 | ⏳ Need | u-blox Neo-M9N breakout |
| **Buzzer (Active or Passive)** | 1 | ⏳ Need | 5V buzzer from Yurobot board |
| **Potentiometer 10kΩ** | 1 | ⏳ Optional | For volume control |

---

## 🔍 Component Verification Checklist

### 1. ESP32-S3-DevKitC-1 ✅

**Check:**
- [ ] Board model: ESP32-S3-DevKitC-1-N16R8
- [ ] Has 2 USB ports (LEFT = USB-OTG, RIGHT = UART)
- [ ] All GPIO pins accessible
- [ ] No physical damage
- [ ] Can be programmed via RIGHT port

**Pin Count Verification:**
- Total GPIOs available: 45
- Currently used: 24 pins
- Available for GPS/Buzzer: 21+ pins ✅

---

### 2. VL53L8CX ToF Sensor ✅

**Check:**
- [ ] Breakout board has 4 pins: VCC, GND, SDA, SCL
- [ ] Has LPN (Low Power) pin or can be controlled via GPIO
- [ ] I2C address: 0x29 (default)
- [ ] 3.3V compatible
- [ ] Already soldered/connected

**Current Wiring:**
- SDA → GPIO 8 ✅
- SCL → GPIO 9 ✅
- LPN → GPIO 4 ✅
- VCC → 3.3V ✅
- GND → GND ✅

---

### 3. BNO08x IMU Sensor ✅

**Check:**
- [ ] Breakout board has SPI pins: MOSI, MISO, SCK, CS, INT, RST
- [ ] Has P0, P1 pins for protocol selection
- [ ] 3.3V compatible
- [ ] Already soldered/connected

**Current Wiring:**
- MOSI → GPIO 11 ✅
- MISO → GPIO 13 ✅
- SCK → GPIO 12 ✅
- CS → GPIO 10 ✅
- INT → GPIO 5 ✅
- RST → GPIO 6 ✅
- P0 → 3.3V ✅
- P1 → 3.3V ✅
- VCC → 3.3V ✅
- GND → GND ✅

---

### 4. SD Card Module (SDMMC) ✅

**Check:**
- [ ] Module supports SDMMC (4-bit mode)
- [ ] Has 6 data pins: CLK, CMD, D0, D1, D2, D3
- [ ] 3.3V compatible
- [ ] SD card slot working
- [ ] Already soldered/connected

**Current Wiring:**
- CLK → GPIO 14 ✅
- CMD → GPIO 15 ✅
- D0 → GPIO 2 ✅
- D1 → GPIO 16 ✅
- D2 → GPIO 17 ✅
- D3 → GPIO 18 ✅
- VCC → 3.3V ✅
- GND → GND ✅

**SD Card Check:**
- [ ] SD card is Class 10 or higher
- [ ] Formatted as FAT32
- [ ] Has enough space (8GB+ recommended)

---

### 5. DS3231 RTC Module ✅

**Check:**
- [ ] Module has I2C pins: SDA, SCL
- [ ] Has CR2032 battery holder
- [ ] Battery installed and working
- [ ] 3.3V compatible
- [ ] Already soldered/connected

**Current Wiring:**
- SDA → GPIO 8 (shared with ToF) ✅
- SCL → GPIO 9 (shared with ToF) ✅
- VCC → 3.3V ✅
- GND → GND ✅

**Battery Check:**
- [ ] CR2032 battery installed
- [ ] Battery voltage > 2.5V (use multimeter)
- [ ] RTC keeps time when power off

---

### 6. OLED Display (SSD1306) ✅

**Check:**
- [ ] Display is 128x64 pixels
- [ ] 4-wire SPI interface (NO CS pin)
- [ ] Dual-color zones (yellow + blue)
- [ ] 3.3V compatible
- [ ] Already soldered/connected

**Current Wiring:**
- SCK → GPIO 7 ✅
- MOSI → GPIO 1 ✅
- DC → GPIO 35 ✅
- RST → GPIO 21 ✅
- VCC → 3.3V ✅
- GND → GND ✅

**Known Issue:**
- [ ] Solder bridge issue preventing backlight (will fix tomorrow)

---

### 7. Push Button ✅

**Check:**
- [ ] Normally Open (NO) momentary button
- [ ] 2 terminals
- [ ] Already soldered/connected

**Current Wiring:**
- Terminal 1 → GPIO 3 ✅
- Terminal 2 → GND ✅
- Internal pull-up enabled in software ✅

---

### 8. Neo-M9N GPS Module ⏳ NEW

**What to Buy:**
- u-blox Neo-M9N GPS module breakout board
- Must have: VCC, GND, TX, RX pins (4 pins minimum)
- Optional: PPS, RESET pins (not needed)

**Check Before Buying:**
- [ ] Module has 5V power input (Neo-M9N needs 5V)
- [ ] TX/RX pins are 3.3V logic compatible
- [ ] Has antenna (built-in or external)
- [ ] Supports NMEA output (default)

**Recommended Modules:**
- SparkFun GPS Breakout - NEO-M9N (Qwiic)
- Adafruit Ultimate GPS Breakout - Neo-M9N
- Generic Neo-M9N breakout boards

**Wiring Plan:**
- TX → GPIO 43 (ESP32 RX) - CROSSOVER!
- RX → GPIO 44 (ESP32 TX) - CROSSOVER!
- VCC → 5V (NOT 3.3V!)
- GND → GND

**Pre-Soldering Check:**
- [ ] GPS module has header pins soldered (or solder them)
- [ ] Antenna is connected (if external)
- [ ] Module is not damaged

---

### 9. Buzzer (Active or Passive) ⏳ NEW

**What to Buy:**
- 5V buzzer (active or passive)
- From Yurobot board or similar
- Must have: IN, VCC, GND pins (3 pins)

**Check Before Buying:**
- [ ] Buzzer operates at 5V (or 3.3V)
- [ ] Has 3 pins: IN (signal), VCC (power), GND
- [ ] Active buzzer: Built-in oscillator (simpler)
- [ ] Passive buzzer: Requires PWM (more control)

**Recommended:**
- Active buzzer: Easier to use, fixed frequency
- Passive buzzer: Better for volume control and tones

**Wiring Plan:**
- IN → GPIO 45 (PWM signal)
- VCC → 5V (or 3.3V, check buzzer specs)
- GND → GND

**Pre-Soldering Check:**
- [ ] Buzzer has header pins or wires
- [ ] Buzzer is not damaged
- [ ] Know if it's active or passive

---

### 10. Potentiometer 10kΩ ⏳ OPTIONAL

**What to Buy:**
- 10kΩ linear potentiometer
- 3 pins: Wiper (middle), End 1, End 2
- Rotary or slide type

**Check Before Buying:**
- [ ] Resistance: 10kΩ (can also use 5kΩ or 20kΩ)
- [ ] Has 3 pins accessible
- [ ] Knob or slider included

**Wiring Plan:**
- Wiper (middle) → GPIO 46 (ADC)
- One end → 3.3V
- Other end → GND

**Pre-Soldering Check:**
- [ ] Potentiometer has wires or pins
- [ ] Resistance measures ~10kΩ (use multimeter)
- [ ] Knob turns smoothly

**Note:** Potentiometer is OPTIONAL. You can set volume in code without it.

---

## 🔧 Soldering/Connection Checklist

### Pre-Soldering Preparation

**Tools Needed:**
- [ ] Soldering iron (if needed)
- [ ] Solder wire
- [ ] Wire strippers
- [ ] Multimeter
- [ ] Jumper wires (male-to-female, female-to-female)
- [ ] Breadboard (for testing)
- [ ] Heat shrink tubing (optional)

**Safety:**
- [ ] Work in well-ventilated area
- [ ] Have sponge/brass wool for cleaning iron
- [ ] Safety glasses
- [ ] Fire extinguisher nearby

---

### Component-by-Component Soldering Plan

#### GPS Module (Neo-M9N)

**Option 1: Breadboard Testing (Recommended First)**
```
1. Use jumper wires (no soldering)
2. Connect GPS to breadboard
3. Connect breadboard to ESP32
4. Test GPS functionality
5. If working, proceed to permanent connection
```

**Option 2: Direct Soldering**
```
1. Solder header pins to GPS module (if not pre-soldered)
2. Use female-to-female jumper wires to connect
3. Or solder wires directly to ESP32 pins
```

**Wiring:**
- [ ] GPS TX → ESP32 GPIO 43 (RX) - CROSSOVER!
- [ ] GPS RX → ESP32 GPIO 44 (TX) - CROSSOVER!
- [ ] GPS VCC → ESP32 5V
- [ ] GPS GND → ESP32 GND

**Testing:**
- [ ] Power on system
- [ ] Check serial output for GPS initialization
- [ ] Wait 26 seconds for first fix
- [ ] Verify GPS coordinates appear

---

#### Buzzer

**Option 1: Breadboard Testing (Recommended First)**
```
1. Use jumper wires (no soldering)
2. Connect buzzer to breadboard
3. Connect breadboard to ESP32
4. Test buzzer functionality
5. If working, proceed to permanent connection
```

**Option 2: Direct Soldering**
```
1. Solder wires to buzzer pins (if needed)
2. Use female-to-female jumper wires to connect
3. Or solder wires directly to ESP32 pins
```

**Wiring:**
- [ ] Buzzer IN → ESP32 GPIO 45 (PWM)
- [ ] Buzzer VCC → ESP32 5V (or 3.3V, check specs)
- [ ] Buzzer GND → ESP32 GND

**Testing:**
- [ ] Power on system
- [ ] Should hear startup melody
- [ ] Press button → should hear beep
- [ ] Trigger near-miss → should hear 3 beeps

---

#### Potentiometer (Optional)

**Option 1: Breadboard Testing (Recommended First)**
```
1. Use jumper wires (no soldering)
2. Connect potentiometer to breadboard
3. Connect breadboard to ESP32
4. Test volume control
5. If working, proceed to permanent connection
```

**Option 2: Direct Soldering**
```
1. Solder wires to potentiometer pins
2. Use female-to-female jumper wires to connect
3. Or solder wires directly to ESP32 pins
```

**Wiring:**
- [ ] Potentiometer Wiper → ESP32 GPIO 46 (ADC)
- [ ] Potentiometer End 1 → ESP32 3.3V
- [ ] Potentiometer End 2 → ESP32 GND

**Testing:**
- [ ] Power on system
- [ ] Play continuous tone
- [ ] Turn potentiometer knob
- [ ] Volume should change

---

## ⚡ Power Supply Checklist

### Current Power Requirements

| Rail | Typical | Peak | Components |
|------|---------|------|------------|
| 3.3V | 195 mA | 846 mA | ESP32, ToF, IMU, SD, OLED, RTC |
| 5V | 55 mA | 117 mA | GPS, Buzzer |
| **Total** | **250 mA** | **963 mA** | All components |

### Power Supply Options

**Option 1: USB Power (Recommended)**
- [ ] USB 3.0 port (900 mA available) ✅ Sufficient
- [ ] USB 2.0 port (500 mA available) ⚠️ Marginal
- [ ] Powered USB hub ✅ Best option

**Option 2: External 5V Supply**
- [ ] 5V/1A power adapter (1000 mA) ✅ Sufficient
- [ ] 5V/2A power adapter (2000 mA) ✅ Plenty of headroom

**Option 3: Battery Pack**
- [ ] 5V USB power bank ✅ Portable
- [ ] 3.7V LiPo + boost converter ⚠️ More complex

**Recommendation:** Use USB 3.0 port or powered USB hub for development, external 5V/1A adapter for field deployment.

---

## 🧪 Testing Plan (Before Final Assembly)

### Phase 1: Individual Component Testing

**Test Each Component Separately:**

1. **GPS Module**
   - [ ] Connect GPS only (no other new components)
   - [ ] Power on, wait for fix
   - [ ] Verify coordinates in serial output
   - [ ] Test for 5 minutes to ensure stability

2. **Buzzer**
   - [ ] Connect buzzer only (no GPS)
   - [ ] Power on, listen for startup melody
   - [ ] Test all patterns (near-miss, button, critical)
   - [ ] Verify volume is acceptable

3. **Potentiometer** (if using)
   - [ ] Connect potentiometer only
   - [ ] Power on, play continuous tone
   - [ ] Turn knob, verify volume changes
   - [ ] Check ADC readings in serial output

### Phase 2: Combined Testing

**Test All Components Together:**

1. **Power Budget Test**
   - [ ] Connect all components
   - [ ] Measure current draw with multimeter
   - [ ] Verify < 1A total
   - [ ] Check for voltage drops

2. **Functionality Test**
   - [ ] All sensors working
   - [ ] GPS acquiring fix
   - [ ] Buzzer playing sounds
   - [ ] Data logging to SD card
   - [ ] No crashes or resets

3. **Stress Test**
   - [ ] Run for 1 hour continuously
   - [ ] Check for overheating
   - [ ] Verify no memory leaks
   - [ ] Check SD card for data

---

## 📦 Final Assembly Checklist

### Before Permanent Assembly

- [ ] All components tested individually
- [ ] All components tested together
- [ ] Power supply verified sufficient
- [ ] No short circuits detected
- [ ] All wiring documented
- [ ] Photos taken of working setup

### Permanent Assembly Options

**Option 1: Breadboard (Temporary)**
- ✅ Easy to modify
- ✅ No soldering required
- ❌ Not portable
- ❌ Wires can come loose

**Option 2: Perfboard (Semi-Permanent)**
- ✅ More stable than breadboard
- ✅ Can be modified if needed
- ⚠️ Requires soldering
- ✅ Reasonably portable

**Option 3: Custom PCB (Permanent)**
- ✅ Most professional
- ✅ Very stable
- ❌ Expensive
- ❌ Cannot be modified easily

**Recommendation for Thesis:** Use perfboard for semi-permanent assembly. Allows modifications if needed while being stable enough for field testing.

---

## 🎯 Shopping List Summary

### Must Buy (For GPS and Buzzer)

| Item | Quantity | Estimated Cost | Where to Buy |
|------|----------|----------------|--------------|
| Neo-M9N GPS Module | 1 | $30-50 | SparkFun, Adafruit, AliExpress |
| 5V Buzzer (Active) | 1 | $2-5 | Yurobot board, Amazon, AliExpress |
| Jumper Wires (M-F) | 10-20 | $5-10 | Amazon, local electronics store |

**Total: ~$40-70**

### Optional

| Item | Quantity | Estimated Cost | Where to Buy |
|------|----------|----------------|--------------|
| 10kΩ Potentiometer | 1 | $2-5 | Amazon, local electronics store |
| Breadboard | 1 | $5-10 | Amazon, local electronics store |
| Perfboard | 1 | $5-10 | Amazon, local electronics store |

**Total Optional: ~$15-30**

---

## ✅ Final Checklist Before Ordering

### Verify You Have

**Already Have (Working):**
- [x] ESP32-S3-DevKitC-1
- [x] VL53L8CX ToF sensor
- [x] BNO08x IMU sensor
- [x] SD card module (SDMMC)
- [x] DS3231 RTC module
- [x] OLED display (SSD1306)
- [x] Push button
- [x] SD card (Class 10, FAT32)
- [x] CR2032 battery

**Need to Buy:**
- [ ] Neo-M9N GPS module (with antenna)
- [ ] 5V buzzer (active or passive)
- [ ] Jumper wires (if needed)
- [ ] Potentiometer 10kΩ (optional)

**Tools Check:**
- [ ] Soldering iron (if needed)
- [ ] Multimeter
- [ ] Wire strippers
- [ ] USB cable (for programming)
- [ ] Computer with ESP-IDF installed

---

## 📝 Notes and Reminders

### Important Points

1. **GPS needs 5V power** - Don't connect to 3.3V!
2. **UART is crossover** - TX→RX, RX→TX (this is correct!)
3. **GPIO 4 is occupied** - Use GPIO 46 for potentiometer
4. **Test before soldering** - Use breadboard first
5. **Take photos** - Document working setup before changes
6. **Backup code** - Commit to git before hardware changes

### Common Mistakes to Avoid

- ❌ Connecting GPS to 3.3V (needs 5V!)
- ❌ Connecting UART TX to TX (must be crossover!)
- ❌ Using GPIO 4 for potentiometer (already used by ToF!)
- ❌ Forgetting GND connections
- ❌ Not testing components individually first

---

## 🎓 For Your Thesis

### Hardware Documentation

**Include in Thesis:**
- Complete component list with part numbers
- Wiring diagrams (from `docs/complete-wiring-diagram.md`)
- Power budget calculations
- Photos of assembled system
- Testing procedures and results

**Hardware Justification:**
- Why Neo-M9N GPS? (Multi-constellation, high accuracy)
- Why active buzzer? (Simplicity, reliability)
- Why potentiometer? (User control, flexibility)

---

**Checklist Complete!**  
**Ready to order GPS and buzzer components.**  
**No more soldering needed after this!**
