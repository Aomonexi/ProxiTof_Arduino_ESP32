@echo off
REM Quick start script for ProxiTof USB Flash Drive
REM Windows batch file for easy USB flash drive access

echo.
echo ========================================
echo   ProxiTof USB Flash Drive Manager
echo ========================================
echo.

REM Check if Python is available
python --version >nul 2>&1
if errorlevel 1 (
    echo ERROR: Python not found. Please install Python first.
    pause
    exit /b 1
)

:menu
echo Choose an option:
echo.
echo 1. Sync recent data files
echo 2. Start USB flash drive
echo 3. Stop USB flash drive
echo 4. Get system status
echo 5. Exit
echo.
set /p choice="Enter your choice (1-5): "

if "%choice%"=="1" (
    echo.
    echo Syncing recent data files...
    python usb_flash_manager.py sync
    echo.
    pause
    goto menu
)

if "%choice%"=="2" (
    echo.
    echo Starting USB flash drive...
    python usb_flash_manager.py start
    echo.
    echo Check your computer for new removable drive!
    pause
    goto menu
)

if "%choice%"=="3" (
    echo.
    echo Stopping USB flash drive...
    python usb_flash_manager.py stop
    echo.
    pause
    goto menu
)

if "%choice%"=="4" (
    echo.
    echo Getting system status...
    python usb_flash_manager.py status
    echo.
    pause
    goto menu
)

if "%choice%"=="5" (
    echo.
    echo Goodbye!
    exit /b 0
)

echo Invalid choice. Please try again.
echo.
goto menu