# ProxiTof Documentation Index

## 📚 Complete System Documentation

Welcome to the ProxiTof Near-Miss Detection System documentation. This index provides access to all guides and references.

---

## 🚀 Quick Start Guides

### Essential Setup
- **[Complete Wiring Diagram](complete-wiring-diagram.md)** - Hardware connections for all components
- **[ESP-IDF Build Guide](esp-idf-build-guide.md)** - Building and flashing firmware
- **[Python Setup Guide](../PYTHON_SETUP.md)** - ML training environment setup

### Time Management
- **[Smart Time System Guide](smart-time-system-guide.md)** - Multi-tier time management overview
- **[DS3231 RTC Setup Guide](ds3231-rtc-setup-guide.md)** - Battery-backed real-time clock
- **[WiFi Time Sync Guide](wifi-time-sync-guide.md)** - Network time synchronization

### Configuration & Access
- **[USB Quick Start Guide](usb-quick-start-guide.md)** - 🚀 **START HERE** for USB commands and flash drive
- **[Field Configuration Guide](field-configuration-guide.md)** - SD card-based configuration
- **[USB Interface Guide](usb-interface-guide.md)** - Computer access via USB
- **[USB Mass Storage Guide](usb-mass-storage-guide.md)** - SD card as USB drive
- **[SD Card Enable Guide](sd-card-enable-guide.md)** - Storage system setup

---

## 🏗️ System Architecture

### Core Components
```
┌─────────────────────────────────────────────────────────┐
│                   ProxiTof System                       │
├─────────────────────────────────────────────────────────┤
│  Sensors: VL53L8CX ToF + BNO08x IMU + Button + LED     │
│  Storage: SD Card (SDMMC 4-bit) + Circular Buffer      │
│  Time: DS3231 RTC + ESP32 RTC + WiFi NTP + USB Sync    │
│  Config: SD Card Files + USB Interface + Smart Defaults│
│  ML: TensorFlow Lite + Training Pipeline + Data Export │
└─────────────────────────────────────────────────────────┘
```

### Data Flow
```
Sensors → Circular Buffer → SD Card → CSV Files → ML Training → TFLite Model → ESP32 Deployment
```

---

## 📖 Detailed Guides

### Hardware & Wiring
| Guide | Description | Status |
|-------|-------------|--------|
| [Complete Wiring Diagram](complete-wiring-diagram.md) | All component connections | ✅ Complete |
| [Hardware Wiring Diagram](hardware-wiring-diagram.md) | Legacy reference | 📚 Archive |
| [VL53L8CX Integration Fixes](vl53l8cx-integration-fixes.md) | ToF sensor troubleshooting | 🔧 Reference |

### Software & Build
| Guide | Description | Status |
|-------|-------------|--------|
| [ESP-IDF Build Guide](esp-idf-build-guide.md) | Build system and commands | ✅ Complete |
| [ESP-IDF Migration](esp-idf-migration.md) | PlatformIO to ESP-IDF transition | 📚 Archive |
| [ESP-IDF Component Structure](esp-idf-component-structure.md) | Code organization | 📚 Reference |
| [Build and Test Guide](build-and-test-guide.md) | Development workflow | 🔧 Reference |

### Time Management
| Guide | Description | Status |
|-------|-------------|--------|
| [Smart Time System Guide](smart-time-system-guide.md) | **Multi-tier time management** | ✅ **Primary** |
| [DS3231 RTC Setup Guide](ds3231-rtc-setup-guide.md) | Battery-backed RTC setup | ✅ Complete |
| [WiFi Time Sync Guide](wifi-time-sync-guide.md) | Network time synchronization | ✅ Complete |

### Configuration & Access
| Guide | Description | Status |
|-------|-------------|--------|
| [Field Configuration Guide](field-configuration-guide.md) | **SD card configuration system** | ✅ **Primary** |
| [USB Interface Guide](usb-interface-guide.md) | **Computer access via USB** | ✅ **Primary** |
| [USB Mass Storage Guide](usb-mass-storage-guide.md) | **SD card as USB drive** | ✅ **Primary** |
| [SD Card Enable Guide](sd-card-enable-guide.md) | Storage system activation | ✅ Complete |

### Data & Storage
| Guide | Description | Status |
|-------|-------------|--------|
| [Storage Integration Guide](storage-integration-guide.md) | Data logging system | ✅ Complete |
| [Data Logging Strategy](data-logging-strategy.md) | CSV format and organization | 📚 Reference |
| [What to Expect on SD Card](what-to-expect-on-sd-card.md) | File structure explanation | 📚 Reference |
| [SD Card SDIO Setup](sd-card-sdio-setup.md) | Hardware interface setup | 🔧 Reference |

### Machine Learning
| Guide | Description | Status |
|-------|-------------|--------|
| [ML Training Complete Guide](ml-training-complete-guide.md) | **Full ML pipeline** | ✅ **Primary** |
| [ML Training Workflow Plan](ml-training-workflow-plan.md) | Training process overview | ✅ Complete |
| [ESP32 TFLite Implementation Guide](esp32-tflite-implementation-guide.md) | Model deployment | ✅ Complete |
| [ML Training Guide](ml-training-guide.md) | Basic training steps | 📚 Reference |

### Development & Testing
| Guide | Description | Status |
|-------|-------------|--------|
| [Testing Guide](testing-guide.md) | System testing procedures | 🔧 Reference |
| [Testing Working Implementation](testing-working-implementation.md) | Validation steps | 📚 Reference |
| [Performance Optimization Plan](performance-optimization-plan.md) | System tuning | 🔧 Reference |

---

## 🎯 Usage Scenarios

### Scenario 1: **Lab Development**
1. **Setup**: [Complete Wiring Diagram](complete-wiring-diagram.md)
2. **Build**: [ESP-IDF Build Guide](esp-idf-build-guide.md)
3. **Configure**: [USB Interface Guide](usb-interface-guide.md)
4. **Time Sync**: [Smart Time System Guide](smart-time-system-guide.md)
5. **Data Collection**: [Storage Integration Guide](storage-integration-guide.md)
6. **ML Training**: [ML Training Complete Guide](ml-training-complete-guide.md)

### Scenario 2: **Field Deployment**
1. **Pre-deployment**: [Field Configuration Guide](field-configuration-guide.md)
2. **Time Setup**: [DS3231 RTC Setup Guide](ds3231-rtc-setup-guide.md)
3. **Configuration**: Edit `/sdcard/config.txt`
4. **Deployment**: Standalone operation
5. **Maintenance**: [USB Interface Guide](usb-interface-guide.md)

### Scenario 3: **Multi-Location Study**
1. **Base Configuration**: [Smart Time System Guide](smart-time-system-guide.md)
2. **Location Setup**: [Field Configuration Guide](field-configuration-guide.md)
3. **Data Management**: [Storage Integration Guide](storage-integration-guide.md)
4. **Remote Access**: [USB Interface Guide](usb-interface-guide.md)

---

## 🔧 Troubleshooting Quick Reference

### Common Issues
| Problem | Solution Guide | Quick Fix |
|---------|----------------|-----------|
| **Build Errors** | [ESP-IDF Build Guide](esp-idf-build-guide.md) | Run `export.ps1` first |
| **Time Issues** | [Smart Time System Guide](smart-time-system-guide.md) | Check DS3231 wiring |
| **SD Card Problems** | [SD Card Enable Guide](sd-card-enable-guide.md) | Format as FAT32 |
| **Sensor Issues** | [Complete Wiring Diagram](complete-wiring-diagram.md) | Check I2C/SPI connections |
| **WiFi Problems** | [Field Configuration Guide](field-configuration-guide.md) | Edit `config.txt` |
| **USB Access** | [USB Interface Guide](usb-interface-guide.md) | Install Python tool |

### Emergency Procedures
- **System Not Responding**: Power cycle, check USB connection
- **Wrong Time**: Use USB interface to sync time
- **Configuration Lost**: Default config auto-created on SD card
- **Data Not Logging**: Check SD card health and format

---

## 📊 System Status Legend

- ✅ **Complete**: Fully documented and tested
- 🔧 **Reference**: Technical reference material
- 📚 **Archive**: Legacy documentation for reference
- ⚠️ **Deprecated**: Outdated, use newer guides

---

## 🆘 Getting Help

### Documentation Issues
- Check the **Quick Start Guides** first
- Use **Troubleshooting Quick Reference** for common problems
- Refer to **System Architecture** for understanding data flow

### Development Support
- **Build Issues**: [ESP-IDF Build Guide](esp-idf-build-guide.md)
- **Hardware Issues**: [Complete Wiring Diagram](complete-wiring-diagram.md)
- **Configuration Issues**: [Field Configuration Guide](field-configuration-guide.md)

### Field Deployment Support
- **Time Management**: [Smart Time System Guide](smart-time-system-guide.md)
- **Remote Configuration**: [USB Interface Guide](usb-interface-guide.md)
- **Data Collection**: [Storage Integration Guide](storage-integration-guide.md)

---

## 📈 System Capabilities Summary

### Hardware
- **Sensors**: 4x4 ToF array (60Hz) + 9-DOF IMU (100Hz)
- **Storage**: High-speed SDMMC (4-bit) with circular buffering
- **Time**: Multi-tier RTC system with automatic fallback
- **Interface**: USB serial + SD card configuration

### Software
- **Real-time**: 200Hz main loop with 2Hz display
- **Data Logging**: CSV format with automatic file rotation
- **Configuration**: Field-configurable without recompilation
- **Time Management**: Automatic sync with graceful degradation

### Machine Learning
- **Training**: TensorFlow 2.20+ with complete pipeline
- **Deployment**: TensorFlow Lite on ESP32-S3
- **Data**: Structured CSV with timestamps and flags
- **Model**: Near-miss detection with real-time inference

**The ProxiTof system is production-ready for research deployment!** 🚀