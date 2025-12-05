# ProxiTof - Multi-Sensor Proximity Detection System

**ESP32-S3 based sensor fusion system combining Time-of-Flight distance sensing with 9-DOF Inertial Measurement for enhanced spatial awareness and near-miss detection.**

## System Overview

### Hardware Components
- **MCU**: ESP32-S3-WROOM-1 (Dual-core Xtensa LX7 @ 240MHz)
- **ToF Sensor**: VL53L8CX (I2C) - 4x4 multizone ranging @ 60Hz
- **IMU**: BNO08x (SPI) - 9-DOF with onboard sensor fusion
- **Indicator**: WS2812 RGB LED for proximity feedback
- **Communication**: I2C (ToF) + SPI (IMU) on separate buses

### Key Features
- ✅ Real-time 4x4 distance grid @ 60Hz
- ✅ Orientation tracking (Roll, Pitch, Yaw)
- ✅ Linear acceleration monitoring
- ✅ Visual proximity alerts (RGB LED)
- ✅ Sensor fusion ready for ESP-NN integration
- ✅ Non-blocking multi-sensor operation

---

## Pin Configuration

### ESP32-S3 Pin Assignments

```
┌─────────────────────────────────────────────┐
│ VL53L8CX ToF Sensor (I2C)                   │
├─────────────────────────────────────────────┤
│ GPIO8  →  SDA                               │
│ GPIO9  →  SCL                               │
│ GPIO4  →  LPN (Low Power/Reset)             │
│ 3.3V   →  VDD                               │
│ GND    →  GND                               │
├─────────────────────────────────────────────┤
│ BNO08x IMU (SPI)                            │
├─────────────────────────────────────────────┤
│ GPIO11 →  MOSI (DI pin on sensor)           │
│ GPIO13 →  MISO (SDA pin on sensor) ⚠️       │
│ GPIO12 →  SCK                               │
│ GPIO10 →  CS                                │
│ GPIO5  →  INT (interrupt)                   │
│ GPIO6  →  RST (reset)                       │
│ 3.3V   →  VDD                               │
│ 3.3V   →  P0 (protocol select) ⚡           │
│ 3.3V   →  P1 (protocol select) ⚡           │
│ FLOAT  →  BT/BOOT (leave disconnected) ⚡   │
│ GND    →  GND                               │
├─────────────────────────────────────────────┤
│ RGB LED (WS2812)                            │
├─────────────────────────────────────────────┤
│ GPIO48 →  DIN                               │
│ 5V     →  VDD (or 3.3V with reduced brightness) │
│ GND    →  GND                               │
└─────────────────────────────────────────────┘
```

### ⚠️ Critical Hardware Notes

#### BNO08x Protocol Selection
The BNO08x requires **both P0 and P1 at 3.3V** for SPI mode:

| P1 | P0 | Mode |
|----|----|----|
| LOW | LOW | I2C |
| LOW | HIGH | UART-RVC |
| HIGH | LOW | UART-SHTP |
| **HIGH** | **HIGH** | **SPI** ✓ |

**Why SPI instead of I2C?**
- The BNO08x I2C implementation violates I2C protocol specs (excessive clock stretching)
- This causes **unreliable operation** with ESP32/ESP32-S3 I2C hardware
- SPI provides **stable, reliable communication** at 3 MHz
- **Separate buses**: ToF on I2C, IMU on SPI prevents bus conflicts

#### BT/BOOT Pin Behavior
- **Leave BT pin DISCONNECTED** (floating)
- Internal pull-up defaults to normal operation mode
- Connecting to GND forces bootloader mode (prevents initialization)

#### Pin Label Confusion
The BNO08x breakout board labels can be misleading:
- **"SDA" on the board** = MISO (SPI data OUT from sensor)
- **"SCL" on the board** = SCK (SPI clock)
- These are **NOT** I2C pins when in SPI mode!

---

## Sensor Specifications

### VL53L8CX Time-of-Flight Sensor
- **Technology**: Multi-zone ranging with SPAD array
- **Range**: Up to 4 meters (400 cm)
- **Resolution**: 4x4 zones (16 simultaneous measurements)
- **Update Rate**: 60 Hz
- **Field of View**: 65° diagonal
- **Interface**: I2C @ 1 MHz
- **Accuracy**: ±1% typical

### BNO08x 9-DOF IMU
- **Sensors**: 3-axis accelerometer, gyroscope, magnetometer
- **Processor**: ARM Cortex-M0+ running sensor fusion algorithms
- **Outputs**: 
  - Rotation vector (quaternion)
  - Euler angles (roll, pitch, yaw)
  - Linear acceleration (gravity-compensated)
  - Raw sensor data
- **Update Rate**: 100 Hz (configurable)
- **Interface**: SPI @ 3 MHz
- **Accuracy**: 
  - Dynamic heading: 2.5° RMS
  - Static heading: 1.0° RMS

---

## Sensor Fusion Methodology

### Data Stream Characteristics

**ToF Sensor (60 Hz)**:
- Provides spatial awareness in 4x4 grid
- Distance measurements in millimeters
- Proximity zones: Close (< 500mm), Medium (< 2000mm), Far (> 2000mm)

**IMU (100 Hz)**:
- Orientation: Roll, Pitch, Yaw in degrees
- Linear acceleration: X, Y, Z in m/s² (gravity removed)
- Enables motion context for distance measurements

### Integration Benefits

1. **Directional Awareness**: Knowing device orientation enhances spatial context
2. **Motion Compensation**: Accelerometer detects rapid movement (impacts, falls)
3. **Near-Miss Detection**: Combine proximity + motion for collision prediction
4. **Gesture Recognition**: Orientation changes + ToF can detect hand gestures

### ESP-NN Integration Readiness

This system provides **synchronized multi-sensor data** suitable for TinyML:

```cpp
// Data available for ML model input:
struct SensorFusionData {
    // ToF: 16 values (4x4 grid)
    uint16_t distances[16];
    
    // IMU: 6 values (orientation + motion)
    float roll, pitch, yaw;          // degrees
    float accel_x, accel_y, accel_z; // m/s²
    
    // Total: 22 input features for neural network
};
```

**Potential ML Applications**:
- **Collision prediction** (combining distance + velocity)
- **Activity recognition** (walking, running, falling)
- **Gesture control** (hand movements in ToF field)
- **Context-aware alerts** (different alerts based on activity)

---

## Software Architecture

### Multi-Sensor Task Structure

```
Main Loop (60 Hz ToF-driven)
├── Poll IMU data (non-blocking)
│   └── data_available() checks internal queue
├── Check ToF data ready
│   ├── Get 4x4 distance grid
│   ├── Calculate minimum distance
│   └── Update LED indicator
└── Display fusion data
    ├── ToF grid visualization
    ├── IMU orientation (if available)
    └── Linear acceleration (if available)
```

### Non-Blocking Design
- **ToF**: Primary timing source (60 Hz)
- **IMU**: Asynchronous updates via interrupt (100 Hz)
- **Display**: Updates only when ToF data arrives
- **No delays**: Maintains real-time responsiveness

### Error Handling
- **IMU optional**: System runs with ToF-only if IMU fails
- **Graceful degradation**: Continues operation with reduced functionality
- **Detailed diagnostics**: Clear error messages for troubleshooting

---

## Build and Flash Instructions

### Prerequisites
- ESP-IDF v5.5.1 (or compatible)
- Python 3.11+
- USB driver for ESP32-S3

### Environment Setup
```powershell
# Set IDF path
$env:IDF_PATH = 'D:/ESP32_Tools/Espressif/frameworks/esp-idf-v5.5.1/'

# Navigate to project
cd "d:/Master's Extension/Master's Thesis/Proximity Device/ProxiTof_AG"
```

### Build Process
```bash
# Clean build (recommended for first build)
idf.py fullclean
idf.py build

# Flash to ESP32-S3
idf.py -p COM5 flash

# Monitor output
idf.py -p COM5 monitor

# Or combined:
idf.py -p COM5 flash monitor
```

### Expected Output (Success)

```
I (xxx) ProxiTof: ✓ BNO08x initialized successfully!
I (xxx) ProxiTof: ✓ BNO08x IMU ready
I (xxx) ProxiTof: ✓ VL53L8CX is alive!
I (xxx) ProxiTof: ✓ VL53L8CX ready! (VL53L8CX_2.0.0)
I (xxx) ProxiTof: ║   System Active - ToF Running @ 60Hz       ║

╔═══════════════════════════════════════════════╗
║  ProxiTof ToF+IMU @ 60Hz | Frame 123          ║
╚═══════════════════════════════════════════════╝
  450  520  610  700 mm
  380  420  510  650 mm
  360  400  490  620 mm
  420  450  530  680 mm

 Closest:  360 mm  |  LED: [RED]   
───────────────────────────────────────────────
 IMU Orientation:
   Roll:    -2.05°  Pitch:    0.72°  Yaw:  151.61°
 Linear Acceleration:
   X:   0.00  Y:   0.01  Z:   0.00 m/s²
```

---

## Monitoring with PuTTY (Recommended)

PuTTY provides better ANSI escape code support than the ESP-IDF monitor for real-time display with colors and in-place updates.

### Finding Your COM Port

**Option 1: Device Manager (Windows)**
1. Press `Win + X` → Select **Device Manager**
2. Expand **Ports (COM & LPT)**
3. Look for **USB Serial Device (COMx)** or **Silicon Labs CP210x USB to UART Bridge (COMx)**
4. Note the COM port number (e.g., `COM5`, `COM7`)

**Option 2: ESP-IDF Command**
```powershell
# List all available serial ports
idf.py -p list

# Or use Windows PowerShell
Get-PnPDevice -Class Ports | Where-Object {$_.Status -eq "OK"}
```

### PuTTY Configuration

1. **Download PuTTY** (if not installed):
   - Download from: https://www.putty.org/
   - Or via `winget`: `winget install PuTTY.PuTTY`

2. **Configure Connection**:
   ```
   Connection Type:  Serial
   Serial line:      COM5    (replace with your actual COM port)
   Speed:            115200  (baudrate - CRITICAL!)
   ```

3. **Configure Terminal Settings** (for optimal display):
   - Go to **Terminal** → **Features**
     - ✅ Enable "Disable application cursor keys mode"
   - Go to **Window** → **Colours**
     - ✅ Enable "Allow terminal to use xterm 256-colour mode"
     - ✅ Enable "Bolded text is a different colour"
   - Go to **Window** → **Translation**
     - Set Remote character set to: **UTF-8**
   - Go to **Session**
     - Save as: **ProxiTof_Monitor** (for easy reuse)

4. **Start Monitoring**:
   - Click **Open** to connect
   - Press the **RESET** button on your ESP32-S3 to see boot messages
   - You should see colorful, formatted output with live updates

### Baudrate Settings

| Mode | Baudrate | Usage |
|------|----------|-------|
| **Standard** | **115200** | Default ESP32 serial output ✓ |
| High-speed | 460800 | Optional (may cause stability issues) |
| Debug | 921600 | Not recommended for this project |

> **⚠️ CRITICAL**: Always use **115200** baud. This is the ESP32-S3 default.

### Expected PuTTY Display

When properly configured, you should see:

```
╔═══════════════════════════════════════════════════════════╗
║         ProxiTof - Real-Time Sensor Fusion                ║
╠═══════════════════════════════════════════════════════════╣
║ Frame: 123    | Min Dist:  360 mm | Rate: 60 Hz          ║
╚═══════════════════════════════════════════════════════════╝

┌─────────────────────────────────────────────────┐
│      VL53L8CX 4x4 Distance Grid (mm)            │
├─────────────────────────────────────────────────┤
│   450   520   610   700  │  ← Color-coded:
│   380   420   510   650  │     🔴 Red    < 500mm
│   360   400   490   620  │     🟡 Yellow < 2000mm
│   420   450   530   680  │     🟢 Green  > 2000mm
└─────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────┐
│ LED: CLOSE    🔴 RED                             │
└─────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────┐
│         BNO08x IMU - Orientation (°)             │
├─────────────────────────────────────────────────┤
│ Roll:    -2.05° │ Pitch:    0.72° │ Yaw:  151.61° │
├─────────────────────────────────────────────────┤
│      BNO08x IMU - Linear Accel (m/s²)            │
├─────────────────────────────────────────────────┤
│   X:    0.00  │  Y:    0.01  │  Z:    0.00      │
└─────────────────────────────────────────────────┘

Press Ctrl+] to exit monitor
```

### Keyboard Shortcuts in PuTTY

| Key | Action |
|-----|--------|
| `Ctrl + Right-Click` | Copy selected text |
| `Right-Click` | Paste from clipboard |
| `Ctrl + C` | Send interrupt (restarts ESP32) |
| Close window or `Alt+F4` | Exit PuTTY |

### PuTTY vs ESP-IDF Monitor

| Feature | PuTTY | ESP-IDF Monitor |
|---------|-------|-----------------|
| ANSI colors | ✅ Full support | ⚠️ Limited |
| Cursor positioning | ✅ Perfect | ❌ Scrolls |
| Real-time display | ✅ Smooth 60Hz | ⚠️ Can jitter |
| Auto-decode ESP logs | ❌ No | ✅ Shows file:line |
| Crash decoding | ❌ No | ✅ Automatic |

**Recommendation**: 
- Use **PuTTY** for real-time monitoring and demonstration
- Use **ESP-IDF Monitor** (`idf.py monitor`) for debugging crashes

---

## Troubleshooting

### BNO08x IMU Not Initializing

**Symptoms**: `✗ BNO08x initialization failed!`

**Checklist**:
1. ✅ P0 pin at 3.3V (measure with multimeter)
2. ✅ P1 pin at 3.3V (measure with multimeter)
3. ✅ BT/BOOT pin floating (NOT connected)
4. ✅ All SPI pins connected (MOSI, MISO, SCK, CS, INT, RST)
5. ✅ VDD stable at 3.3V
6. ✅ Good ground connection

**Common Mistakes**:
- ❌ P1 at GND instead of 3.3V → Wrong protocol mode!
- ❌ BT pin grounded → Stuck in bootloader mode!
- ❌ MISO connected to wrong pin → No data communication!

### VL53L8CX ToF Not Detected

**Symptoms**: I2C NACK errors, device not found

**Checklist**:
1. ✅ I2C wiring correct (SDA=GPIO8, SCL=GPIO9)
2. ✅ Power stable (3.3V)
3. ✅ Ground connected (check continuity)
4. ✅ LPN pin connected to GPIO4

**Common Mistakes**:
- ❌ Loose breadboard connections (most common!)
- ❌ Ground not properly connected
- ❌ I2C pullups missing (internal pullups enabled in code)

### Serial Display Issues

**Note**: The display uses ANSI escape codes for a fixed grid.

**Best viewing**:
- Use **PuTTY** or similar terminal that supports ANSI
- ESP-IDF monitor works but may show artifacts
- Avoid Arduino Serial Monitor (limited ANSI support)

---

## Performance Metrics

### Timing Analysis
- **ToF Update**: 16.67 ms (60 Hz)
- **IMU Update**: 10 ms (100 Hz)  
- **Display Refresh**: 16.67 ms (synchronized with ToF)
- **LED Update**: < 1 ms
- **Total Latency**: < 20 ms (end-to-end)

### Resource Usage
- **CPU**: ~15% average (dual-core, 240 MHz)
- **RAM**: ~45 KB (main variables + buffers)
- **Flash**: ~850 KB (firmware + libraries)
- **SPI Speed**: 3 MHz (BNO08x)
- **I2C Speed**: 1 MHz (VL53L8CX)

---

## Dependencies

Managed via `main/idf_component.yml`:

```yaml
dependencies:
  rjrp44/vl53l8cx:
    version: "^4.0.0"
  espressif/led_strip:
    version: "^2.5.5"
  myles-parfeniuk/esp32_BNO08x:
    git: https://github.com/myles-parfeniuk/esp32_BNO08x.git
```

---

## Future Enhancements

### Immediate Next Steps (Thesis)
1. **ESP-NN Integration**: Train model on sensor fusion data
2. **Data Collection**: Log synchronized ToF+IMU data
3. **Feature Engineering**: Extract meaningful patterns
4. **Model Deployment**: Run inference on ESP32-S3

### Potential Improvements
- [ ] Add magnetometer data (full 9-DOF)
- [ ] Implement Kalman filter for smoother orientation
- [ ] Add UART output for external logging
- [ ] Web interface for real-time visualization
- [ ] SD card logging for offline analysis
- [ ] Battery power optimization

---

## References

### Datasheets
- [VL53L8CX Datasheet](https://www.st.com/resource/en/datasheet/vl53l8cx.pdf)
- [BNO08x Datasheet](https://www.ceva-ip.com/wp-content/uploads/2019/10/BNO080_085-Datasheet.pdf)
- [ESP32-S3 Technical Reference](https://www.espressif.com/sites/default/files/documentation/esp32-s3_technical_reference_manual_en.pdf)

### Libraries Used
- [VL53L8CX ESP-IDF Driver](https://github.com/rjrp44/vl53l8cx)
- [BNO08x ESP32 Driver](https://github.com/myles-parfeniuk/esp32_BNO08x)
- [ESP-IDF LED Strip](https://components.espressif.com/components/espressif/led_strip)

### Related Work
- [ESP-NN Documentation](https://github.com/espressif/esp-nn)
- [TensorFlow Lite for Microcontrollers](https://www.tensorflow.org/lite/microcontrollers)

---

## License

This project is developed for academic research purposes as part of a Master's thesis.

## Contact

For questions or collaboration:
- **Project**: ProxiTof Multi-Sensor System
- **Platform**: ESP32-S3 + ESP-IDF v5.5.1
- **Sensors**: VL53L8CX (ToF) + BNO08x (IMU)

---

**Last Updated**: 2025-11-29  
**Status**: ✅ Both sensors operational, ready for ESP-NN integration
