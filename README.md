# ProxiTof Near-Miss Detection System

An ESP32-S3 based multi-sensor proximity detection system with machine learning capabilities for near-miss event detection and data logging.

## Overview

The ProxiTof system combines a VL53L8CX Time-of-Flight sensor (4x4 distance grid at 60Hz) with a BNO08x 9-DOF IMU (orientation and motion at 100Hz) to detect and predict near-miss collision events. The system logs synchronized sensor data to an SD card and can deploy trained TensorFlow Lite models for real-time near-miss prediction.

## Features

- **Multi-sensor fusion**: ToF distance grid + IMU orientation/acceleration
- **High-speed data logging**: 60Hz synchronized sensor data to SD card
- **Manual event flagging**: Button-based annotation during data collection
- **ML inference**: On-device TensorFlow Lite model execution with ESP-NN acceleration
- **Visual feedback**: WS2812 RGB LED status indication
- **Configurable parameters**: SD card-based configuration file
- **Optional GNSS**: Location tracking support (future enhancement)

## Hardware Requirements

### Core Components

- **ESP32-S3-DevKitC-1** (or compatible ESP32-S3 board)
- **VL53L8CX** Time-of-Flight sensor (4x4 zone, 8x8 capable)
- **BNO08x** 9-DOF IMU (I2C or SPI)
- **MicroSD card module** (SPI interface, FAT32 formatted)
- **Push button** (for manual event flagging)
- **WS2812 RGB LED** (built-in on most ESP32-S3 dev boards)

### Optional Components

- **Neo-M9N GNSS module** (for location tracking)

### Pin Configuration

```
ESP32-S3 Pin Assignments:

ToF Sensor (I2C):
  - SDA: GPIO 8
  - SCL: GPIO 9
  - LPN: GPIO 4 (Low Power Enable)

IMU Sensor (SPI):
  - MOSI: GPIO 11
  - MISO: GPIO 13
  - SCK:  GPIO 12
  - CS:   GPIO 10
  - INT:  GPIO 5
  - RST:  GPIO 6

SD Card (SPI - shared bus with IMU):
  - MOSI: GPIO 11
  - MISO: GPIO 13
  - SCK:  GPIO 12
  - CS:   GPIO 7

Button:
  - Input: GPIO 3 (with internal pull-up)

Status LED:
  - WS2812: GPIO 48 (built-in on ESP32-S3-DevKitC-1)
```

## Software Requirements

### Development Environment

- **PlatformIO** (recommended) or Arduino IDE
- **Python 3.7+** (for data analysis and ML training)
- **Git** (for version control)

### PlatformIO Installation

1. Install [Visual Studio Code](https://code.visualstudio.com/)
2. Install the [PlatformIO IDE extension](https://platformio.org/install/ide?install=vscode)
3. Restart VS Code

Alternatively, install PlatformIO Core CLI:
```bash
pip install platformio
```

## Building and Flashing

### Using PlatformIO (Recommended)

1. **Clone the repository**:
   ```bash
   git clone https://github.com/yourusername/ProxiTof_Arduino_ESP32.git
   cd ProxiTof_Arduino_ESP32
   ```

2. **Build the project**:
   ```bash
   pio run
   ```

3. **Flash to ESP32-S3**:
   ```bash
   pio run -t upload
   ```

4. **Monitor serial output**:
   ```bash
   pio device monitor
   ```
   Serial baud rate: 115200

### Using Arduino IDE

1. Install Arduino IDE 2.0+
2. Add ESP32 board support:
   - Go to File → Preferences
   - Add to "Additional Board Manager URLs": 
     ```
     https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
     ```
   - Go to Tools → Board → Boards Manager
   - Search for "esp32" and install "esp32 by Espressif Systems"

3. Install required libraries via Library Manager:
   - Adafruit VL53L8CX
   - SparkFun BNO08x Cortex Based IMU
   - Adafruit NeoPixel
   - TensorFlow Lite Micro

4. Select board: Tools → Board → ESP32 Arduino → ESP32S3 Dev Module
5. Configure settings:
   - USB CDC On Boot: Enabled
   - PSRAM: Enabled
   - Upload Speed: 921600

6. Open `src/main.cpp` and upload

## SD Card Setup

1. **Format SD card** as FAT32
2. **Copy configuration file**:
   - Copy `data/config.txt` to the root of the SD card
3. **Insert SD card** into the module before powering on

### Configuration Parameters

Edit `config.txt` on the SD card to adjust system behavior:

```
log_interval_ms=16          # Logging rate (~60Hz)
flag_window_ms=2000         # Time window around flagged events (ms)
ml_threshold=0.70           # ML confidence threshold (0.0-1.0)
approach_threshold_mm_s=500 # Approach rate for near-miss (mm/s)
jerk_threshold_m_s3=2.0     # Jerk magnitude for near-miss (m/s³)
```

## Usage

### Data Collection Mode

1. Power on the system with SD card inserted
2. Green breathing LED indicates active logging
3. Press the flag button during near-miss events
4. Yellow LED flash confirms button press
5. Data is saved to `/YYYY-MM-DD/log_YYYYMMDD_HHMMSS.csv`

### LED Status Indicators

| Color/Pattern | Meaning |
|--------------|---------|
| Blue solid | Initializing |
| Green breathing | Logging active |
| Yellow flash (500ms) | Button pressed |
| Orange alert (1s) | ML near-miss detected |
| Red solid | Critical proximity alert |
| Red blinking (2Hz) | SD card error |
| Purple tint | Buffer warning (>90% full) |

### Serial Commands

Connect via serial monitor (115200 baud) and send:

- `config` - Display current configuration
- `status` - Display system state and metrics
- `errors` - Display error counters

## Data Format

CSV files contain synchronized sensor data with the following columns:

```
timestamp_ms, tof_00, tof_01, ..., tof_33, roll, pitch, yaw,
accel_x, accel_y, accel_z, gyro_x, gyro_y, gyro_z,
min_distance_mm, approach_rate_mm_s, jerk_magnitude_m_s3, angular_velocity_deg_s,
flag_manual, flag_auto, confidence
```

- **timestamp_ms**: Milliseconds since boot
- **tof_00 to tof_33**: 16 distance values in mm (4x4 grid, row-major order)
- **roll, pitch, yaw**: Orientation in degrees
- **accel_x, accel_y, accel_z**: Linear acceleration in m/s²
- **gyro_x, gyro_y, gyro_z**: Angular velocity in deg/s
- **min_distance_mm**: Minimum distance across ToF grid
- **approach_rate_mm_s**: Rate of distance change
- **jerk_magnitude_m_s3**: Rate of acceleration change
- **angular_velocity_deg_s**: Magnitude of rotation
- **flag_manual**: Manual button flag (0 or 1)
- **flag_auto**: ML detection flag (0 or 1)
- **confidence**: ML confidence score (0.00 to 1.00)

## Machine Learning

### Training a Model

1. **Collect training data**: Run system in data collection mode
2. **Label events**: Use manual flags to mark near-miss events
3. **Preprocess data**: Use provided Python scripts in `docs/ml_training/`
4. **Train model**: TensorFlow/Keras model with 25 input features
5. **Convert to TFLite**: Quantize and optimize for ESP32-S3
6. **Flash model**: Include model in firmware build

See `docs/ml_training/README.md` for detailed instructions (coming soon).

## Troubleshooting

### SD Card Not Detected

- Verify SD card is FAT32 formatted
- Check wiring connections (CS pin especially)
- Try a different SD card (some cards are incompatible)
- Check serial output for specific error codes

### Sensor Initialization Failed

- Verify I2C/SPI connections
- Check power supply (sensors need stable 3.3V)
- Ensure correct pin configuration in code
- Check serial output for I2C/SPI error codes

### Frame Rate Drops

- Reduce logging frequency in config.txt
- Use faster SD card (Class 10 or UHS-I)
- Disable ML inference if not needed
- Check buffer utilization in serial output

### ML Inference Slow

- Verify ESP-NN is enabled in build
- Check model size (should be <100KB)
- Reduce model complexity if needed
- Monitor inference latency in serial output

## Project Structure

```
ProxiTof_Arduino_ESP32/
├── src/                    # Source code
│   └── main.cpp           # Main application
├── include/               # Header files
├── lib/                   # Custom libraries
├── data/                  # Data files (config, models)
│   └── config.txt        # Configuration template
├── docs/                  # Documentation
├── platformio.ini        # PlatformIO configuration
├── README.md             # This file
└── .gitignore           # Git ignore rules
```

## Development

### Code Organization

The codebase follows a modular architecture:

- **Sensor Manager**: Hardware abstraction for ToF, IMU, button
- **Data Processor**: Synchronization and feature extraction
- **Storage Manager**: SD card operations and buffering
- **ML Inference**: TensorFlow Lite model execution
- **Status LED**: Visual feedback system

### Contributing

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit changes (`git commit -m 'Add amazing feature'`)
4. Push to branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

### Coding Standards

- Follow Arduino/C++ style guidelines
- Use descriptive variable and function names
- Comment complex algorithms
- Keep functions focused and modular
- Test on hardware before committing

## License

[Specify your license here]

## Acknowledgments

- Adafruit for VL53L8CX and NeoPixel libraries
- SparkFun for BNO08x library
- TensorFlow team for TFLite Micro
- Espressif for ESP32-S3 and ESP-NN

## Support

For issues, questions, or contributions:
- Open an issue on GitHub
- Check existing documentation in `docs/`
- Review serial output for diagnostic information

## Version History

- **v0.1.0** - Initial project setup and structure
