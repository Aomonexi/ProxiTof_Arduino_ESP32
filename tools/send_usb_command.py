#!/usr/bin/env python3
"""
USB Serial JTAG Command Sender
Send commands to ESP32-S3 via RIGHT port (USB Serial JTAG)
"""

import serial
import sys
import time

def send_command(port, command):
    """Send a command to ESP32-S3 via USB Serial JTAG"""
    try:
        # Open serial connection with shorter timeout
        ser = serial.Serial(port, 115200, timeout=1)
        time.sleep(0.2)  # Small delay for connection
        
        print(f"📨 Sending command: {command}")
        
        # Clear any existing data in buffer
        ser.reset_input_buffer()
        ser.reset_output_buffer()
        
        # Send command
        ser.write(f"{command}\r\n".encode())
        ser.flush()
        
        # Wait for response with timeout
        start_time = time.time()
        response = ""
        
        while (time.time() - start_time) < 5.0:  # 5 second timeout
            if ser.in_waiting > 0:
                data = ser.read(ser.in_waiting).decode('utf-8', errors='ignore')
                response += data
                # Continue reading for a bit longer to get complete response
            time.sleep(0.1)
        
        if response.strip():
            print(f"📥 Response: {response.strip()}")
        else:
            print("📥 No response received (device may need firmware update)")
            print("💡 Try flashing the device first: idf.py flash")
        
        ser.close()
        return True
        
    except serial.SerialException as e:
        print(f"❌ Serial error: {e}")
        print("💡 Make sure device is connected to RIGHT port (COM port)")
        return False
    except KeyboardInterrupt:
        print("\n⚠️  Command interrupted by user")
        return False
    except Exception as e:
        print(f"❌ Error: {e}")
        return False

def main():
    if len(sys.argv) < 2:
        print("Usage: python send_usb_command.py <command> [port]")
        print("")
        print("Available commands:")
        print("  STATUS           - Get system status")
        print("  TEST_FILE_WRITE  - Test SD card writing")
        print("  USB_MSC_START    - Start USB Mass Storage")
        print("  USB_MSC_STOP     - Stop USB Mass Storage")
        print("  HELP             - Show ESP32 help")
        print("  TIME:2026-01-07 15:30:00 - Set time")
        print("")
        print("Example:")
        print("  python send_usb_command.py STATUS")
        print("  python send_usb_command.py STATUS COM3")
        return
    
    command = sys.argv[1]
    
    # Auto-detect port or use provided port
    if len(sys.argv) >= 3:
        port = sys.argv[2]
    else:
        # Try common Windows COM ports
        ports_to_try = ['COM3', 'COM4', 'COM5', 'COM6', 'COM7', 'COM8']
        port = None
        
        print("🔍 Auto-detecting ESP32-S3 port...")
        for test_port in ports_to_try:
            try:
                ser = serial.Serial(test_port, 115200, timeout=0.5)
                ser.close()
                port = test_port
                print(f"✓ Found ESP32-S3 on {port}")
                break
            except:
                continue
        
        if not port:
            print("❌ Could not auto-detect port. Please specify manually:")
            print("  python send_usb_command.py STATUS COM3")
            return
    
    print(f"🔌 Using port: {port}")
    send_command(port, command)

if __name__ == "__main__":
    main()