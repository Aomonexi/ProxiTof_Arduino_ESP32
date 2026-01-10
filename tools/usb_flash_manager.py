#!/usr/bin/env python3
"""
USB Flash Manager for ProxiTof ESP32-S3
Manages the new internal flash partition USB MSC system
"""

import serial
import serial.tools.list_ports
import time
import sys
import argparse

def find_esp32_port():
    """Auto-detect ESP32-S3 USB Serial JTAG port"""
    print("🔍 Auto-detecting ESP32-S3 port...")
    
    # Look for ESP32-S3 USB Serial JTAG
    for port in serial.tools.list_ports.comports():
        if "USB Serial" in port.description or "ESP32-S3" in port.description:
            print(f"✓ Found ESP32-S3 on {port.device}")
            return port.device
    
    # Fallback: look for common ESP32 patterns
    for port in serial.tools.list_ports.comports():
        if any(pattern in port.description.lower() for pattern in ["esp32", "cp210", "ch340", "ch343"]):
            print(f"? Possible ESP32 on {port.device}: {port.description}")
            return port.device
    
    print("❌ No ESP32-S3 found. Please specify port manually.")
    return None

def send_command(port, command, timeout=5):
    """Send command to ESP32-S3 and get response"""
    try:
        with serial.Serial(port, 115200, timeout=timeout) as ser:
            print(f"🔌 Using port: {port}")
            print(f"📨 Sending command: {command}")
            
            # Clear any existing data
            ser.reset_input_buffer()
            
            # Send command
            ser.write(f"{command}\r\n".encode())
            
            # Read response
            start_time = time.time()
            response_lines = []
            
            while time.time() - start_time < timeout:
                if ser.in_waiting > 0:
                    line = ser.readline().decode('utf-8', errors='ignore').strip()
                    if line:
                        response_lines.append(line)
                        print(f"📥 {line}")
                        
                        # Check for command completion
                        if any(keyword in line for keyword in ["_OK", "_ERROR", "_STARTED", "_STOPPED"]):
                            break
                
                time.sleep(0.1)
            
            if not response_lines:
                print("⚠️  No response received")
                return False
            
            # Check if command was successful
            success = any("_OK" in line or "_STARTED" in line for line in response_lines)
            return success
            
    except serial.SerialException as e:
        print(f"❌ Serial error: {e}")
        return False
    except KeyboardInterrupt:
        print("\n⚠️  Operation cancelled by user")
        return False

def main():
    parser = argparse.ArgumentParser(description="USB Flash Manager for ProxiTof ESP32-S3")
    parser.add_argument("--port", help="Serial port (auto-detect if not specified)")
    parser.add_argument("command", choices=["start", "stop", "sync", "status", "help"], 
                       help="Command to execute")
    
    args = parser.parse_args()
    
    # Auto-detect port if not specified
    port = args.port or find_esp32_port()
    if not port:
        sys.exit(1)
    
    print(f"\n🚀 ProxiTof USB Flash Manager")
    print(f"{'='*50}")
    
    # Execute command
    if args.command == "start":
        print("📁 Starting USB flash drive mode...")
        print("💡 Device will appear as removable drive")
        print("💡 LED should turn BLUE")
        success = send_command(port, "USB_MSC_START")
        if success:
            print("\n✅ USB flash drive started successfully!")
            print("💾 Check your computer for new removable drive")
            print("📁 Drive contains recent data files")
        else:
            print("\n❌ Failed to start USB flash drive")
            
    elif args.command == "stop":
        print("🛑 Stopping USB flash drive mode...")
        print("💡 LED should return to distance indication")
        success = send_command(port, "USB_MSC_STOP")
        if success:
            print("\n✅ USB flash drive stopped successfully!")
            print("📊 Data logging resumed")
        else:
            print("\n❌ Failed to stop USB flash drive")
            
    elif args.command == "sync":
        print("🔄 Syncing recent data files...")
        print("💡 Copying recent CSV files from SD card to flash partition")
        success = send_command(port, "USB_SYNC_DATA")
        if success:
            print("\n✅ Data sync completed successfully!")
            print("📁 Recent files now available on USB drive")
            print("💡 Use 'start' command to access via USB")
        else:
            print("\n❌ Failed to sync data files")
            
    elif args.command == "status":
        print("📊 Getting system status...")
        success = send_command(port, "STATUS")
        if not success:
            print("\n❌ Failed to get system status")
            
    elif args.command == "help":
        print("📖 Available commands:")
        print("  start  - Start USB flash drive mode")
        print("  stop   - Stop USB flash drive mode")
        print("  sync   - Sync recent data files to flash partition")
        print("  status - Get system status")
        print("  help   - Show this help")
        print("\n💡 Workflow:")
        print("  1. sync   - Copy recent files to flash partition")
        print("  2. start  - Enable USB flash drive")
        print("  3. Access files on computer")
        print("  4. stop   - Return to normal operation")

if __name__ == "__main__":
    main()