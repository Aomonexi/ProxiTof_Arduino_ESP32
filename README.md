# ProxiTof Near-Miss Detection System

An ESP32-S3 based multi-sensor proximity detection system with machine learning capabilities, smart time management, and field-configurable operation for near-miss event detection and data logging.

## Overview

The ProxiTof system combines a VL53L8CX Time-of-Flight sensor (4x4 distance grid at 60Hz) with a BNO08x 9-DOF IMU (orientation and motion at 100Hz) to detect and predict near-miss collision events. The system features comprehensive time management, field configuration capabilities, USB interface access, and logs synchronized sensor data to SD card for machine learning training.

## Key Features

### Core Sensing & Logging
- **Multi-sensor fusion**: VL53L8CX ToF (4x4 grid @ 60Hz) + BNO08x IMU (9-DOF @ 100Hz)
- **High-speed data logging**: Synchronized sensor data with circular buffering
- **Manual event flagging**: Button-based annotation during data collection
- **Real-time feedback**: WS2812 RGB LED with distance-based color coding

### Smart Time Management
- **Multi-tier time system**: DS3231 RTC + ESP32 internal RTC + WiFi NTP sync
- **Battery-backed accuracy**: DS3231 maintains time during power cycles
- **Automatic fallback**: Graceful degradation when components unavailable
- **Health monitoring**: Tracks sync status and time source reliability

### Field Configuration
- **SD card configuration**: Edit `/sdcard/config.txt` without recompilation
- **WiFi credentials**: Field-configurable network settings
- **System parameters**: Adjustable logging rates, thresholds, and hardware pins
- **Zero-downtime updates**: Configuration changes applied at next boot

### Computer Interface
- **USB Serial access**: Direct computer connection for configuration
- **Python companion tool**: Automated device detection and management
- **Real-time commands**: Status monitoring, time sync, and configuration
- **Cross-platform support**: Works on Windows, macOS, and Linux

### Machine Learning Pipeline
- **TensorFlow training**: Complete ML pipeline with Python environment
- **TensorFlow Lite deployment**: On-device inference with ESP-NN acceleration
- **Structured data export**: CSV format optimized for ML training
- **Model management**: Easy model updates and performance monitoring

## Hardware Requirements

### Core Components

- **ESP32-S3-DevKitC-1** (or compatible ESP32-S3 board)
- **VL53L8CX** Time-of-Flight sensor (4x4 zone, 8x8 capable)
- **BNO08x** 9-DOF IMU (I2C or SPI interface)
- **MicroSD card** (Class 10+ recommended, FAT32 formatted)
- **Adafruit Micro SD SPI/SDIO Breakout** (Product #4682)
- **DS3231 RTC module** (battery-backed real-time clock)
- **Push button** (for manual event flagging)
- **WS2812 RGB LED** (built-in on most ESP32-S3 dev boards)

### Optional Components

- **Neo-M9N GNSS module** (for location tracking - future enhancement)

### Complete Wiring Diagram

```
ESP32-S3 Pin Assignments (Conflict-Free):

ToF Sensor VL53L8CX (I2C):
  - SDA: GPIO 8
  - SCL: GPIO 9
  - LPN: GPIO 4 (Low Power Enable)
  - VCC: 3.3V
  - GND: GND

IMU BNO08x (SPI):
  - MOSI: GPIO 11
  - MISO: GPIO 13
  - SCK:  GPIO 12
  - CS:   GPIO 10
  - INT:  GPIO 5
  - RST:  GPIO 6
  - VCC:  3.3V
  - GND:  GND

SD Card (SDMMC 4-bit mode):
  - CLK:  GPIO 14
  - CMD:  GPIO 15
  - D0:   GPIO 2
  - D1:   GPIO 16
  - D2:   GPIO 17
  - D3:   GPIO 18
  - VCC:  3.3V
  - GND:  GND

DS3231 RTC (I2C - shared with ToF):
  - SDA: GPIO 8
  - SCL: GPIO 9
  - VCC: 3.3V
  - GND: GND
  - Battery: CR2032 (for backup power)

Button & LED:
  - Button: GPIO 3 (with internal pull-up)
  - LED:    GPIO 48 (built-in WS2812)
```

**Note**: All pin assignments are carefully chosen to avoid conflicts between components. The system uses both I2C and SPI buses efficiently with proper CS pin management.

## Software Requirements

### Development Environment

- **ESP-IDF v5.5+** (Espressif IoT Development Framework)
- **Python 3.13** (for ML training and USB tool)
- **Git** (for version control)

**Important**: This project uses ESP-IDF framework (not Arduino or PlatformIO) for optimal sensor support, ML performance, and advanced features like SDMMC interface.

### ESP-IDF Installation

1. **Install ESP-IDF v5.5+**:
   - Follow the [ESP-IDF Getting Started Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/get-started/)
   - Recommended installation path: `C:\Users\[username]\esp\v5.5.1\esp-idf`

2. **Set up environment** (run before each build session):
   ```powershell
   # Windows PowerShell
   C:\Users\[username]\esp\v5.5.1\esp-idf\export.ps1
   ```

3. **Verify installation**:
   ```bash
   idf.py --version
   ```

### Python Environment Setup

1. **Create virtual environment**:
   ```bash
   python -m venv venv
   venv\Scripts\activate  # Windows
   # source venv/bin/activate  # Linux/macOS
   ```

2. **Install ML training dependencies**:
   ```bash
   pip install -r requirements_proxitof_thesis.txt
   ```

3. **Install USB tool**:
   ```bash
   pip install pyserial
   ```

## Quick Start

### 1. Hardware Setup
Follow the [Complete Wiring Diagram](docs/complete-wiring-diagram.md) to connect all components.

### 2. Build and Flash Firmware

1. **Clone repository**:
   ```bash
   git clone https://github.com/yourusername/ProxiTof_Arduino_ESP32.git
   cd ProxiTof_Arduino_ESP32
   ```

2. **Set up ESP-IDF environment**:
   ```powershell
   # Windows - run this before each build session
   C:\Users\[username]\esp\v5.5.1\esp-idf\export.ps1
   ```

3. **Build project**:
   ```bash
   idf.py build
   ```

4. **Flash to ESP32-S3**:
   ```bash
   idf.py flash monitor
   ```

### 3. Initial Configuration

1. **Format SD card** as FAT32
2. **Insert SD card** - system will create default `config.txt`
3. **Edit configuration** (see [Field Configuration Guide](docs/field-configuration-guide.md)):
   ```
   # WiFi Settings
   wifi_enabled=1
   wifi_ssid=YourWiFiName
   wifi_password=YourWiFiPassword
   
   # Time Management
   timezone=EST5EDT,M3.2.0,M11.1.0
   sync_interval_hours=24
   ```

### 4. USB Interface Setup

1. **Install Python tool**:
   ```bash
   pip install pyserial
   python tools/proxitof_usb_tool.py --status
   ```

2. **Sync time with computer**:
   ```bash
   python tools/proxitof_usb_tool.py --sync-time
   ```

### 5. Start Data Collection

The system automatically begins logging when:
- SD card is inserted and formatted
- Sensors initialize successfully
- Time is synchronized

**LED Status**: Green breathing = active logging

## System Configuration

### SD Card Configuration

The system uses `/sdcard/config.txt` for field-configurable parameters:

```ini
# WiFi Configuration
wifi_enabled=1
wifi_ssid=YourNetworkName
wifi_password=YourNetworkPassword

# Time Management
timezone=EST5EDT,M3.2.0,M11.1.0
sync_interval_hours=24
ds3231_enabled=1
ds3231_sda_pin=8
ds3231_scl_pin=9

# Data Logging
log_interval_ms=16          # ~60Hz logging rate
flag_window_ms=2000         # Time window around flagged events
buffer_size=1000            # Circular buffer size

# ML Parameters
ml_threshold=0.70           # ML confidence threshold (0.0-1.0)
approach_threshold_mm_s=500 # Approach rate for near-miss (mm/s)
jerk_threshold_m_s3=2.0     # Jerk magnitude for near-miss (m/s³)

# Hardware Pins (advanced users only)
tof_sda_pin=8
tof_scl_pin=9
tof_lpn_pin=4
# ... (see docs/field-configuration-guide.md for complete list)
```

### USB Interface Commands

Connect via USB and use serial terminal (115200 baud) or Python tool:

```bash
# Show system status
python tools/proxitof_usb_tool.py --status

# Set WiFi credentials
python tools/proxitof_usb_tool.py --wifi "MyWiFi" "MyPassword"

# Sync time with computer
python tools/proxitof_usb_tool.py --sync-time

# Interactive mode
python tools/proxitof_usb_tool.py --interactive
```

### Time Management

The system uses a multi-tier time management approach:

1. **DS3231 RTC** (Primary): Battery-backed, high accuracy
2. **ESP32 Internal RTC** (Secondary): Maintains time during operation
3. **WiFi NTP Sync** (Tertiary): Periodic network synchronization
4. **USB Time Sync** (Manual): Computer-based time setting

**Automatic Fallback**: System gracefully degrades when components are unavailable.

## Usage

### Data Collection Mode

1. **Power on** with SD card inserted and configured
2. **LED indicators**:
   - Blue solid: Initializing
   - Green breathing: Active logging
   - Yellow flash: Button pressed (event flagged)
   - Red: Critical proximity alert
3. **Manual flagging**: Press button during near-miss events
4. **Automatic logging**: System continuously logs at 60Hz
5. **Data storage**: Files saved to `/YYYY-MM-DD/log_YYYYMMDD_HHMMSS.csv`

### LED Status Reference

| Color/Pattern | Meaning | Action Required |
|--------------|---------|-----------------|
| Blue solid | System initializing | Wait for completion |
| Green breathing | Logging active | Normal operation |
| Yellow flash (500ms) | Button pressed | Event flagged successfully |
| Orange alert (1s) | ML near-miss detected | Review data |
| Red solid | Critical proximity (<10cm) | Check sensor placement |
| Red blinking (2Hz) | SD card error | Check card/wiring |
| Purple tint | Buffer warning (>90%) | Check SD card speed |

### Computer Interface

#### USB Serial Commands (115200 baud)
```
help                        # Show available commands
status                      # Display system status
config                      # Show current configuration
config wifi SSID PASSWORD  # Set WiFi credentials
time                        # Show current time
time YYYY-MM-DD HH:MM:SS   # Set time manually
sync                        # Sync time via NTP
files                       # List recent log files
```

#### Python Tool Usage
```bash
# Quick status check
python tools/proxitof_usb_tool.py --status

# Configure WiFi
python tools/proxitof_usb_tool.py --wifi "MyNetwork" "MyPassword"

# Set timezone
python tools/proxitof_usb_tool.py --timezone "EST5EDT,M3.2.0,M11.1.0"

# Sync time with computer
python tools/proxitof_usb_tool.py --sync-time

# Interactive terminal
python tools/proxitof_usb_tool.py --interactive
```

## Data Format & Machine Learning

### CSV Data Structure

Log files contain synchronized sensor data with precise timestamps:

```csv
timestamp_ms,tof_00,tof_01,...,tof_33,roll,pitch,yaw,accel_x,accel_y,accel_z,
gyro_x,gyro_y,gyro_z,min_distance_mm,approach_rate_mm_s,jerk_magnitude_m_s3,
angular_velocity_deg_s,flag_manual,flag_auto,confidence
```

**Data Columns**:
- `timestamp_ms`: Milliseconds since boot (synchronized across sensors)
- `tof_00` to `tof_33`: 16 distance values in mm (4x4 grid, row-major)
- `roll`, `pitch`, `yaw`: Orientation in degrees
- `accel_x/y/z`: Linear acceleration in m/s²
- `gyro_x/y/z`: Angular velocity in deg/s
- `min_distance_mm`: Minimum distance across ToF grid
- `approach_rate_mm_s`: Rate of distance change (derivative)
- `jerk_magnitude_m_s3`: Rate of acceleration change
- `angular_velocity_deg_s`: Magnitude of rotation
- `flag_manual`: Manual button flag (0 or 1)
- `flag_auto`: ML detection flag (0 or 1)
- `confidence`: ML confidence score (0.00 to 1.00)

### Machine Learning Pipeline

#### 1. Training Environment Setup
```bash
# Create Python environment
python -m venv venv
venv\Scripts\activate

# Install ML dependencies
pip install -r requirements_proxitof_thesis.txt
```

#### 2. Data Collection & Labeling
- Collect training data using manual button flags
- System automatically extracts features from raw sensor data
- Time-synchronized data ensures accurate event correlation

#### 3. Model Training
```python
# See docs/ml-training-complete-guide.md for full pipeline
import tensorflow as tf
from proxitof_ml import load_data, train_model, convert_to_tflite

# Load and preprocess data
X, y = load_data('path/to/csv/files')

# Train TensorFlow model
model = train_model(X, y)

# Convert to TensorFlow Lite for ESP32
tflite_model = convert_to_tflite(model)
```

#### 4. Model Deployment
- Convert trained model to TensorFlow Lite format
- Quantize for ESP32-S3 performance
- Flash model with firmware for real-time inference
- Monitor inference performance via USB interface

**Complete ML documentation**: See [ML Training Complete Guide](docs/ml-training-complete-guide.md)

## Documentation

### 📚 Complete Documentation System

This project includes comprehensive documentation for all aspects of development and deployment:

#### Quick Start Guides
- **[Complete Wiring Diagram](docs/complete-wiring-diagram.md)** - Hardware setup and connections
- **[ESP-IDF Build Guide](docs/esp-idf-build-guide.md)** - Building and flashing firmware
- **[Field Configuration Guide](docs/field-configuration-guide.md)** - SD card configuration system

#### System Guides
- **[Smart Time System Guide](docs/smart-time-system-guide.md)** - Multi-tier time management
- **[USB Interface Guide](docs/usb-interface-guide.md)** - Computer access and control
- **[Storage Integration Guide](docs/storage-integration-guide.md)** - Data logging and SD card setup

#### Development Guides
- **[ML Training Complete Guide](docs/ml-training-complete-guide.md)** - Full machine learning pipeline
- **[Python Setup Guide](PYTHON_SETUP.md)** - ML environment configuration
- **[ESP32 TFLite Implementation Guide](docs/esp32-tflite-implementation-guide.md)** - Model deployment

#### Reference Documentation
- **[Documentation Index](docs/README.md)** - Complete guide index with usage scenarios
- **[System Overview](docs/system-overview.md)** - Architecture and component interaction
- **[Quick Reference Card](docs/quick-reference-card.md)** - Essential commands and procedures

### 🎯 Usage Scenarios

**Lab Development**: Hardware setup → Build firmware → Configure via USB → Collect data → Train ML models

**Field Deployment**: Pre-configure SD card → Deploy device → Monitor via USB → Collect data remotely

**Multi-Site Studies**: Standardize configuration → Deploy multiple units → Centralized data collection

## Troubleshooting

### Common Issues & Solutions

| Problem | Quick Fix | Detailed Guide |
|---------|-----------|----------------|
| **Build errors** | Run `export.ps1` first | [ESP-IDF Build Guide](docs/esp-idf-build-guide.md) |
| **Time inaccurate** | Check DS3231 wiring/battery | [Smart Time System Guide](docs/smart-time-system-guide.md) |
| **SD card not detected** | Format as FAT32, check wiring | [Storage Integration Guide](docs/storage-integration-guide.md) |
| **Sensors not working** | Verify I2C/SPI connections | [Complete Wiring Diagram](docs/complete-wiring-diagram.md) |
| **WiFi connection fails** | Edit `/sdcard/config.txt` | [Field Configuration Guide](docs/field-configuration-guide.md) |
| **USB not detected** | Install Python tool, check cable | [USB Interface Guide](docs/usb-interface-guide.md) |

### Emergency Procedures

**System Not Responding**:
1. Power cycle the device
2. Check USB connection
3. Use Python tool: `python tools/proxitof_usb_tool.py --status`

**Wrong Time**:
1. USB sync: `python tools/proxitof_usb_tool.py --sync-time`
2. Manual set via USB interface
3. Check DS3231 battery

**Configuration Lost**:
1. System auto-creates default `config.txt` on SD card
2. Edit configuration via USB interface
3. Backup working configurations

**Data Not Logging**:
1. Check SD card health and format
2. Verify sufficient free space
3. Check buffer status via USB interface

### Performance Optimization

- **Faster logging**: Use Class 10+ SD card with SDMMC interface
- **Better accuracy**: Ensure DS3231 has fresh CR2032 battery
- **Stable operation**: Use quality 3.3V power supply
- **Reliable WiFi**: Place device within good signal range

## Project Structure

```
ProxiTof_Arduino_ESP32/
├── main/                           # Main application
│   ├── main.c                     # Core application logic
│   └── CMakeLists.txt             # Build configuration
├── components/                     # ESP-IDF components
│   ├── sensor_manager/            # Multi-sensor interface
│   ├── storage_manager/           # SD card data logging
│   ├── smart_time/                # Multi-tier time management
│   ├── config_manager/            # Field configuration system
│   ├── usb_interface/             # Computer interface
│   ├── rtc_ds3231/               # Battery-backed RTC
│   └── time_sync/                # Network time synchronization
├── tools/                         # Development tools
│   └── proxitof_usb_tool.py      # Python companion tool
├── docs/                          # Comprehensive documentation
│   ├── README.md                  # Documentation index
│   ├── complete-wiring-diagram.md # Hardware setup
│   ├── esp-idf-build-guide.md    # Build instructions
│   ├── smart-time-system-guide.md # Time management
│   ├── field-configuration-guide.md # Configuration system
│   ├── usb-interface-guide.md     # Computer interface
│   └── ml-training-complete-guide.md # ML pipeline
├── data/                          # Configuration templates
│   ├── config.txt                # Default configuration
│   └── wifi_config.txt           # WiFi template
├── requirements_proxitof_thesis.txt # Python ML dependencies
├── PYTHON_SETUP.md               # ML environment setup
├── CMakeLists.txt                # ESP-IDF project config
├── sdkconfig.defaults            # ESP32 configuration
└── README.md                     # This file
```

### Code Architecture

The system follows a modular ESP-IDF component architecture:

- **Sensor Manager**: Hardware abstraction for ToF, IMU, button, LED
- **Storage Manager**: High-speed SDMMC logging with circular buffering
- **Smart Time**: Multi-tier RTC system with automatic fallback
- **Config Manager**: SD card-based field configuration
- **USB Interface**: Serial command interface for computer access
- **Data Processor**: Feature extraction and ML inference (future)

### Development Workflow

1. **Hardware Setup**: Follow wiring diagram and component guides
2. **Firmware Development**: Use ESP-IDF with component-based architecture
3. **Configuration**: Field-configurable via SD card and USB interface
4. **Data Collection**: High-speed logging with real-time feedback
5. **ML Training**: Python pipeline with TensorFlow and structured data
6. **Deployment**: TensorFlow Lite models with ESP-NN acceleration

## Contributing

### Development Guidelines

1. **Fork the repository** and create a feature branch
2. **Follow ESP-IDF coding standards** and component architecture
3. **Test on hardware** before submitting changes
4. **Update documentation** for new features
5. **Submit pull request** with clear description

### Coding Standards

- Use ESP-IDF component structure for new features
- Follow C99 standard with clear variable naming
- Add comprehensive error handling and logging
- Include unit tests where applicable
- Document public APIs and complex algorithms

### Testing Checklist

- [ ] Hardware functionality verified
- [ ] SD card logging works correctly
- [ ] USB interface responds properly
- [ ] Time synchronization functions
- [ ] Configuration system updates
- [ ] Documentation updated

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Acknowledgments

- **Espressif Systems** for ESP32-S3 and ESP-IDF framework
- **STMicroelectronics** for VL53L8CX ToF sensor
- **Bosch Sensortec** for BNO08x IMU sensor
- **Maxim Integrated** for DS3231 RTC
- **TensorFlow Team** for TensorFlow Lite Micro
- **Adafruit** for hardware breakout boards and libraries

## Support & Contact

### Getting Help

1. **Check Documentation**: Start with [docs/README.md](docs/README.md) for comprehensive guides
2. **Hardware Issues**: Refer to [Complete Wiring Diagram](docs/complete-wiring-diagram.md)
3. **Software Issues**: See [ESP-IDF Build Guide](docs/esp-idf-build-guide.md)
4. **Configuration Help**: Use [Field Configuration Guide](docs/field-configuration-guide.md)

### Reporting Issues

- **Hardware Problems**: Include wiring photos and serial output
- **Software Bugs**: Provide build logs and error messages
- **Feature Requests**: Describe use case and expected behavior
- **Documentation Issues**: Specify which guide needs improvement

### Community

- Open issues on GitHub for bugs and feature requests
- Check existing documentation before asking questions
- Share your deployment experiences and improvements
- Contribute to documentation and code improvements

---

**The ProxiTof system is production-ready for research deployment with comprehensive documentation, field configuration capabilities, and robust time management!** 🚀
