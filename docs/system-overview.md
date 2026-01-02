# ProxiTof System Overview

## 🎯 Project Summary

**ProxiTof** is a near-miss detection system for research applications, combining high-frequency sensor data collection with machine learning inference on embedded hardware.

### Key Features
- **Real-time sensor fusion**: ToF distance array + 9-DOF IMU
- **High-speed data logging**: SDMMC interface with circular buffering
- **Smart time management**: Multi-tier RTC system with automatic fallback
- **Field-configurable**: USB interface + SD card configuration
- **ML-ready**: Complete TensorFlow training pipeline + ESP32 deployment

---

## 🏗️ System Architecture

### Hardware Components
```
ESP32-S3 DevKitC-1 (16MB Flash, 8MB PSRAM)
├── VL53L8CX ToF Sensor (I2C) - 4x4 distance grid @ 60Hz
├── BNO08x IMU Sensor (SPI) - 9-DOF motion @ 100Hz  
├── DS3231 RTC Module (I2C) - Battery-backed time keeping
├── SD Card Module (SDMMC) - High-speed data storage
├── Push Button (GPIO) - Manual event flagging
└── WS2812 RGB LED (GPIO) - Status indication
```

### Software Architecture
```
Application Layer
├── Sensor Manager - Multi-sensor data fusion
├── Storage Manager - Circular buffer + SD logging
├── Smart Time System - Multi-tier time management
├── Config Manager - Field configuration system
├── USB Interface - Computer communication
└── Data Processor - ML inference (future)

Hardware Abstraction Layer
├── ESP-IDF Framework v5.5+
├── Component Libraries (ToF, IMU, RTC)
├── USB Serial JTAG Driver
└── SDMMC Host Driver
```

---

## 📊 Performance Specifications

### Data Collection
| Metric | Specification | Notes |
|--------|---------------|-------|
| **ToF Sample Rate** | 60 Hz | 4x4 distance grid |
| **IMU Sample Rate** | 100 Hz | 9-DOF (accel + gyro + orientation) |
| **Main Loop Rate** | 200 Hz | Real-time LED response |
| **Display Update** | 2 Hz | Serial monitor output |
| **Data Logging** | Variable | Buffered writes every 500ms |

### Storage Performance
| Metric | Specification | Notes |
|--------|---------------|-------|
| **Interface** | SDMMC 4-bit | 4x faster than SPI |
| **Write Speed** | Up to 40 MB/s | Class 10 SD card |
| **Buffer Size** | 1000 entries | Circular buffer |
| **File Format** | CSV | ML-ready format |
| **File Rotation** | 100 MB | Automatic new files |

### Time Accuracy
| Source | Accuracy | Backup Duration |
|--------|----------|-----------------|
| **DS3231 RTC** | ±2 ppm | Years (battery) |
| **NTP Sync** | ±50 ms | N/A (network) |
| **ESP32 RTC** | ±40 ppm | Power-on only |
| **USB Sync** | Computer accuracy | Instant |

---

## 🔄 Data Flow

### Sensor → Storage Pipeline
```
1. Sensor Reading (60-100 Hz)
   ├── VL53L8CX: 4x4 distance array
   └── BNO08x: Roll, pitch, yaw, accel, gyro

2. Data Fusion & Buffering
   ├── Timestamp assignment
   ├── Button state capture
   └── Circular buffer insertion

3. Storage Management
   ├── Buffer threshold check (80%)
   ├── Batch write to SD card
   └── File rotation (100MB limit)

4. File Organization
   ├── Date-based folders: /YYYY-MM-DD/
   ├── Timestamped files: log_YYYYMMDD_HHMMSS.csv
   └── CSV format: timestamp,tof_00-33,roll,pitch,yaw,accel_xyz,gyro_xyz,flag
```

### Configuration Flow
```
1. Boot Sequence
   ├── Load config from /sdcard/config.txt
   ├── Apply hardware settings (pins, intervals)
   └── Initialize time management

2. Runtime Updates
   ├── USB interface commands
   ├── Configuration file monitoring
   └── Automatic system adaptation

3. Field Deployment
   ├── SD card configuration editing
   ├── USB-based time synchronization
   └── Status monitoring via USB
```

---

## ⚙️ Configuration System

### Multi-Level Configuration
```
Priority 1: SD Card config.txt (Field editable)
Priority 2: USB Interface commands (Real-time)
Priority 3: Compiled defaults (Fallback)
```

### Configuration Parameters
| Category | Parameters | Field Editable |
|----------|------------|----------------|
| **WiFi** | SSID, Password, Enable/Disable | ✅ Yes |
| **Time** | Timezone, Sync interval | ✅ Yes |
| **Hardware** | DS3231 pins, Enable/Disable | ✅ Yes |
| **Logging** | Buffer size, File rotation | 🔧 Code only |
| **Sensors** | Sample rates, Calibration | 🔧 Code only |

### Configuration Access Methods
1. **SD Card Editing**: Remove card, edit config.txt, reinsert
2. **USB Interface**: Serial commands via computer
3. **Python Tool**: Automated configuration management
4. **Code Modification**: Recompile for advanced changes

---

## 🕐 Time Management System

### Multi-Tier Architecture
```
Primary: DS3231 RTC (Battery-backed, ±2ppm accuracy)
    ↓ (if DS3231 fails)
Secondary: ESP32 Internal RTC (Volatile, ±40ppm accuracy)
    ↓ (if time invalid)
Fallback: Default timestamps (Development only)

Sync Sources:
├── WiFi + NTP (Automatic, configurable interval)
├── USB Interface (Manual, computer time)
└── Manual Setting (Emergency, via commands)
```

### Automatic Behaviors
- **Health Monitoring**: Detects DS3231 disconnection/failure
- **Auto-Recovery**: Switches back when DS3231 reconnects
- **Scheduled Sync**: NTP sync every 24 hours (configurable)
- **Graceful Degradation**: System continues with available sources

---

## 🔌 Interface Systems

### USB Interface
```
Physical: USB-C connector (ESP32-S3 built-in)
Protocols: 
├── USB Serial JTAG (Programming + Commands)
└── USB Mass Storage Class (SD card as USB drive)

Features:
├── Real-time configuration via serial commands
├── SD card access as USB drive (MSC)
├── Automatic switching between logging and USB access
├── Time synchronization from computer
├── System status monitoring
├── Command-line interface
├── Python tool automation
└── Dual USB functionality (Serial + MSC)

USB MSC Commands:
├── msc start (Enable USB drive mode)
├── msc stop (Resume data logging)
└── msc status (Check current mode)
```

### SD Card Interface
```
Physical: SDMMC 4-bit interface
Pins: CLK=14, CMD=15, D0=2, D1=16, D2=17, D3=18
Speed: Up to 40 MHz (Class 10 cards)
Features:
├── High-speed data logging
├── Configuration file storage
├── Automatic file organization
└── Health monitoring
```

---

## 🤖 Machine Learning Pipeline

### Training Pipeline (Computer)
```
1. Data Collection
   ├── CSV files from SD card
   ├── Timestamp-aligned sensor data
   └── Manual event flags

2. Data Preprocessing
   ├── Feature extraction
   ├── Normalization
   └── Sequence windowing

3. Model Training
   ├── TensorFlow 2.20+
   ├── Neural network architecture
   └── Validation & testing

4. Model Conversion
   ├── TensorFlow Lite conversion
   ├── Quantization optimization
   └── ESP32 compatibility check
```

### Inference Pipeline (ESP32)
```
1. Real-time Data
   ├── Sensor fusion
   ├── Feature extraction
   └── Sliding window buffer

2. ML Inference
   ├── TensorFlow Lite Micro
   ├── Quantized model execution
   └── Confidence scoring

3. Decision Making
   ├── Threshold comparison
   ├── Alert generation
   └── Action triggering
```

---

## 🛡️ Reliability Features

### Hardware Reliability
- **Redundant time sources**: DS3231 + ESP32 RTC + NTP + USB
- **Conflict-free pin assignment**: No shared resources
- **Health monitoring**: Automatic failure detection
- **Graceful degradation**: System continues with reduced functionality

### Software Reliability
- **Circular buffering**: Prevents data loss during SD writes
- **Automatic recovery**: Reconnection handling for all components
- **Configuration validation**: Invalid settings use safe defaults
- **Error logging**: Comprehensive diagnostic information

### Data Reliability
- **Atomic writes**: Complete records or nothing
- **File rotation**: Prevents single large files
- **Timestamp validation**: Ensures chronological ordering
- **Checksum protection**: Future enhancement for data integrity

---

## 📈 Deployment Scenarios

### Lab Development
```
Configuration:
├── WiFi enabled (frequent NTP sync)
├── DS3231 enabled (backup time)
├── USB interface active (development)
└── High logging verbosity

Workflow:
1. Hardware setup and testing
2. Software development and debugging
3. Data collection and validation
4. ML model training and testing
```

### Field Deployment
```
Configuration:
├── WiFi optional (location-dependent)
├── DS3231 required (primary time)
├── USB interface for maintenance
└── Optimized logging

Workflow:
1. Pre-deployment configuration
2. Field installation
3. Autonomous data collection
4. Periodic maintenance via USB
```

### Multi-Site Study
```
Configuration:
├── Site-specific config files
├── Standardized hardware setup
├── Centralized data processing
└── Remote monitoring capability

Workflow:
1. Batch device preparation
2. Site-specific configuration
3. Coordinated deployment
4. Data aggregation and analysis
```

---

## 🔮 Future Enhancements

### Planned Features
- **USB Mass Storage**: Direct SD card access via USB
- **Web Interface**: Browser-based configuration
- **Wireless Communication**: WiFi/Bluetooth data transfer
- **Advanced ML**: On-device training and adaptation
- **Power Management**: Battery operation optimization
- **Remote Monitoring**: Cloud connectivity and dashboards

### Research Applications
- **Transportation Safety**: Near-miss detection in traffic
- **Industrial Safety**: Workplace hazard monitoring
- **Sports Analytics**: Performance and safety analysis
- **Robotics**: Collision avoidance systems
- **IoT Integration**: Smart city and building applications

---

## 📋 System Requirements

### Development Environment
- **ESP-IDF**: v5.5+ (required)
- **Python**: 3.13+ (ML training)
- **Computer**: Windows/Linux/Mac
- **Storage**: 2GB+ for development tools
- **SD Card**: Class 10, FAT32, 8GB+

### Hardware Requirements
- **ESP32-S3**: DevKitC-1 (16MB Flash, 8MB PSRAM)
- **Power**: USB-C or external 5V/1A supply
- **Sensors**: VL53L8CX + BNO08x + DS3231
- **Storage**: MicroSD card + SDIO breakout
- **Accessories**: Breadboard, jumper wires, resistors

### Field Deployment
- **Power**: Stable 5V supply or battery pack
- **Environment**: Indoor/outdoor (IP rating dependent on enclosure)
- **Temperature**: -10°C to +60°C (component dependent)
- **Connectivity**: Optional WiFi for time sync
- **Maintenance**: USB access for configuration

---

**The ProxiTof system provides a complete, production-ready platform for near-miss detection research with professional-grade reliability and ease of use.** 🚀