# Sensor Integration Guide

**Last Updated**: March 27, 2026

---

## System Overview

The ProxiTof near-miss detection system integrates five sensors and two input devices, all managed through dedicated ESP-IDF components. Each sensor runs on a separate bus to avoid conflicts.

| Sensor | Interface | Sample Rate | Role |
|--------|-----------|-------------|------|
| VL53L8CX ToF | I2C (0x29) | 60 Hz | Distance measurement (4x4 grid) |
| BNO08x IMU | SPI (SPI2) | 100 Hz | Orientation, acceleration, angular velocity |
| Neo-M9N GPS | UART1 | 1 Hz (configurable) | Location, speed, time |
| DS3231 RTC | I2C (0x68) | On-demand | Battery-backed timekeeping |
| Buzzer | PWM (LEDC) | N/A | Audio alerts for near-miss events |
| Potentiometer | ADC1 | Polled at 10 Hz | Buzzer volume control |

Bus allocation:
- I2C0: ToF (0x29) + RTC (0x68) — shared bus, no address conflict
- SPI2: BNO08x IMU (dedicated)
- SPI3: SSD1306 OLED (dedicated, output only)
- UART1: Neo-M9N GPS (dedicated)
- LEDC PWM: Buzzer (GPIO 45)
- ADC1_CH3: Potentiometer (GPIO 4)

---

## Cross-Sensor Integration

### Why Multiple Sensors?

No single sensor can reliably detect a cycling near-miss on its own:

- **ToF alone** knows something is close and approaching fast, but can't tell if the cyclist reacted (maybe it was a wall they rode past intentionally).
- **IMU alone** knows the cyclist jerked or swerved, but can't tell why (pothole? wind gust? near-miss?).
- **GPS alone** knows where and how fast, but has no awareness of nearby objects.

The system fuses all three to answer: "Did an object approach dangerously close, and did the cyclist react?" — which is the definition of a near-miss.

| Sensor | Contribution | Without It |
|--------|-------------|------------|
| ToF | "How close is the object?" + "How fast is it approaching?" | No distance awareness — can't detect proximity events |
| IMU | "Did the cyclist react?" (jerk, swerve, dodge) | Can't confirm severity — every close pass looks the same |
| GPS | "Where did it happen?" + "How fast was the cyclist going?" | No geo-tagging — can't map dangerous locations |
| RTC | "When exactly did it happen?" (even without GPS/WiFi) | Timestamps may be wrong on first boot |
| Buzzer | Real-time audio feedback to cyclist | Cyclist doesn't know a near-miss was detected |


### How They Complement Each Other

The detection pipeline uses a tiered approach where ToF is the primary trigger and IMU provides confirmation:

**Tier 1 — Critical Proximity (ToF only, no IMU needed):**
Object at 20–100mm with approach rate >500mm/s. This is so dangerous that no body reaction is needed to confirm it. The ToF data alone is sufficient.

**Tier 2 — Proximity Alert (ToF only, no IMU needed):**
Object at <150mm with approach rate >1000mm/s. Fast approach in the near-miss zone. Even if the cyclist doesn't react (late reaction, didn't notice), this is still a near-miss.

**Tier 3 — Confirmed Near-Miss (ToF + IMU):**
Tier 2 conditions plus high jerk (>7 m/s³) from the IMU. The body reaction confirms the cyclist perceived the danger. This is tracked for training data quality and severity classification.

GPS enriches every detection with location context. The buzzer provides immediate feedback. The RTC ensures timestamps are always valid.

### Two Detection Paths

The system actually has two near-miss detection implementations running in parallel:

**1. Data Processor (`data_processor.cpp`)** — the formal detection pipeline:
- Drives the actual `near_miss_detected` flag, buzzer alerts, CSV `flag_auto`, and emergency SD flush
- Thresholds: rapid approach >1000mm/s, critical proximity 20–100mm + >500mm/s, jerk >7.0 m/s³
- Uses unfiltered jerk from raw IMU data
- Uses 5-sample adaptive moving average on distance (noise reduction with bypass for fast changes)

**2. OLED State Machine (`oled_display.c`)** — display-side detection with its own state machine:
- Drives the on-screen near-miss indicator and reason text (e.g., "CRITICAL: 85mm", "RAPID: 1200mm/s")
- Uses a time-based state machine (tracks approach start, duration, min distance during approach)
- Thresholds differ: rapid approach >800mm/s, critical proximity <200mm, jerk >50 m/s³ (filtered), angular velocity >300 deg/s
- Uses 5-sample moving average filtered jerk for display smoothness
- More conservative jerk threshold (50 vs 7) because the filtered jerk values are much lower than raw

Both paths evaluate independently. The data processor path is what matters for logging and alerts. The OLED path provides visual feedback with its own tuned thresholds.

---

## Initialization Order

The sensors are initialized sequentially in `app_main()` because some depend on others:

```
1. Config Manager        ← reads config.txt from SD card
2. Sensor Manager        ← initializes I2C bus, then ToF, then SPI bus, then IMU
   a. I2C bus init       (GPIO 8/9, 100kHz, internal pull-ups)
   b. I2C bus scan       (finds devices at 0x29, 0x57, 0x68)
   c. VL53L8CX ToF init  (firmware load ~1s, configure 4x4 @ 60Hz, start ranging)
   d. BNO08x IMU init    (SPI2 @ 3MHz, 5s timeout, enable 3 reports @ 100Hz)
3. GPS                   ← UART1 init, UBX config, starts background task
4. Smart Time            ← reads RTC via shared I2C bus (needs sensor manager first)
5. Storage Manager       ← mounts SD card, creates log file
6. Buzzer                ← LEDC PWM init, ADC init for potentiometer
7. OLED Display          ← SPI3 init (separate from IMU on SPI2)
8. ML Inference          ← loads TFLite model into memory
9. Data Processor        ← initializes sync window, feature state
10. Main Loop starts     ← begins reading sensors and processing
```

Key dependency: the RTC (step 4) shares the I2C bus with the ToF sensor, so it uses `sensor_manager_get_i2c_bus()` to get the already-initialized bus handle rather than creating a second I2C master.

Key isolation: the OLED uses SPI3 while the IMU uses SPI2. This was a critical implementation choice — the SSD1306 library originally defaulted to SPI2 (same as IMU), causing an `assert` crash on boot. Fixed by setting `CONFIG_SPI3_HOST=y` in sdkconfig.

---

## Handling Different Sampling Rates

The three main sensors run at different rates, and the main loop handles this with a polling architecture:

```
Main loop iteration (~every 1ms, no fixed delay):
├── sensor_manager_read_tof()   → returns ESP_OK ~60 times/sec
│                                  returns ESP_ERR_NOT_FOUND when no new frame
├── sensor_manager_read_imu()   → returns ESP_OK ~100 times/sec
│                                  returns ESP_ERR_NOT_FOUND when no new data
├── GPS                         → runs in background task, parsed asynchronously
│                                  gps_read() returns latest cached data (1Hz)
└── Potentiometer               → polled at 10Hz (every 100ms)
```

The main loop polls ToF and IMU as fast as possible. Both functions are non-blocking — they check if new data is ready and return immediately if not. This means:

- The loop runs much faster than either sensor's output rate
- Most iterations, `sensor_manager_read_tof()` returns `ESP_ERR_NOT_FOUND` (no new frame yet)
- When a new ToF frame arrives (~every 16.7ms at 60Hz), it's processed immediately
- IMU data arrives more frequently (~every 10ms at 100Hz)
- GPS data is never polled in the main loop — it runs in its own FreeRTOS task that continuously reads UART and parses NMEA sentences. Other components call `gps_read()` to get the latest cached position.

### The 10ms Synchronization Window

Since ToF (60Hz) and IMU (100Hz) produce data at different rates, the data processor synchronizes them:

1. When both ToF and IMU have new data with timestamps within 10ms of each other → they are paired directly.
2. When timestamps differ by more than 10ms → the most recent IMU reading is used with the current ToF frame (since IMU updates more frequently, the latest IMU value is always recent enough).
3. If timestamp drift exceeds the sync window consistently → a drift warning is logged.

The ToF timestamp is used as the reference timestamp for the synchronized pair, because ToF is the lower-rate sensor and its frames define the "heartbeat" of the detection pipeline.

### Why These Specific Rates?

- **ToF at 60Hz**: Maximum rate supported by VL53L8CX in 4x4 mode. Higher resolution (8x8) would reduce this to 15Hz — the 4x4 mode was chosen for speed over spatial resolution, since near-miss detection needs fast temporal response more than fine spatial detail.
- **IMU at 100Hz**: Fast enough to capture sudden jerk events (a swerve reaction happens in ~100–200ms, so 100Hz gives 10–20 samples during the event). The BNO08x supports up to 400Hz, but 100Hz was chosen to balance CPU load.
- **GPS at 1Hz**: Default rate, configurable up to 25Hz. For cycling near-miss detection, 1Hz is sufficient since GPS provides context (location, speed) rather than real-time detection data. Higher rates (5Hz) can be set in config for smoother route tracking.
- **Potentiometer at 10Hz**: Volume changes are slow human interactions — 10Hz is more than responsive enough.

---


## Sensor Data Flow (Step-by-Step)

```
                    ┌─────────────┐
                    │  Main Loop  │ (polls as fast as possible)
                    └──────┬──────┘
                           │
              ┌────────────┼────────────┐
              ▼            ▼            │
        ┌──────────┐ ┌──────────┐      │
        │ ToF Read │ │ IMU Read │      │  ┌──────────────┐
        │  (60Hz)  │ │ (100Hz)  │      │  │ GPS Task     │
        │  polled  │ │  polled  │      │  │ (background) │
        └────┬─────┘ └────┬─────┘      │  │ UART1 → NMEA │
             │             │            │  │ parse → cache │
             ▼             ▼            │  └──────┬───────┘
        ┌─────────────────────────┐     │         │
        │   Data Processor        │     │         │
        │  1. Sync (10ms window)  │     │         │
        │  2. Extract features    │     │         │
        │  3. Detect near-miss    │     │         │
        └────────┬────────────────┘     │         │
                 │                      │         │
    ┌────────────┼──────────┬───────────┤         │
    ▼            ▼          ▼           ▼         ▼
┌────────┐ ┌─────────┐ ┌────────┐ ┌──────────────────┐
│ OLED   │ │ Buzzer  │ │ LED    │ │ SD Card CSV Log  │
│Display │ │ Alert   │ │ Color  │ │ (ToF+IMU+GPS+    │
│(2 Hz)  │ │(on event│ │(cont.) │ │  flags+features) │
└────────┘ └─────────┘ └────────┘ └──────────────────┘
```

For one complete detection cycle:

1. Main loop calls `sensor_manager_read_tof()` — gets 16 distances (or "not ready")
2. Main loop calls `sensor_manager_read_imu()` — gets orientation + accel + gyro (or "not ready")
3. When both have valid data, `data_processor_synchronize_data()` pairs them within 10ms
4. `data_processor_extract_features()` calculates:
   - Min distance across 16 ToF zones (with 5-sample adaptive moving average — bypasses for fast changes >100mm)
   - Approach rate: `(current_min - previous_min) / delta_time`
   - Jerk magnitude: derivative of acceleration vector magnitude
   - Angular velocity magnitude: `sqrt(gyro_x² + gyro_y² + gyro_z²)`
5. `data_processor_detect_near_miss()` checks tiered conditions
6. If near-miss detected:
   - Buzzer plays 3-beep pattern
   - LED turns orange
   - `flag_auto = 1` in CSV log
   - Emergency flush to SD card (prevents data loss on power failure)
   - Event persists for 2 seconds after last detection
7. ML inference (if model loaded): normalizes 25 features (16 ToF + 9 IMU) using training scaler, runs TFLite model, threshold at 70% confidence
8. Detection modes combine algorithm and ML results:
   - Hybrid (default): algorithm OR ML
   - Algorithm only: rule-based only
   - Neural Network only: ML only
9. All data (ToF grid, IMU, derived features, GPS position, flags) written to CSV
10. OLED updated at configured refresh rate (default 2Hz)

---

## 1. VL53L8CX Time-of-Flight Sensor

### What It Does

Measures distance to objects using an infrared laser. Returns a 4x4 grid of 16 independent distance readings per frame — essentially a low-resolution depth camera. This is the primary sensor for detecting how close passing vehicles get to the cyclist.

### How It Works

The sensor emits a 940nm infrared VCSEL laser pulse and measures the round-trip time for the light to bounce back. Since the speed of light is known, the time directly gives distance. Each of the 16 zones has its own SPAD (Single-Photon Avalanche Diode) detector array. It uses its own IR light source, so it works in complete darkness.

### Specifications

| Parameter | Value |
|-----------|-------|
| Model | STMicroelectronics VL53L8CX |
| I2C address | 0x29 (7-bit) |
| Resolution | 4x4 zones (16 distances per frame) |
| Ranging frequency | 60 Hz |
| Range | 20mm – 4000mm |
| Field of view | 63° diagonal |
| IR wavelength | 940nm VCSEL (eye-safe Class 1) |
| Supply voltage | 3.3V |
| Current | 20mA typical, 40mA peak |
| Library | rjrp44/vl53l8cx (ESP-IDF component) |

### Wiring

```
ESP32-S3          VL53L8CX
--------          --------
GPIO 8  (SDA) ↔  SDA
GPIO 9  (SCL) →  SCL
GPIO 36 (LPN) →  LPN (reset/enable)
3.3V          →  VCC
GND           →  GND
```

I2C clock: 100kHz (reduced from 400kHz for stability). Internal pull-ups enabled. Bus shared with DS3231 RTC.

### Initialization and Configuration

1. I2C bus init (GPIO 8/9, 100kHz, internal pull-ups)
2. Register I2C device at address 0x29
3. Hardware reset via LPN pin (GPIO 36)
4. Alive check (`vl53l8cx_is_alive`)
5. Firmware load (`vl53l8cx_init`) — takes ~1 second as the sensor loads its internal DSP firmware
6. Set resolution to 4x4 (`VL53L8CX_RESOLUTION_4X4`)
7. Set ranging frequency to 60Hz
8. 200ms stabilization delay
9. Second alive check (verify sensor survived configuration)
10. Start ranging with 3 retry attempts

### How Data Is Read

`sensor_manager_read_tof()` is called every main loop iteration. It:
1. Calls `vl53l8cx_check_data_ready()` — returns immediately if no new frame
2. If ready, calls `vl53l8cx_get_ranging_data()` to get the 16 distance values
3. Flips the matrix (sensor is mounted backwards): both vertical and horizontal flip
4. Returns `tof_data_t` with 16 distances in mm and a timestamp

```c
typedef struct {
    uint32_t timestamp_ms;
    uint16_t distances[16];     // 4x4 grid, row-major order (mm)
    bool valid;
} tof_data_t;
```

### 4x4 Zone Layout (After Flip)

```
  Zone 0  | Zone 1  | Zone 2  | Zone 3
  --------+---------+---------+--------
  Zone 4  | Zone 5  | Zone 6  | Zone 7
  --------+---------+---------+--------
  Zone 8  | Zone 9  | Zone 10 | Zone 11
  --------+---------+---------+--------
  Zone 12 | Zone 13 | Zone 14 | Zone 15
```

### Distance Values and Edge Cases

| Reading | Meaning |
|---------|---------|
| 20–4000mm | Valid measurement |
| 0mm | No return signal |
| >4000mm (8191) | Out of range / target error |
| 9999mm | Code sentinel for "no valid reading" |

Objects closer than ~20mm saturate the detector — this is a physical limitation of all ToF sensors.

### Environmental Performance

| Condition | Performance |
|-----------|-------------|
| Darkness / night | Excellent — ideal, no ambient IR noise |
| Direct sunlight | Good — ambient light rejection built in, slight range reduction |
| Light rain | Good — droplets don't significantly scatter the beam |
| Heavy rain | Degraded — scattering reduces range |
| Dense fog | Poor — significant IR scattering |

### Error Recovery

If 50+ consecutive read errors occur (~1 second at 60Hz), the system automatically attempts recovery: stop ranging → re-init firmware → reconfigure → restart ranging. If recovery fails, the sensor is marked unhealthy but the system continues running.

### Practical Limitations Found During Implementation

- **I2C speed**: Had to reduce from 400kHz to 100kHz for reliable communication. At 400kHz, occasional NACK errors caused read failures.
- **LPN pin**: Originally used GPIO 4 for LPN, but GPIO 4 was reassigned to the potentiometer (ADC1_CH3). Moved LPN to GPIO 36 (free pin).
- **Firmware load time**: The ~1 second firmware load on every boot is unavoidable — the VL53L8CX has no internal flash and loads its DSP firmware from the host via I2C each time.
- **4x4 vs 8x8**: Chose 4x4 (60Hz) over 8x8 (15Hz) because temporal resolution matters more than spatial resolution for detecting fast-moving vehicles.
- **Sensor orientation**: The sensor was mounted backwards, requiring a double-flip (vertical + horizontal) in software. Discovered during testing when the depth map was mirrored.

---


## 2. BNO08x IMU (Inertial Measurement Unit)

### What It Does

Provides 9-DOF motion sensing: 3-axis orientation (roll/pitch/yaw), 3-axis linear acceleration, and 3-axis angular velocity. The BNO08x has an onboard Cortex-M0+ processor that runs sensor fusion algorithms internally, so the ESP32 receives pre-processed orientation data rather than raw magnetometer/accelerometer/gyro readings.

This sensor detects the cyclist's reaction to a near-miss — sudden swerving (high jerk), evasive maneuvers (angular velocity), and body movement patterns.

### Specifications

| Parameter | Value |
|-----------|-------|
| Model | Hillcrest/CEVA BNO08x |
| Interface | SPI (SPI2_HOST / FSPI) |
| SPI clock | 3 MHz |
| Sample rate | 100 Hz (all three report types) |
| Orientation | Rotation vector → Euler angles (roll, pitch, yaw) |
| Acceleration | Linear accelerometer (gravity removed by onboard fusion) |
| Gyroscope | Calibrated gyroscope (deg/s) |
| Supply voltage | 3.3V |
| Current | 10mA typical, 15mA peak |
| Library | esp32_BNO08x (ESP-IDF component) |

### Wiring

```
ESP32-S3          BNO08x
--------          ------
GPIO 11 (MOSI) →  DI
GPIO 13 (MISO) ←  SDA (MISO)
GPIO 12 (SCK)  →  SCK
GPIO 10 (CS)   →  CS
GPIO 5  (INT)  ←  INT
GPIO 6  (RST)  →  RST
3.3V           →  VCC, P0, P1
GND            →  GND
FLOATING       →  BT (leave disconnected)
```

P0=HIGH and P1=HIGH selects SPI mode. BT must be floating (not connected).

### Initialization and Configuration

1. Create BNO08x config (SPI2_HOST, 3MHz, all pin assignments)
2. Allocate BNO08x object on heap (`new BNO08x(config)`)
3. Run `imu->initialize()` in a separate FreeRTOS task with 5-second timeout — the BNO08x takes ~2 seconds to load its internal firmware
4. If timeout: kill the init task, delete the object, mark unhealthy
5. Enable rotation vector reports at 100Hz (10ms = 10000µs interval)
6. Enable linear accelerometer reports at 100Hz
7. Enable calibrated gyroscope reports at 100Hz

The 5-second timeout with a separate task was an implementation choice to prevent the entire system from hanging if the IMU doesn't respond (e.g., wiring issue).

### How Data Is Read

`sensor_manager_read_imu()` is called every main loop iteration. It:
1. Calls `imu->data_available()` — returns false if no new data
2. Checks each report type independently (rotation vector, linear accel, gyro)
3. Each report has its own `has_new_data()` flag — they may arrive at slightly different times
4. Returns `imu_data_t` with all fields populated

```c
typedef struct {
    uint32_t timestamp_ms;
    float roll, pitch, yaw;           // Orientation (degrees)
    float accel_x, accel_y, accel_z;  // Linear acceleration (m/s², gravity removed)
    float gyro_x, gyro_y, gyro_z;    // Angular velocity (deg/s)
    bool valid;
} imu_data_t;
```

### Three Report Types and What They Provide

1. **Rotation Vector** → Euler angles (roll, pitch, yaw in degrees)
   - Uses onboard sensor fusion (accelerometer + gyroscope + magnetometer)
   - More accurate than computing orientation from raw sensors on the ESP32
   - Logged to CSV for post-analysis of cyclist body position

2. **Linear Accelerometer** → acceleration with gravity removed (m/s²)
   - The BNO08x internally subtracts the gravity vector
   - Pure body movement acceleration — used to calculate jerk
   - Jerk = derivative of acceleration = rate of change of force on the body

3. **Calibrated Gyroscope** → angular velocity (deg/s)
   - Measures rotation speed around each axis
   - Used to detect evasive maneuvers (sudden turns/swerves)
   - If no new gyro data on a given read, values are set to 0.0

### Role in Near-Miss Detection

- **Jerk magnitude**: `sqrt(jerk_x² + jerk_y² + jerk_z²)` where jerk = `(current_accel - previous_accel) / delta_time`
  - In the data processor: calculated from raw (unfiltered) IMU data — this is what goes into CSV logs and ML inference
  - In the OLED display: filtered with 5-sample moving average (circular buffer, ~50ms window at 100Hz) to smooth the displayed value and reduce false triggers in the display's own state machine
  - Data processor threshold: >7.0 m/s³ (raw) indicates sudden body reaction
  - OLED state machine threshold: >50.0 m/s³ (filtered) — higher because the moving average reduces peak values
  - Normal cycling vibration: <5 m/s³
  - Turning: gradual, low jerk
  - Bumpy road: high-frequency, filtered out

- **Angular velocity magnitude**: `sqrt(gyro_x² + gyro_y² + gyro_z²)`
  - High values (>300 deg/s) indicate evasive spinning/turning

### Re-initialization at Runtime

The IMU can be re-initialized without rebooting via `sensor_manager_reinit_imu()`. Triggered from OLED IMU Focus mode (Mode 3) by pressing ACTION. Useful if the IMU stops responding after a communication glitch.

### Practical Limitations Found During Implementation

- **SPI bus isolation**: The IMU must be on a separate SPI bus from the OLED. Both use SPI, but sharing a bus caused initialization conflicts. IMU uses SPI2, OLED uses SPI3.
- **Initialization timeout**: The BNO08x occasionally hangs during init if wiring is marginal. The 5-second timeout with a separate FreeRTOS task prevents the entire system from blocking.
- **Protocol select pins**: P0 and P1 must both be at 3.3V for SPI mode. If either is floating, the sensor defaults to I2C and SPI communication fails silently.
- **BT pin**: Must be floating (disconnected). Connecting it to GND or 3.3V puts the sensor in bootloader mode.
- **Report independence**: The three report types (rotation vector, linear accel, gyro) arrive independently. A single `data_available()` call may have new rotation data but stale gyro data. The code handles this by checking each report's `has_new_data()` separately.

---

## 3. Neo-M9N GPS Module

### What It Does

Provides geographic location (latitude, longitude, altitude), ground speed, heading, and precise UTC time from satellites. Used for geo-tagging near-miss events and route tracking. Runs entirely in a background FreeRTOS task — the main loop never blocks on GPS.

### Specifications

| Parameter | Value |
|-----------|-------|
| Model | u-blox Neo-M9N |
| Interface | UART1 (38400 baud, 8N1) |
| Default update rate | 1 Hz (configurable up to 25 Hz via UBX commands) |
| Constellations | GPS, GLONASS, Galileo, BeiDou, QZSS, SBAS (all 6 enabled) |
| Tracking channels | 92 (31 configured across constellations) |
| Horizontal accuracy | 2.0m CEP (multi-GNSS) |
| Cold start TTFF | 24–29 seconds |
| Hot start TTFF | 1–2 seconds (with backup battery) |
| Supply voltage | 3.3V |
| Current | 25mA typical, 67mA peak |

### Wiring

```
ESP32-S3          Neo-M9N
--------          -------
GPIO 38 (TX) →  RX (GPS receives commands)
GPIO 47 (RX) ←  TX (GPS sends NMEA data)
3.3V         →  VCC
GND          →  GND
```

UART crossover: ESP32 TX → GPS RX, ESP32 RX ← GPS TX. GPIO 38/47 are free pins with no conflicts. GPIO 43/44 are UART0 (console) — must not be used for GPS.

### Initialization and Configuration

1. Configure UART1 (38400 baud, 8N1, no flow control)
2. Install UART driver with 2KB receive buffer
3. Set UART pins (TX=38, RX=47)
4. Send UBX CFG-ANT commands for both IPEX and SMA antenna (module auto-selects)
5. Send UBX CFG-GNSS to enable all 6 GNSS systems (GPS 8ch, GLONASS 8ch, Galileo 4ch, BeiDou 8ch, QZSS 2ch, SBAS 1ch)
6. Send UBX CFG-NAV5 to set dynamic platform model (default: Bike)
7. Send UBX CFG-CFG to save configuration to GPS flash (persists across power cycles)
8. Start background FreeRTOS task (`gps_task`) for continuous NMEA parsing

All UBX commands include proper checksum calculation. Configuration is saved to the GPS module's flash, so it only needs to be sent once (but is sent every boot for safety).

### How Data Is Read

GPS runs in its own FreeRTOS task that:
1. Continuously reads bytes from UART1
2. Assembles complete NMEA sentences (terminated by `\r\n`)
3. Verifies checksum before parsing
4. Parses $GPGGA (position), $GPRMC (date/time/speed), $GPVTG (track/speed)
5. Updates a cached `gps_data_t` structure protected by the task's execution context

Other components call `gps_read(&data)` to get the latest cached data — this is non-blocking and returns immediately.

```c
typedef struct {
    double latitude, longitude;
    float altitude_m, speed_kmh, speed_ms, heading_deg;
    uint8_t hour, minute, second, day, month;
    uint16_t year;
    gps_fix_quality_t fix_quality;
    uint8_t satellites_used;
    float hdop;
    bool valid;
    uint32_t last_update_ms;
} gps_data_t;
```

### Dynamic Platform Models

The GPS Kalman filter is optimized for different movement types. Configurable from `config.txt` or cycled from OLED GPS mode (Mode 4):

| Model | Name | Best For |
|-------|------|----------|
| 0 | Portable | General purpose |
| 3 | Pedestrian | Walking (0–10 km/h) |
| 4 | Automotive | Driving (0–100 km/h) |
| 10 | Bike | Cycling (0–30 km/h) — recommended |

### Role in the System

- Geo-tags every logged CSV row with latitude, longitude, altitude
- Provides ground speed and heading for data enrichment
- UTC time used as one of three time sources (GPS > WiFi NTP > RTC)
- Enables post-analysis: near-miss heatmaps, dangerous road identification
- Can be paused during USB transfer mode (`gps_pause()`) to free CPU, resumed after (`gps_resume()`)

### Practical Limitations Found During Implementation

- **Pin conflict**: Originally planned for GPIO 43/44, but those are UART0 (console). Moved to GPIO 38/47 (free pins).
- **Baud rate**: Neo-M9N defaults to 38400, not 9600 like older modules. Using wrong baud rate produces garbled data.
- **Indoor testing**: GPS will never get a fix indoors. NMEA sentences are still received (the module is alive), but `valid` stays false. Must test outdoors.
- **Cold start time**: First fix takes ~26 seconds outdoors. The backup battery enables hot starts (1–2s) on subsequent power-ons within 4 hours.
- **Background task**: GPS parsing runs in its own FreeRTOS task rather than being polled in the main loop. This prevents UART buffer overflows — at 38400 baud, data arrives continuously and must be consumed promptly.
- **Configuration persistence**: UBX config is saved to GPS flash memory, so the module remembers its settings across power cycles. This avoids re-sending all config commands on every boot (though we do it anyway for safety).

---


## 4. DS3231 RTC (Real-Time Clock)

### What It Does

Maintains accurate time even when the device is powered off, using a CR2032 backup battery. Ensures timestamps in logged data are always correct, even without GPS fix or internet.

### Specifications

| Parameter | Value |
|-----------|-------|
| Model | DS3231 |
| Interface | I2C (shared bus with ToF) |
| I2C address | 0x68 (7-bit) |
| Accuracy | ±2 ppm (±1 minute/year) |
| Backup battery | CR2032 |
| Current | 0.1mA typical |

### Wiring

```
ESP32-S3          DS3231
--------          ------
GPIO 8  (SDA) ↔  SDA (shared with ToF)
GPIO 9  (SCL) →  SCL (shared with ToF)
3.3V          →  VCC
GND           →  GND
```

SQW and 32K pins are not connected (not needed for basic timekeeping).

### How It Integrates

The RTC is accessed through the shared I2C bus handle from the sensor manager (`sensor_manager_get_i2c_bus()`). The smart time system (`components/smart_time/`) manages three time sources with priority:

1. **WiFi NTP** (most accurate, requires internet connection)
2. **GPS time** (accurate, requires satellite fix — available after ~26s outdoors)
3. **DS3231 RTC** (always available with battery — the fallback)

On boot, the system reads the RTC to set the initial system clock immediately. If WiFi or GPS later provides a more accurate time, the RTC is updated to match. This ensures the device always has a reasonable timestamp for data logging, even on first boot in a field with no connectivity.

### Practical Limitations Found During Implementation

- **Shared I2C bus**: The RTC shares the bus with the ToF sensor. Both use different addresses (0x29 vs 0x68) so there's no conflict, but the RTC must be initialized after the sensor manager creates the I2C bus.
- **Battery dependency**: Without a CR2032 battery, the RTC loses time on every power cycle. The system falls back to GPS time or a default timestamp, but CSV filenames may be wrong until a valid time source is available.
- **I2C scan noise**: During the I2C bus scan at boot, every address that doesn't have a device responds with NACK. This produces a wall of error messages in the serial log (harmless but noisy). The scan successfully finds devices at 0x29 (ToF), 0x57, and 0x68 (RTC).

---

## 5. Buzzer and Potentiometer

### What They Do

The buzzer provides audio feedback for near-miss events, button presses, and system status. The potentiometer controls buzzer volume in real-time.

### Buzzer Specifications

| Parameter | Value |
|-----------|-------|
| Interface | LEDC PWM (10-bit resolution, 0–1023 duty) |
| GPIO | 45 |
| Default frequency | 2000 Hz |
| Frequency range | 100–10000 Hz |

### Potentiometer Specifications

| Parameter | Value |
|-----------|-------|
| Interface | ADC1_CH3 (12-bit, 0–4095) |
| GPIO | 4 |
| Resistance | 10kΩ |
| Voltage range | 0–3.3V (voltage divider) |

### Wiring

```
ESP32-S3          Buzzer              Potentiometer (10kΩ)
--------          ------              --------------------
GPIO 45 (PWM) →  Signal              GPIO 4 (ADC) → Wiper (middle)
                                      3.3V         → One end
                                      GND          → Other end
```

### Initialization and Configuration

1. Configure LEDC timer (low-speed mode, 10-bit resolution, default 2000Hz)
2. Configure LEDC channel on GPIO 45 (starts silent, duty=0)
3. Initialize ADC1 for potentiometer (if enabled in config):
   - ADC unit: ADC1 (works with WiFi — ADC2 conflicts with WiFi radio)
   - Channel: ADC1_CH3 (GPIO 4)
   - Attenuation: 12dB (0–3.3V range)
   - Resolution: 12-bit (0–4095)
   - Test read to verify ADC is working
4. Create FreeRTOS software timer for pattern playback (100ms tick)

### How Data Is Read

The potentiometer is polled at 10Hz (every 100ms) in the main loop:
1. `buzzer_update_volume_from_pot()` reads ADC value (0–4095)
2. Maps to volume percentage (0–100%)
3. If volume changed significantly, updates PWM duty cycle
4. Volume change triggers a popup on the OLED display

### Alert Patterns

| Pattern | Sound | Trigger |
|---------|-------|---------|
| NEAR_MISS | 3 short beeps (200ms on, 100ms off) | Near-miss event detected |
| CRITICAL | Continuous tone | Critical proximity (<100mm) |
| BUTTON | Single short beep (100ms) | Button press feedback |
| STARTUP | Ascending melody | Device boot complete |
| SHUTDOWN | Descending melody | Safe shutdown initiated |
| ERROR | Error tone | System error |

Patterns play in the background using a FreeRTOS software timer — non-blocking, so the main loop continues processing while the buzzer plays.

### Practical Limitations Found During Implementation

- **ADC1 vs ADC2**: The potentiometer was originally on GPIO 46 (ADC2_CH5), but ADC2 cannot be used while WiFi is active. Moved to GPIO 4 (ADC1_CH3) which works regardless of WiFi state.
- **GPIO 4 conflict**: GPIO 4 was originally used for the ToF LPN pin. When the potentiometer was moved to GPIO 4, the ToF LPN was relocated to GPIO 36.
- **Volume popup**: Rapid potentiometer changes caused OLED flicker. Added a popup overlay system that shows volume for 2 seconds then fades, rather than redrawing the entire display.

---

## Sensor Health and Graceful Degradation

```c
typedef struct {
    bool tof_healthy;
    bool imu_healthy;
    uint32_t tof_error_count;
    uint32_t imu_error_count;
} sensor_health_t;
```

The system continues running even if individual sensors fail:

| Failure | Impact | Behavior |
|---------|--------|----------|
| ToF fails | No distance data | Near-miss detection disabled, IMU still logged |
| IMU fails | No motion data | Detection runs on ToF only (Tier 1/2 still work) |
| GPS no fix | No location | Data logged without geo-tags, timestamps from RTC |
| RTC battery dead | No persistent time | Falls back to GPS time or default timestamp |
| SD card error | No logging | System runs normally, data not saved |
| Buzzer disconnected | No audio | Detection still works, LED still indicates events |

Health status is visible on OLED Status mode (Mode 1) and in serial output. The LED blinks red for sensor errors.

---

## CSV Log Format

Each row contains all sensor data fused together:

```
timestamp_ms,
tof_00, tof_01, tof_02, tof_03,    ← ToF 4x4 grid (16 columns, mm)
tof_10, tof_11, tof_12, tof_13,
tof_20, tof_21, tof_22, tof_23,
tof_30, tof_31, tof_32, tof_33,
roll, pitch, yaw,                    ← IMU orientation (degrees)
accel_x, accel_y, accel_z,          ← Linear acceleration (m/s²)
gyro_x, gyro_y, gyro_z,            ← Angular velocity (deg/s)
min_distance,                        ← Derived: min across ToF grid
approach_rate,                       ← Derived: distance change rate (mm/s)
jerk,                                ← Derived: acceleration change rate (m/s³)
angular_velocity,                    ← Derived: rotation magnitude (deg/s)
latitude, longitude, altitude,       ← GPS position
speed_kmh, heading,                  ← GPS velocity
satellites, hdop,                    ← GPS quality
flag_manual, flag_auto, confidence   ← Event flags (button, algorithm/ML, ML probability)
```

This format captures the complete sensor state at each moment, enabling both real-time detection and offline analysis/ML training.

---

## Implementation Files

| File | Contents |
|------|----------|
| `components/sensor_manager/sensor_manager.cpp` | ToF + IMU init, read, recovery, I2C/SPI bus management |
| `components/sensor_manager/include/sensor_manager.h` | Pin definitions, data structures, API |
| `components/gps/gps.c` | GPS UART driver, NMEA parsing, UBX commands, background task |
| `components/gps/include/gps.h` | GPS data structures, dynamic models, API |
| `components/buzzer/buzzer.c` | PWM driver, alert patterns, ADC volume control |
| `components/buzzer/include/buzzer.h` | Buzzer config, pattern enum, API |
| `components/rtc_ds3231/` | DS3231 RTC I2C driver |
| `components/smart_time/smart_time.c` | Time source priority (RTC + GPS + WiFi NTP) |
| `components/data_processor/data_processor.cpp` | Sensor sync, feature extraction, near-miss detection |
| `components/data_processor/include/data_processor_types.h` | Feature vector, synced data structures |
| `components/storage_manager/storage_manager.c` | CSV logging to SD card |
| `components/oled_display/oled_display.c` | Sensor data visualization (depth map, modes) |
| `main/main.c` | Main loop — polling, pipeline orchestration, ML inference |

---

## Related Documentation

- [complete-wiring-diagram.md](complete-wiring-diagram.md) — Full hardware wiring for all components
- [gpio-pin-reference.md](gpio-pin-reference.md) — GPIO assignments verified from code
- [near-miss-detection-algorithm.md](near-miss-detection-algorithm.md) — Detection algorithm details
- [oled-display-guide.md](oled-display-guide.md) — Display modes and depth map visualization
- [smart-time-system-guide.md](smart-time-system-guide.md) — Time source management (RTC + GPS + WiFi)
- [data-collection-field-guide.md](data-collection-field-guide.md) — Field data collection procedures
- [ml-complete-guide.md](ml-complete-guide.md) — ML training and deployment
