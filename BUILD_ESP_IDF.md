# Building with ESP-IDF Native Build System

This project can be built using either PlatformIO or the native ESP-IDF build system.

## Prerequisites

1. **Install ESP-IDF**: Follow the official guide at https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/get-started/
2. **ESP-IDF Version**: This project requires ESP-IDF v4.4 or later (v5.x recommended)

## Building with ESP-IDF

### 1. Set up ESP-IDF environment

On Windows (PowerShell):
```powershell
# Navigate to your ESP-IDF installation directory
cd C:\path\to\esp-idf

# Run the export script to set up environment variables
.\export.ps1
```

On Windows (Command Prompt):
```cmd
cd C:\path\to\esp-idf
export.bat
```

On Linux/macOS:
```bash
cd ~/esp/esp-idf
. ./export.sh
```

### 2. Navigate to project directory

```bash
cd /path/to/ProxiTof_Arduino_ESP32
```

### 3. Configure the project (first time only)

```bash
idf.py set-target esp32s3
idf.py menuconfig
```

In menuconfig, you can adjust:
- Serial flasher config (baud rate, port)
- Component config settings
- Partition table settings

### 4. Build the project

```bash
idf.py build
```

This will:
- Compile all components (sensor_manager, storage_manager, data_processor)
- Link the final binary
- Generate flash images

### 5. Flash to device

```bash
idf.py -p COM3 flash
```

Replace `COM3` with your actual serial port:
- Windows: `COM3`, `COM4`, etc.
- Linux: `/dev/ttyUSB0`, `/dev/ttyACM0`, etc.
- macOS: `/dev/cu.usbserial-*`

### 6. Monitor serial output

```bash
idf.py -p COM3 monitor
```

Or combine flash and monitor:
```bash
idf.py -p COM3 flash monitor
```

Press `Ctrl+]` to exit the monitor.

## Project Structure

```
ProxiTof_Arduino_ESP32/
├── CMakeLists.txt              # Root CMake file
├── sdkconfig                   # ESP-IDF configuration
├── main/
│   ├── CMakeLists.txt         # Main component
│   └── main.c                 # Application entry point
├── components/
│   ├── sensor_manager/        # Sensor management component
│   ├── storage_manager/       # SD card and logging component
│   └── data_processor/        # NEW: Data processing component
├── managed_components/        # External dependencies
└── include/                   # Shared headers (DataModels.h)
```

## Component Dependencies

The data_processor component depends on:
- DataModels.h (shared data structures)
- Arduino.h (for compatibility)

All components are automatically discovered by ESP-IDF from the `components/` directory.

## Troubleshooting

### Issue: "Component not found"

Make sure the component has:
1. A `CMakeLists.txt` file with `idf_component_register()`
2. An `idf_component.yml` file (optional but recommended)
3. Proper SRCS and INCLUDE_DIRS specified

### Issue: "Arduino.h not found"

The project uses Arduino as an ESP-IDF component. Make sure:
1. Arduino is enabled in menuconfig: `Component config → Arduino Configuration`
2. Or add `arduino` to the REQUIRES list in your component's CMakeLists.txt

### Issue: Build fails with missing DataModels.h

The DataModels.h is in the `include/` directory. You may need to add it to the include path:

In your component's CMakeLists.txt:
```cmake
idf_component_register(
    SRCS "your_source.cpp"
    INCLUDE_DIRS "include" "../../include"
    REQUIRES ""
)
```

## Clean Build

If you encounter issues, try a clean build:

```bash
idf.py fullclean
idf.py build
```

## Advanced: Custom Partition Table

For larger applications with ML models, you may need a custom partition table.

1. Create `partitions.csv`:
```csv
# Name,   Type, SubType, Offset,  Size, Flags
nvs,      data, nvs,     0x9000,  0x6000,
phy_init, data, phy,     0xf000,  0x1000,
factory,  app,  factory, 0x10000, 2M,
storage,  data, fat,     ,        4M,
```

2. Configure in menuconfig:
   - `Partition Table → Partition Table → Custom partition table CSV`
   - `Partition Table → Custom partition CSV file → partitions.csv`

## Comparison: PlatformIO vs ESP-IDF

| Feature | PlatformIO | ESP-IDF |
|---------|-----------|---------|
| Setup | Easier | More complex |
| IDE Integration | Excellent (VSCode) | Good (VSCode, Eclipse) |
| Build Speed | Slower | Faster |
| Debugging | Integrated | Requires setup |
| Component Management | Automatic | Manual |
| Flexibility | Limited | Full control |

**Recommendation**: Use PlatformIO for development, ESP-IDF for production builds.

## Next Steps

After successful build:
1. Test the data_processor component functionality
2. Verify sensor synchronization works correctly
3. Test feature extraction calculations
4. Validate near-miss detection logic

For testing, see `test/` directory for property-based tests.
