#!/usr/bin/env python3
"""
USB Debug Tool for ProxiTof ESP32-S3
Debug USB MSC issues and check system status
"""

import serial
import serial.tools.list_ports
import time
import sys

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

def debug_usb_msc(port):
    """Debug USB MSC functionality"""
    try:
        with serial.Serial(port, 115200, timeout=2) as ser:
            print(f"🔌 Connected to: {port}")
            print("📊 Debugging USB MSC functionality...")
            print("=" * 60)
            
            # Clear any existing data
            ser.reset_input_buffer()
            
            # Test 1: Get system status
            print("\n1️⃣ Testing system status...")
            ser.write(b"STATUS\r\n")
            time.sleep(1)
            
            response = ""
            while ser.in_waiting > 0:
                line = ser.readline().decode('utf-8', errors='ignore').strip()
                if line:
                    print(f"   📥 {line}")
                    response += line + "\n"
            
            # Test 2: Test USB hardware
            print("\n2️⃣ Testing USB hardware...")
            ser.write(b"USB_HARDWARE_TEST\r\n")
            time.sleep(2)
            
            while ser.in_waiting > 0:
                line = ser.readline().decode('utf-8', errors='ignore').strip()
                if line:
                    print(f"   📥 {line}")
            
            # Test 3: Try to start USB MSC
            print("\n3️⃣ Attempting to start USB MSC...")
            ser.write(b"USB_MSC_START\r\n")
            time.sleep(3)
            
            msc_started = False
            while ser.in_waiting > 0:
                line = ser.readline().decode('utf-8', errors='ignore').strip()
                if line:
                    print(f"   📥 {line}")
                    if "STARTED" in line:
                        msc_started = True
                    elif "ERROR" in line or "FAIL" in line:
                        print(f"   ❌ Error detected: {line}")
            
            # Test 4: Check status after MSC start
            print("\n4️⃣ Checking status after MSC start...")
            ser.write(b"STATUS\r\n")
            time.sleep(1)
            
            while ser.in_waiting > 0:
                line = ser.readline().decode('utf-8', errors='ignore').strip()
                if line:
                    print(f"   📥 {line}")
            
            # Test 5: Monitor for a few seconds
            print("\n5️⃣ Monitoring system for 10 seconds...")
            print("   (Look for any error messages or USB events)")
            
            start_time = time.time()
            while time.time() - start_time < 10:
                if ser.in_waiting > 0:
                    line = ser.readline().decode('utf-8', errors='ignore').strip()
                    if line:
                        print(f"   📥 {line}")
                time.sleep(0.1)
            
            print("\n" + "=" * 60)
            print("🔧 Troubleshooting Summary:")
            print("=" * 60)
            
            if msc_started:
                print("✅ USB MSC appears to have started")
                print("💡 Check if LEFT port is connected to computer")
                print("💡 Try different USB cable or USB port on computer")
                print("💡 Check Device Manager (Windows) for new devices")
                print("💡 Look for BLUE LED on ESP32-S3")
            else:
                print("❌ USB MSC failed to start")
                print("💡 Check USB-OTG bridge soldering (GPIO19/20)")
                print("💡 Verify SD card is properly connected and working")
                print("💡 Check system logs above for specific error messages")
            
            print("\n🔧 Next steps:")
            print("1. Connect LEFT port to computer (not RIGHT port)")
            print("2. Check Windows Device Manager for new USB devices")
            print("3. Try 'USB_MSC_STOP' then 'USB_MSC_START' again")
            print("4. Verify SD card has data files")
            
    except serial.SerialException as e:
        print(f"❌ Serial error: {e}")
        return False
    except KeyboardInterrupt:
        print("\n⚠️  Debug session cancelled by user")
        return False

def main():
    print("🚀 ProxiTof USB MSC Debug Tool")
    print("=" * 60)
    
    # Auto-detect port
    port = find_esp32_port()
    if not port:
        port = input("Enter COM port manually (e.g., COM4): ").strip()
        if not port:
            print("❌ No port specified")
            sys.exit(1)
    
    # Run debug session
    debug_usb_msc(port)

if __name__ == "__main__":
    main()