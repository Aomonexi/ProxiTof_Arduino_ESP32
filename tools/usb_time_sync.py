#!/usr/bin/env python3
"""
USB Time Sync Tool for ProxiTof System

This tool connects to the ESP32-S3 via USB serial and synchronizes
the system time, which gets saved to the DS3231 RTC.

Usage:
    python usb_time_sync.py [--port COM3] [--time "2024-12-24 15:30:00"]

If no time is specified, current system time is used.
If no port is specified, it will try to auto-detect.
"""

import serial
import serial.tools.list_ports
import time
import datetime
import argparse
import sys

def find_esp32_port():
    """Try to find ESP32-S3 USB port automatically"""
    ports = serial.tools.list_ports.comports()
    
    for port in ports:
        # Look for ESP32-S3 specific identifiers
        if any(keyword in port.description.lower() for keyword in 
               ['esp32', 'usb serial', 'ch343', 'cp210', 'ftdi']):
            print(f"Found potential ESP32 port: {port.device} - {port.description}")
            return port.device
    
    # If no specific match, return the first available port
    if ports:
        print(f"Using first available port: {ports[0].device}")
        return ports[0].device
    
    return None

def sync_time_via_usb(port, time_str=None):
    """Sync time with ESP32 via USB serial"""
    
    if time_str is None:
        # Use current system time
        now = datetime.datetime.now()
        time_str = now.strftime("%Y-%m-%d %H:%M:%S")
    
    print(f"Connecting to {port}...")
    
    try:
        # Open serial connection
        ser = serial.Serial(port, 115200, timeout=5)
        time.sleep(2)  # Wait for connection to stabilize
        
        print(f"Sending time sync command: {time_str}")
        
        # Send time sync command
        command = f"TIME:{time_str}\r\n"
        ser.write(command.encode())
        ser.flush()
        
        # Wait for response
        print("Waiting for response...")
        start_time = time.time()
        response_received = False
        
        while time.time() - start_time < 10:  # 10 second timeout
            if ser.in_waiting > 0:
                response = ser.readline().decode().strip()
                print(f"ESP32 Response: {response}")
                
                if "TIME_SYNC_OK" in response:
                    print("✓ Time synchronized successfully!")
                    print("✓ Time saved to DS3231 RTC (if available)")
                    response_received = True
                    break
                elif "TIME_SYNC_ERROR" in response:
                    print(f"❌ Time sync failed: {response}")
                    response_received = True
                    break
        
        if not response_received:
            print("⚠️  No response received (timeout)")
            print("💡 Check if ESP32 is running and USB cable is connected")
        
        ser.close()
        return response_received
        
    except serial.SerialException as e:
        print(f"❌ Serial connection failed: {e}")
        print("💡 Make sure:")
        print("   1. ESP32 is connected via USB")
        print("   2. Correct COM port is selected")
        print("   3. No other program is using the port")
        return False
    except Exception as e:
        print(f"❌ Unexpected error: {e}")
        return False

def test_connection(port):
    """Test connection to ESP32 and get status"""
    print(f"Testing connection to {port}...")
    
    try:
        ser = serial.Serial(port, 115200, timeout=3)
        time.sleep(2)
        
        # Send status command
        ser.write(b"STATUS\r\n")
        ser.flush()
        
        # Wait for response
        start_time = time.time()
        while time.time() - start_time < 5:
            if ser.in_waiting > 0:
                response = ser.readline().decode().strip()
                if response.startswith("STATUS:"):
                    print(f"✓ Connection OK - {response}")
                    ser.close()
                    return True
        
        print("⚠️  No status response received")
        ser.close()
        return False
        
    except Exception as e:
        print(f"❌ Connection test failed: {e}")
        return False

def main():
    parser = argparse.ArgumentParser(description="USB Time Sync Tool for ProxiTof System")
    parser.add_argument("--port", "-p", help="Serial port (e.g., COM3, /dev/ttyUSB0)")
    parser.add_argument("--time", "-t", help="Time to set (YYYY-MM-DD HH:MM:SS). Uses current time if not specified.")
    parser.add_argument("--test", action="store_true", help="Test connection only")
    parser.add_argument("--list-ports", action="store_true", help="List available serial ports")
    
    args = parser.parse_args()
    
    if args.list_ports:
        print("Available serial ports:")
        ports = serial.tools.list_ports.comports()
        for port in ports:
            print(f"  {port.device} - {port.description}")
        return
    
    # Find or use specified port
    if args.port:
        port = args.port
    else:
        port = find_esp32_port()
        if not port:
            print("❌ No serial ports found")
            print("💡 Make sure ESP32 is connected via USB")
            return
    
    print(f"Using port: {port}")
    
    if args.test:
        test_connection(port)
        return
    
    # Validate time format if provided
    if args.time:
        try:
            datetime.datetime.strptime(args.time, "%Y-%m-%d %H:%M:%S")
        except ValueError:
            print("❌ Invalid time format. Use: YYYY-MM-DD HH:MM:SS")
            return
    
    # Perform time sync
    success = sync_time_via_usb(port, args.time)
    
    if success:
        print("\n🎉 Time synchronization completed!")
        print("💡 The ESP32 system time is now synchronized")
        print("💾 Time has been saved to DS3231 RTC for persistence")
    else:
        print("\n❌ Time synchronization failed")
        print("💡 Try running with --test to check connection")

if __name__ == "__main__":
    main()