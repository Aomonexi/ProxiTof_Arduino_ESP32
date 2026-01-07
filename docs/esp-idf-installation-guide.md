# ESP-IDF Installation Guide

Complete guide for installing ESP-IDF v5.5+ on Windows, macOS, and Linux for ProxiTof development.

## Overview

ESP-IDF (Espressif IoT Development Framework) is the official development framework for ESP32 series chips. This guide covers installation and setup for ProxiTof development.

**Requirements:**
- ESP-IDF v5.5 or later
- Python 3.8 or later
- Git
- CMake and Ninja build tools

## Windows Installation

### Method 1: ESP-IDF Installer (Recommended)

1. **Download ESP-IDF Installer**
   - Go to [ESP-IDF Releases](https://github.com/espressif/esp-idf/releases)
   - Download `esp-idf-tools-setup-x.x.x.exe` (latest v5.5+)

2. **Run Installer**
   - Run the installer as Administrator
   - Choose installation path: `C:\Users\[username]\esp\v5.5.1\esp-idf`
   - Select "Install ESP-IDF" and "Install ESP-IDF Tools"
   - Wait for installation to complete (~15-20 minutes)

3. **Verify Installation**
   ```powershell
   # Open PowerShell and navigate to ESP-IDF directory
   cd C:\Users\[username]\esp\v5.5.1\esp-idf
   
   # Activate ESP-IDF environment
   .\export.ps1
   
   # Verify installation
   idf.py --version
   ```

### Method 2: Manual Installation

1. **Install Prerequisites**
   ```powershell
   # Install Python 3.8+ from python.org
   # Install Git from git-scm.com
   # Install CMake from cmake.org
   ```

2. **Clone ESP-IDF**
   ```powershell
   mkdir C:\Users\[username]\esp
   cd C:\Users\[username]\esp
   git clone -b v5.5.1 --recursive https://github.com/espressif/esp-idf.git v5.5.1/esp-idf
   cd v5.5.1/esp-idf
   ```

3. **Install ESP-IDF Tools**
   ```powershell
   .\install.ps1
   ```

4. **Set up Environment**
   ```powershell
   .\export.ps1
   ```

### Setting Up Environment Variables (Windows)

For permanent setup, add to your PowerShell profile:

1. **Create/Edit PowerShell Profile**
   ```powershell
   notepad $PROFILE
   ```

2. **Add ESP-IDF Alias**
   ```powershell
   # ESP-IDF Environment Setup
   function Set-EspIdfEnv {
       & "C:\Users\$env:USERNAME\esp\v5.5.1\esp-idf\export.ps1"
   }
   Set-Alias -Name "esp-idf" -Value Set-EspIdfEnv
   ```

3. **Usage**
   ```powershell
   # Activate ESP-IDF environment
   esp-idf
   
   # Verify
   idf.py --version
   ```

## macOS Installation

### Method 1: Using Homebrew (Recommended)

1. **Install Prerequisites**
   ```bash
   # Install Homebrew if not already installed
   /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
   
   # Install required tools
   brew install cmake ninja dfu-util python3 git
   ```

2. **Clone ESP-IDF**
   ```bash
   mkdir -p ~/esp
   cd ~/esp
   git clone -b v5.5.1 --recursive https://github.com/espressif/esp-idf.git v5.5.1/esp-idf
   cd v5.5.1/esp-idf
   ```

3. **Install ESP-IDF Tools**
   ```bash
   ./install.sh
   ```

4. **Set up Environment**
   ```bash
   . ./export.sh
   ```

### Method 2: ESP-IDF Installer

1. **Download and Run Installer**
   - Download ESP-IDF installer for macOS
   - Follow installation wizard
   - Choose installation path: `~/esp/v5.5.1/esp-idf`

### Setting Up Environment (macOS)

Add to your shell profile (`~/.zshrc` or `~/.bash_profile`):

```bash
# ESP-IDF Environment Setup
alias esp-idf='. ~/esp/v5.5.1/esp-idf/export.sh'

# Optional: Auto-activate in specific directories
function cd() {
    builtin cd "$@"
    if [[ -f "CMakeLists.txt" && -f "sdkconfig.defaults" ]]; then
        if [[ ! $IDF_PATH ]]; then
            echo "ESP-IDF project detected. Activating environment..."
            . ~/esp/v5.5.1/esp-idf/export.sh
        fi
    fi
}
```

## Linux Installation (Ubuntu/Debian)

### Install Prerequisites

```bash
# Update package list
sudo apt update

# Install required packages
sudo apt install git wget flex bison gperf python3 python3-pip python3-venv cmake ninja-build ccache libffi-dev libssl-dev dfu-util libusb-1.0-0
```

### Install ESP-IDF

1. **Clone Repository**
   ```bash
   mkdir -p ~/esp
   cd ~/esp
   git clone -b v5.5.1 --recursive https://github.com/espressif/esp-idf.git v5.5.1/esp-idf
   cd v5.5.1/esp-idf
   ```

2. **Install Tools**
   ```bash
   ./install.sh esp32s3
   ```

3. **Set up Environment**
   ```bash
   . ./export.sh
   ```

### Setting Up Environment (Linux)

Add to `~/.bashrc` or `~/.zshrc`:

```bash
# ESP-IDF Environment Setup
alias esp-idf='. ~/esp/v5.5.1/esp-idf/export.sh'

# Optional: Auto-activate function
esp-idf-activate() {
    if [[ -f "CMakeLists.txt" && -f "sdkconfig.defaults" ]]; then
        if [[ ! $IDF_PATH ]]; then
            echo "Activating ESP-IDF environment..."
            . ~/esp/v5.5.1/esp-idf/export.sh
        fi
    fi
}
```

## Verification and Testing

### Verify Installation

```bash
# Activate ESP-IDF environment (run appropriate command for your OS)
# Windows: .\export.ps1
# macOS/Linux: . ./export.sh

# Check ESP-IDF version
idf.py --version

# Check Python and tools
python --version
cmake --version
ninja --version

# List available targets
idf.py --list-targets
```

Expected output:
```
ESP-IDF v5.5.1
Python 3.x.x
cmake version 3.x.x
ninja 1.x.x
esp32, esp32s2, esp32s3, esp32c2, esp32c3, esp32c6, esp32h2
```

### Test with ProxiTof Project

1. **Clone ProxiTof Repository**
   ```bash
   git clone https://github.com/yourusername/ProxiTof_Arduino_ESP32.git
   cd ProxiTof_Arduino_ESP32
   ```

2. **Activate ESP-IDF Environment**
   ```bash
   # Windows PowerShell
   C:\Users\[username]\esp\v5.5.1\esp-idf\export.ps1
   
   # macOS/Linux
   . ~/esp/v5.5.1/esp-idf/export.sh
   ```

3. **Build Project**
   ```bash
   idf.py build
   ```

4. **Flash to ESP32-S3** (with device connected)
   ```bash
   idf.py flash monitor
   ```

## Common Issues and Solutions

### Windows Issues

**Issue: PowerShell execution policy**
```powershell
# Solution: Set execution policy
Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser
```

**Issue: Python not found**
```powershell
# Solution: Add Python to PATH or reinstall Python with "Add to PATH" option
```

**Issue: USB driver not recognized**
- Install CP210x USB driver from Silicon Labs
- Install CH340 driver if using clone boards

### macOS Issues

**Issue: Command line tools not found**
```bash
# Solution: Install Xcode command line tools
xcode-select --install
```

**Issue: Permission denied on USB port**
```bash
# Solution: Add user to dialout group (if exists) or use sudo
sudo dmesg | grep tty  # Find device name
sudo chmod 666 /dev/tty.usbserial-*
```

### Linux Issues

**Issue: Permission denied on USB port**
```bash
# Solution: Add user to dialout group
sudo usermod -a -G dialout $USER
# Log out and log back in
```

**Issue: Python virtual environment issues**
```bash
# Solution: Install python3-venv
sudo apt install python3-venv
```

## IDE Integration

### Visual Studio Code

1. **Install ESP-IDF Extension**
   - Open VS Code
   - Install "ESP-IDF" extension by Espressif
   - Configure ESP-IDF path: `Ctrl+Shift+P` → "ESP-IDF: Configure ESP-IDF Extension"

2. **Open ProxiTof Project**
   - File → Open Folder → Select ProxiTof project directory
   - Extension will auto-detect ESP-IDF project

3. **Build and Flash**
   - Use Command Palette: `Ctrl+Shift+P`
   - "ESP-IDF: Build Project"
   - "ESP-IDF: Flash Device"
   - "ESP-IDF: Monitor Device"

### CLion

1. **Install ESP-IDF Plugin**
   - File → Settings → Plugins
   - Search and install "ESP-IDF" plugin

2. **Configure Toolchain**
   - File → Settings → Build, Execution, Deployment → Toolchains
   - Add ESP-IDF toolchain
   - Set ESP-IDF path and tools

## Environment Management

### Daily Workflow

**Start Development Session:**
```bash
# Navigate to project
cd /path/to/ProxiTof_Arduino_ESP32

# Activate ESP-IDF (choose your platform)
# Windows:
C:\Users\[username]\esp\v5.5.1\esp-idf\export.ps1

# macOS/Linux:
. ~/esp/v5.5.1/esp-idf/export.sh

# Verify environment
idf.py --version

# Build and flash
idf.py build
idf.py flash monitor
```

### Multiple ESP-IDF Versions

If you need multiple ESP-IDF versions:

```bash
# Install different versions in separate directories
~/esp/v5.5.1/esp-idf
~/esp/v5.4.0/esp-idf

# Create version-specific aliases
alias esp-idf-5.5='. ~/esp/v5.5.1/esp-idf/export.sh'
alias esp-idf-5.4='. ~/esp/v5.4.0/esp-idf/export.sh'
```

## Performance Optimization

### Build Speed Optimization

1. **Enable ccache**
   ```bash
   # Already included in ESP-IDF setup
   idf.py --ccache build
   ```

2. **Parallel Build**
   ```bash
   # Use all CPU cores
   idf.py build -j$(nproc)  # Linux/macOS
   idf.py build -j%NUMBER_OF_PROCESSORS%  # Windows
   ```

3. **Component-only Build**
   ```bash
   # Build specific component only
   idf.py build --component sensor_manager
   ```

## Troubleshooting

### Reset ESP-IDF Environment

If you encounter persistent issues:

```bash
# Clean build directory
idf.py fullclean

# Reconfigure project
idf.py reconfigure

# Reinstall ESP-IDF tools
cd ~/esp/v5.5.1/esp-idf  # or your ESP-IDF path
./install.sh --reinstall
```

### Check ESP-IDF Health

```bash
# Run ESP-IDF doctor
idf.py doctor

# Check Python environment
python -m pip list | grep esp

# Verify tools installation
ls $IDF_TOOLS_PATH  # Should show installed tools
```

## Next Steps

After successful installation:

1. **Read [ESP-IDF Build Guide](esp-idf-build-guide.md)** for ProxiTof-specific build instructions
2. **Follow [Complete Wiring Diagram](complete-wiring-diagram.md)** for hardware setup
3. **Check [Quick Reference Card](quick-reference-card.md)** for common commands

## Support

**Official ESP-IDF Resources:**
- [ESP-IDF Programming Guide](https://docs.espressif.com/projects/esp-idf/en/latest/)
- [ESP-IDF GitHub Repository](https://github.com/espressif/esp-idf)
- [Espressif Community Forum](https://esp32.com/)

**ProxiTof-Specific Help:**
- Check [System Overview](system-overview.md) for architecture details
- See [Troubleshooting Guide](README.md#troubleshooting) for common issues
- Review [Build and Test Guide](build-and-test-guide.md) for development workflow

---

**Installation complete!** You're now ready to build and develop ProxiTof firmware with ESP-IDF. 🚀