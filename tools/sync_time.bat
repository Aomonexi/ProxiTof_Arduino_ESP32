@echo off
echo ProxiTof USB Time Sync Tool
echo ===========================
echo.

REM Check if Python is available
python --version >nul 2>&1
if errorlevel 1 (
    echo ERROR: Python is not installed or not in PATH
    echo Please install Python 3.x and try again
    pause
    exit /b 1
)

REM Check if pyserial is installed
python -c "import serial" >nul 2>&1
if errorlevel 1 (
    echo Installing pyserial...
    pip install pyserial
    if errorlevel 1 (
        echo ERROR: Failed to install pyserial
        echo Please run: pip install pyserial
        pause
        exit /b 1
    )
)

echo Synchronizing time with ESP32...
python "%~dp0usb_time_sync.py" %*

if errorlevel 1 (
    echo.
    echo Time sync failed. Try these troubleshooting steps:
    echo 1. Make sure ESP32 is connected via USB
    echo 2. Check if the correct COM port is being used
    echo 3. Close any other programs using the serial port
    echo 4. Try specifying the port manually: sync_time.bat --port COM3
    echo.
)

pause