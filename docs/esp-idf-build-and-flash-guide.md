# ESP-IDF Build & Flash Guide — ProxiTof

Quick reference for building and flashing the ProxiTof firmware using ESP-IDF v5.5.1 on Windows.

## Prerequisites

- ESP-IDF v5.5.1 installed at `C:\Users\<USERNAME>\esp\v5.5.1\esp-idf\`
- ESP32-S3-DevKitC-1 board
- USB cable connected to the board
- Project directory: `F:\Master's_Extension\Master's_Thesis\Proximity_Device\ProxiTof_Arduino_ESP32`

## Step 1: Activate the ESP-IDF Environment

Every time you open a new terminal, you must source the ESP-IDF environment first. Without this, `idf.py` won't be recognized.

```powershell
& "C:\Users\<USERNAME>\esp\v5.5.1\esp-idf\export.ps1"
```

You should see output like:

```
Activating ESP-IDF 5.5
Setting IDF_PATH to 'C:\Users\<USERNAME>\esp\v5.5.1\esp-idf'.
...
Done! You can now compile ESP-IDF projects.
```

Verify it worked:

```powershell
idf.py --version
```

## Step 2: Navigate to the Project

```powershell
cd "F:\Master's_Extension\Master's_Thesis\Proximity_Device\ProxiTof_Arduino_ESP32"
```

## Step 3: Clean Workspace (When Needed)

A clean build is recommended when:
- You switched ESP-IDF versions
- You changed `sdkconfig` or `sdkconfig.defaults`
- You added/removed components or managed dependencies
- The build directory was created by a different project path
- You're getting weird linker or CMake errors

### Option A: Full Clean (removes build + managed components)

```powershell
Remove-Item -Recurse -Force build
Remove-Item -Recurse -Force managed_components
idf.py build
```

This forces ESP-IDF to re-download managed components (like `esp-tflite-micro`) and regenerate all build files. Takes longer but guarantees a fresh state.

### Option B: Standard Clean

```powershell
idf.py fullclean
idf.py build
```

This removes the build directory but keeps managed components. Faster than Option A.

### Option C: Incremental Build (no clean)

```powershell
idf.py build
```

Use this for day-to-day development when you only changed source files. Fastest option.

## Step 4: Build

```powershell
idf.py build
```

The build compiles ~1400 objects (including TFLite Micro). First build takes 5-10 minutes. Incremental builds are much faster.

Expected final output:

```
Project build complete. To flash, run:
 idf.py flash
```

It also shows binary size info:

```
proxitof.bin binary size 0x130050 bytes. Smallest app partition is 0x180000 bytes. 0x4ffb0 bytes (21%) free.
```

## Step 5: Flash

Connect the ESP32-S3 via USB, then:

```powershell
idf.py flash
```

Or specify the COM port explicitly:

```powershell
idf.py -p COM3 flash
```

To find your COM port on Windows:
- Open Device Manager → Ports (COM & LPT)
- Look for "USB-SERIAL" or "CP210x" or "JTAG"
- The ESP32-S3-DevKitC-1 may show two ports — use the lower-numbered one

## Step 6: Monitor Serial Output

```powershell
idf.py -p COM3 monitor
```

Or combine flash and monitor in one command:

```powershell
idf.py -p COM3 flash monitor
```

Exit the monitor with `Ctrl+]`.

## Quick Reference — Copy-Paste Commands

### Fresh build (new terminal)

```powershell
& "C:\Users\<USERNAME>\esp\v5.5.1\esp-idf\export.ps1"
cd "F:\Master's_Extension\Master's_Thesis\Proximity_Device\ProxiTof_Arduino_ESP32"
idf.py build
```

### Full clean + build (new terminal)

```powershell
& "C:\Users\<USERNAME>\esp\v5.5.1\esp-idf\export.ps1"
cd "F:\Master's_Extension\Master's_Thesis\Proximity_Device\ProxiTof_Arduino_ESP32"
Remove-Item -Recurse -Force build
Remove-Item -Recurse -Force managed_components
idf.py build
```

### Build + flash + monitor (new terminal)

```powershell
& "C:\Users\<USERNAME>\esp\v5.5.1\esp-idf\export.ps1"
cd "F:\Master's_Extension\Master's_Thesis\Proximity_Device\ProxiTof_Arduino_ESP32"
idf.py build; idf.py -p COM3 flash monitor
```

## Troubleshooting

### `idf.py : The term 'idf.py' is not recognized`

You forgot to run `export.ps1`. Run it first (Step 1).

### `CMake Error: The source directory does not contain a CMakeLists.txt file`

You're not in the project directory. Make sure you `cd` to the project root.

### `Build directory was configured for a different project`

The `build/` folder has stale CMake cache from a different path. Fix:

```powershell
Remove-Item -Recurse -Force build
idf.py build
```

### `fatal error: <some_header>.h: No such file or directory`

A component is missing a dependency. Check the component's `CMakeLists.txt` REQUIRES list. Run a full clean build if unsure.

### `Component manager: downloading...` takes forever

The `esp-tflite-micro` component is large (~200MB). First download takes a while. Subsequent builds reuse the cached version in `managed_components/`.

### Flash fails with `serial port not found`

- Check USB cable (some cables are charge-only, no data)
- Check Device Manager for the correct COM port
- Try holding BOOT button on the ESP32-S3 while flashing
- On ESP32-S3-DevKitC-1, use the USB-UART port (not the USB-OTG port)

### Binary too large

If you see `app partition is too small`, you need to adjust the partition table in `partitions.csv` or reduce code size.

## Build Targets Reference

| Command | What it does |
|---------|-------------|
| `idf.py build` | Compile the project |
| `idf.py flash` | Flash firmware to device |
| `idf.py monitor` | Open serial monitor |
| `idf.py flash monitor` | Flash then monitor |
| `idf.py fullclean` | Delete build directory |
| `idf.py menuconfig` | Open configuration menu |
| `idf.py set-target esp32s3` | Set chip target (first time) |
| `idf.py size` | Show binary size breakdown |
| `idf.py size-components` | Show size per component |
