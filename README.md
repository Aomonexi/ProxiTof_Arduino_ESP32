# ProxiTof Near-Miss Detection System

An ESP32-S3 based multi-sensor proximity detection system with near-miss detection algorithms, machine learning capabilities, GPS geotagging, audio alerts, OLED visualization, smart time management, and field-configurable operation for near-miss event detection and data logging.

## Overview

The ProxiTof system combines a VL53L8CX Time-of-Flight sensor (4x4 distance grid at 60Hz) with a BNO08x 9-DOF IMU (orientation and motion at 100Hz) to detect and predict near-miss collision events. A three-tier detection algorithm (proximity + approach rate, jerk reaction, combined) identifies near-miss events in real-time. The system features GPS geotagging, buzzer audio alerts, OLED real-time visualization, comprehensive time management, field configuration, USB mass storage access, and logs synchronized sensor data to SD card for machine learning training.

## Key Features

### Core Sensing & Detection
- **Multi-sensor fusion**: VL53L8CX ToF (4x4 grid @ 60Hz) + BNO08x IMU (9-DOF @ 100Hz)
- **Three-tier near-miss detection**: Proximity+approach, jerk reaction, combined analysis
- **ML inference**: TensorFlow Lite Micro with ESP-NN acceleration for on-device detection
- **Hybrid detection modes**: Algorithm-only, ML-only, or hybrid (configurable)
- **High-speed data logging**: Synchronized sensor data with circular buffering
- **Manual event flagging**: Button-based annotation during data collection

### Real-Time Feedback
- **OLED display**: SSD1306 128x64 SPI with 9 display modes (Dashboard, Status, ToF Focus, IMU Focus, GPS, Near-Miss, WiFi Settings, USB Transfer, Shutdown)
- **WS2812 RGB LED**: Distance-based color coding + near-miss flash alerts
- **Buzzer audio alerts**: PWM-driven with 5 alert patterns (startup, near-miss, critical, button, shutdown)
- **Potentiometer volume control**: Optional analog volume adjustment via ADC

### GPS Geotagging
- **Neo-M9N GNSS module**: Multi-constellation (GPS + GLONASS + Galileo + BeiDou)
- **Location logging**: Latitude, longitude, speed, heading logged to CSV
- **Configurable update rate**: 1-25 Hz via config file
- **Dynamic model selection**: Bike, Pedestrian, Portable, Automotive modes
- **No external library needed**: Built-in ESP-IDF UART driver + manual NMEA parsing

### Smart Time Management
- **Multi-tier time system**: DS3231 RTC + ESP32 internal RTC + WiFi NTP sync
- **Battery-backed accuracy**: DS3231 maintains time during power cycles
- **Automatic fallback**: Graceful degradation when components unavailable

### Field Configuration
- **SD card configuration**: Edit `/sdcard/config.txt` without recompilation
- **WiFi credentials**: Field-configurable network settings
- **System parameters**: Adjustable logging rates, thresholds, detection modes, GPS settings, and hardware pins

### Computer Interface
- **Dual USB ports**: RIGHT = COM/UART (programming/serial), LEFT = USB-OTG (mass storage)
- **USB Mass Storage**: Access SD card as flash drive via LEFT USB port
- **USB Serial access**: Direct computer connection for configuration via RIGHT USB port
- **Python companion tools**: Automated device detection, time sync, data analysis, ML training

### Machine Learning Pipeline
- **TensorFlow training**: Complete ML pipeline with Python environment
- **TensorFlow Lite deployment**: On-device inference with ESP-NN acceleration
- **Structured data export**: CSV format optimized for ML training
- **Model management**: Easy model updates and performance monitoring

## Hardware Requirements

### Core Components

| Component | Interface | Purpose |
|-----------|-----------|---------|
| **ESP32-S3-DevKitC-1** (N16R8) | - | Main controller (16MB Flash, 8MB PSRAM) |
| **VL53L8CX** ToF sensor | I2C | 4x4 distance grid @ 60Hz (0-4000mm) |
| **BNO08x** 9-DOF IMU | SPI | Orientation + acceleration @ 100Hz |
| **MicroSD card** (Class 10+) | SDMMC 4-bit | High-speed data logging (FAT32) |
| **Adafruit Micro SD SDIO Breakout** (#4682) | SDMMC | SD card interface |
| **DS3231 RTC module** | I2C (shared) | Battery-backed real-time clock |
| **SSD1306 OLED 128x64** (7-pin SPI) | SPI | Real-time sensor visualization |
| **Neo-M9N GPS module** | UART | Location geotagging |
| **Buzzer** (Yurobot board: IN, VCC, GND) | PWM | Audio near-miss alerts |
| **Push button** (x2) | GPIO | Flag button + Mode switch button |
| **WS2812 RGB LED** | GPIO | Built-in status indicator |

### Optional Components

| Component | Purpose |
|-----------|---------|
| **10kΩ Potentiometer** | Buzzer volume control (ADC) |


### Complete Wiring Diagram

```
ESP32-S3 Pin Assignments (All Verified, Conflict-Free):

VL53L8CX ToF Sensor (I2C):
  SDA:  GPIO 8
  SCL:  GPIO 9
  LPN:  GPIO 36
  VCC:  3.3V
  GND:  GND

BNO08x IMU (SPI):
  MOSI: GPIO 11
  MISO: GPIO 13
  SCK:  GPIO 12
  CS:   GPIO 10
  INT:  GPIO 5
  RST:  GPIO 6
  P0:   3.3V (SPI mode)
  P1:   3.3V (SPI mode)
  BT:   FLOATING

DS3231 RTC (I2C - shared bus with ToF):
  SDA:  GPIO 8
  SCL:  GPIO 9
  VCC:  3.3V
  GND:  GND
  Battery: CR2032

SD Card (SDMMC 4-bit mode):
  CLK:  GPIO 14
  CMD:  GPIO 15
  D0:   GPIO 2
  D1:   GPIO 16
  D2:   GPIO 17
  D3:   GPIO 18

OLED Display SSD1306 (SPI - 7 pin module):
  SCK:  GPIO 7   (Pin 3: D0)
  MOSI: GPIO 1   (Pin 4: D1)
  DC:   GPIO 35  (Pin 6: D/C)
  RST:  GPIO 21  (Pin 5: RES)
  CS:   GPIO 0   (Pin 7: CS)
  VCC:  3.3V     (Pin 2)
  GND:  GND      (Pin 1)
  Note: Dual-color zones - top 16px yellow, bottom 48px blue

Neo-M9N GPS (UART1):
  TX:   GPIO 38  → GPS RX (crossover wiring - this is correct!)
  RX:   GPIO 47  ← GPS TX (crossover wiring - this is correct!)
  VCC:  5V (if breakout has regulator) or 3.3V (bare module)
  GND:  GND

Buzzer (PWM):
  IN:   GPIO 45
  VCC:  5V or 3.3V (check buzzer specs)
  GND:  GND

Potentiometer - Volume Control (Optional):
  Wiper: GPIO 4  (ADC1_CH3 - WiFi compatible)
  End 1: 3.3V
  End 2: GND

Buttons:
  Flag Button: GPIO 3  (internal pull-up, active LOW)
  Mode Button: GPIO 20 (internal pull-up, active LOW)

LED:
  WS2812: GPIO 48 (built-in on DevKitC-1)
```

See [Complete Wiring Diagram](docs/complete-wiring-diagram.md) for detailed wiring with troubleshooting.

## Software Requirements

### Development Environment

- **ESP-IDF v5.5+** (Espressif IoT Development Framework)
- **Python 3.13** (for ML training and USB tools)
- **Git** (for version control)

This project uses ESP-IDF framework (not Arduino or PlatformIO) for optimal sensor support, ML performance, and advanced features like SDMMC interface.

### ESP-IDF Installation

See [ESP-IDF Installation Guide](docs/esp-idf-installation-guide.md) for detailed setup.

```powershell
# Windows - set up environment before each build session
C:\Users\[username]\esp\v5.5.1\esp-idf\export.ps1

# Verify
idf.py --version
```

### Python Environment Setup

```bash
python -m venv venv
venv\Scripts\activate          # Windows
pip install -r requirements_proxitof_thesis.txt
pip install pyserial            # For USB tool
```

## Quick Start

### 1. Hardware Setup
Follow the [Complete Wiring Diagram](docs/complete-wiring-diagram.md) to connect all components.

### 2. Build and Flash

```bash
# Set up ESP-IDF environment
C:\Users\[username]\esp\v5.5.1\esp-idf\export.ps1

# Build
idf.py build

# Flash and monitor (use RIGHT USB port - COM/UART)
idf.py flash monitor
```

### 3. Initial Configuration

Format SD card as FAT32, insert it, and the system creates a default `config.txt`. Edit it:

```ini
# WiFi Settings
wifi_enabled=1
wifi_ssid=YourWiFiName
wifi_password=YourWiFiPassword

# Time Management
timezone=EST5EDT,M3.2.0,M11.1.0
sync_interval_hours=24

# GPS Settings
gps_logging_enabled=1
gps_update_rate=1
gps_dynamic_model=1

# Detection Mode (0=Hybrid, 1=Algorithm, 2=ML)
detection_mode=0
```

See [Field Configuration Guide](docs/field-configuration-guide.md) for all parameters.

### 4. USB Ports

The ESP32-S3-DevKitC-1 has two USB ports:
- **RIGHT port** (COM/UART): Programming, serial monitor, USB serial commands
- **LEFT port** (USB-OTG): USB Mass Storage mode (access SD card as flash drive)

```bash
# USB serial tool (RIGHT port)
python tools/proxitof_usb_tool.py --status
python tools/proxitof_usb_tool.py --sync-time
```

### 5. Data Collection

The system automatically begins logging when SD card is inserted and sensors initialize. Press the flag button during near-miss events to annotate data.


## Near-Miss Detection

### Three-Tier Detection Algorithm

The system uses a priority-based detection algorithm with sensor fusion:

| Priority | Trigger | Condition | Example |
|----------|---------|-----------|---------|
| **P1: Critical Proximity** | Distance + Approach Rate | 20-100mm AND >500 mm/s approach | Motorcycle passing at close range |
| **P2: Rapid Approach** | Approach Rate + Jerk | >1000 mm/s approach AND >7 m/s³ jerk | Sudden swerve reaction to vehicle |
| **P3: Combined** | Distance + Time Window | 20-150mm within 500ms window | Close pass with delayed reaction |

Detection filters out false positives from:
- Sensor blockage (distance < 20mm)
- Normal walking/pedaling vibrations (jerk < 7 m/s³)
- Gradual turning (low jerk, high angular velocity)
- Bumpy road surfaces (high-frequency vibration patterns)

### Detection Modes

Configurable via `config.txt` or OLED mode button:
- **Mode 0 - Hybrid**: Algorithm + ML neural network (recommended)
- **Mode 1 - Algorithm Only**: Rule-based detection (no ML model needed)
- **Mode 2 - Neural Network Only**: TFLite model inference

### ML Pipeline

1. Collect training data with manual button flags
2. Train TensorFlow model: `python tools/quick_start_ml.py`
3. Convert to TFLite: `python tools/convert_to_tflite.py`
4. Flash model with firmware for real-time inference

See [Near-Miss Detection Algorithm](docs/near-miss-detection-algorithm.md) and [ML Complete Guide](docs/ML_COMPLETE_GUIDE.md).

## Data Format

### CSV Structure

Log files are saved to `/sdcard/YYYY-MM-DD/log_YYYYMMDD_HHMMSS.csv`:

```csv
timestamp_ms,tof_00,tof_01,...,tof_33,roll,pitch,yaw,accel_x,accel_y,accel_z,gyro_x,gyro_y,gyro_z,latitude,longitude,speed_ms,heading_deg,gps_satellites,flag_manual,flag_auto
```

| Column(s) | Description |
|-----------|-------------|
| `timestamp_ms` | Milliseconds since boot |
| `tof_00` to `tof_33` | 16 ToF distances in mm (4x4 grid, row-major) |
| `roll`, `pitch`, `yaw` | Orientation in degrees |
| `accel_x/y/z` | Linear acceleration in m/s² |
| `gyro_x/y/z` | Angular velocity in deg/s |
| `latitude`, `longitude` | GPS coordinates (decimal degrees, if GPS enabled) |
| `speed_ms` | GPS ground speed in m/s |
| `heading_deg` | GPS heading in degrees (0-360) |
| `gps_satellites` | Number of satellites used |
| `flag_manual` | Manual button flag (0 or 1) |
| `flag_auto` | Algorithm/ML auto-detection flag (0 or 1) |

## OLED Display Modes

The SSD1306 OLED cycles through 9 modes via the mode button (GPIO 20):

| Mode | Name | Shows |
|------|------|-------|
| 0 | Dashboard | Full overview: ToF grid + IMU + stats |
| 1 | Status | System health and component status |
| 2 | ToF Focus | Large 4x4 distance grid with bar graphs |
| 3 | IMU Focus | Detailed orientation and acceleration |
| 4 | GPS | Location, satellites, speed, heading, fix status |
| 5 | Near-Miss | Detection stats, event count, current thresholds |
| 6 | WiFi Settings | Network configuration |
| 7 | USB Transfer | USB Mass Storage activation (LEFT port) |
| 8 | Shutdown | Safe shutdown option |

## LED Status Reference

| Color/Pattern | Meaning |
|--------------|---------|
| Blue solid | System initializing |
| Green | Logging active, safe distance |
| Yellow | Medium proximity (10-50cm) |
| Red solid | Critical proximity (<10cm) |
| Orange/Red flash (4Hz) | Near-miss event detected |
| Red blinking (2Hz) | SD card error |

## Project Structure

```
ProxiTof_Arduino_ESP32/
├── main/
│   ├── main.c                          # Main application with sensor loop + detection
│   └── CMakeLists.txt                  # Build config (all 14 components linked)
├── components/                          # ESP-IDF components (14 total)
│   ├── sensor_manager/                  # VL53L8CX ToF + BNO08x IMU hardware abstraction
│   ├── storage_manager/                 # SDMMC high-speed logging with circular buffer
│   ├── data_processor/                  # Near-miss detection algorithm + feature extraction
│   ├── ml_inference/                    # TFLite Micro inference with ESP-NN acceleration
│   ├── smart_time/                      # Multi-tier RTC system (DS3231 + NTP + internal)
│   ├── config_manager/                  # SD card field configuration system
│   ├── usb_interface/                   # USB serial command interface (RIGHT port)
│   ├── usb_flash_partition/             # USB Mass Storage / flash drive (LEFT port)
│   ├── display_manager/                 # Serial console display formatting
│   ├── oled_display/                    # SSD1306 SPI OLED with 9 display modes
│   ├── gps/                             # Neo-M9N GNSS (UART NMEA parser, no ext library)
│   ├── buzzer/                          # PWM audio alerts with potentiometer volume
│   ├── rtc_ds3231/                      # DS3231 I2C driver
│   └── time_sync/                       # WiFi NTP synchronization
├── tools/                               # Python companion tools (16 scripts)
│   ├── proxitof_usb_tool.py            # Main USB serial tool (status, config, time sync)
│   ├── quick_start_ml.py               # One-click ML training pipeline
│   ├── train_model.py                  # TensorFlow model training
│   ├── convert_to_tflite.py            # Model conversion for ESP32
│   ├── load_data.py                    # CSV data loader for training
│   ├── prepare_training_data.py        # Data preprocessing
│   ├── analyze_data.py                 # Data quality analysis
│   ├── check_data_quality.py           # Data validation
│   ├── verify_packages.py             # Python dependency checker
│   ├── usb_flash_manager.py           # USB flash drive manager
│   ├── usb_debug_tool.py              # USB debugging
│   ├── usb_time_sync.py               # Time synchronization
│   ├── send_usb_command.py            # Raw USB command sender
│   ├── convert_logo.py                # OLED logo converter
│   ├── sync_time.bat                  # Windows time sync batch
│   └── usb_flash_quick_start.bat      # Windows USB flash batch
├── docs/                                # Comprehensive documentation (~150 files)
│   ├── README.md                       # Documentation index
│   ├── note-1 through note-12          # Thesis development notes
│   └── ...                             # Guides, references, fix docs
├── data/
│   ├── config.txt                      # Default configuration template
│   └── wifi_config.txt                 # WiFi template
├── requirements_proxitof_thesis.txt     # Python ML dependencies
├── PYTHON_SETUP.md                      # ML environment setup
├── CMakeLists.txt                       # ESP-IDF project config
├── sdkconfig                            # ESP32 build configuration
└── README.md                            # This file
```


## Documentation

### Quick Start & Essential Guides
- [Complete Wiring Diagram](docs/complete-wiring-diagram.md) - All component connections with pin map
- [ESP-IDF Build Guide](docs/esp-idf-build-guide.md) - Building and flashing firmware
- [Field Configuration Guide](docs/field-configuration-guide.md) - SD card configuration system
- [USB Complete Guide](docs/USB_COMPLETE_GUIDE.md) - USB serial + mass storage
- [Quick Reference Card](docs/quick-reference-card.md) - Essential commands and procedures

### Component Guides
- [OLED Complete Guide](docs/OLED_COMPLETE_GUIDE.md) - Display modes and configuration
- [Button Complete Guide](docs/BUTTON_COMPLETE_GUIDE.md) - Button behavior in all modes
- [Smart Time System Guide](docs/smart-time-system-guide.md) - Multi-tier time management
- [GPS-Buzzer Integration Guide](docs/gps-buzzer-integration-guide.md) - GPS and buzzer setup

### Algorithm & ML Documentation
- [Near-Miss Detection Algorithm](docs/near-miss-detection-algorithm.md) - Three-tier algorithm with math formulations
- [Near-Miss Definition and ML Pipeline](docs/near-miss-definition-and-ml-pipeline.md) - Definitions and labeling strategy
- [Why Machine Learning Justification](docs/why-machine-learning-justification.md) - ML approach justification
- [ML Complete Guide](docs/ML_COMPLETE_GUIDE.md) - Full training and deployment pipeline
- [Quick Reference Near-Miss](docs/QUICK-REFERENCE-NEAR-MISS.md) - Detection quick reference

### Thesis Development Notes
| Note | Topic |
|------|-------|
| [note-1](docs/note-1-development-journey.md) | Project development history |
| [note-2](docs/note-2-sensor-manager-implementation.md) | Sensor integration |
| [note-3](docs/note-3-esp-idf-migration-success.md) | ESP-IDF migration |
| [note-4](docs/note-4-data-processor-implementation.md) | Data processing |
| [note-5](docs/note-5-near-miss-algorithm-integration.md) | Algorithm integration |
| [note-6](docs/note-6-gps-buzzer-components.md) | GPS and buzzer components |
| [note-7](docs/note-7-OLED_RESOLUTION_FINAL.md) | OLED resolution |
| [note-8](docs/note-8-ds3231-rtc-time-persistence-fix.md) | RTC time persistence |
| [note-9](docs/note-9-gps-geotagging-implementation.md) | GPS geotagging |
| [note-10](docs/note-10-usb-host-device-mode-fix.md) | USB mode fix |
| [note-11](docs/note-11-ml-training-methodology-and-validation.md) | ML methodology (thesis-quality) |
| [note-12](docs/note-12-detection-mode-cycling.md) | Detection mode cycling |

Full documentation index: [docs/README.md](docs/README.md)

## Troubleshooting

| Problem | Quick Fix | Guide |
|---------|-----------|-------|
| Build errors | Run `export.ps1` first | [ESP-IDF Build Guide](docs/esp-idf-build-guide.md) |
| Time inaccurate | Check DS3231 wiring/battery | [Smart Time Guide](docs/smart-time-system-guide.md) |
| SD card not detected | Format FAT32, check SDMMC wiring | [Storage Guide](docs/storage-integration-guide.md) |
| Sensors not working | Verify I2C/SPI connections | [Wiring Diagram](docs/complete-wiring-diagram.md) |
| GPS no fix | Move outdoors, wait 1-5 min | [GPS Testing Guide](docs/GPS_TESTING_GUIDE.md) |
| OLED blank | Check SPI wiring (7 pins) | [OLED Guide](docs/OLED_COMPLETE_GUIDE.md) |
| USB not detected | Use RIGHT port for serial | [USB Guide](docs/USB_COMPLETE_GUIDE.md) |
| No near-miss detection | Check detection_mode in config | [Near-Miss Algorithm](docs/near-miss-detection-algorithm.md) |

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Acknowledgments

- Espressif Systems - ESP32-S3 and ESP-IDF framework
- STMicroelectronics - VL53L8CX ToF sensor
- Bosch Sensortec - BNO08x IMU sensor
- Maxim Integrated - DS3231 RTC
- u-blox - Neo-M9N GNSS module
- TensorFlow Team - TensorFlow Lite Micro
- Adafruit - Hardware breakout boards
