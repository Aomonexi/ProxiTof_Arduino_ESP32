#!/usr/bin/env python3
"""
ProxiTof USB Configuration Tool

Easy interface to configure ProxiTof device via USB
Provides GUI and command-line access to device settings
"""

import serial
import serial.tools.list_ports
import time
import datetime
import argparse
import sys
from typing import Optional

class ProxiTofUSB:
    def __init__(self, port: Optional[str] = None):
        self.port = port
        self.serial = None
        
    def find_device(self) -> Optional[str]:
        """Find ProxiTof device automatically"""
        ports = serial.tools.list_ports.comports()
        
        for port in ports:
            # Look for ESP32-S3 USB Serial JTAG
            if "USB Serial" in port.description or "ESP32" in port.description:
                print(f"Found potential ProxiTof device: {port.device}")
                return port.device
                
        return None
    
    def connect(self) -> bool:
        """Connect to ProxiTof device"""
        if not self.port:
            self.port = self.find_device()
            
        if not self.port:
            print("❌ No ProxiTof device found")
            print("💡 Make sure device is connected via USB cable")
            return False
            
        try:
            self.serial = serial.Serial(self.port, 115200, timeout=2)
            time.sleep(0.5)  # Wait for connection
            
            # Test connection
            self.send_command("status")
            response = self.read_response()
            
            if "System Status" in response:
                print(f"✓ Connected to ProxiTof device on {self.port}")
                return True
            else:
                print("❌ Device found but not responding correctly")
                return False
                
        except Exception as e:
            print(f"❌ Failed to connect: {e}")
            return False
    
    def disconnect(self):
        """Disconnect from device"""
        if self.serial:
            self.serial.close()
            self.serial = None
    
    def send_command(self, command: str):
        """Send command to device"""
        if not self.serial:
            raise Exception("Not connected to device")
            
        self.serial.write(f"{command}\n".encode())
        self.serial.flush()
    
    def read_response(self, timeout: float = 2.0) -> str:
        """Read response from device"""
        if not self.serial:
            raise Exception("Not connected to device")
            
        response = ""
        start_time = time.time()
        
        while time.time() - start_time < timeout:
            if self.serial.in_waiting > 0:
                data = self.serial.read(self.serial.in_waiting).decode('utf-8', errors='ignore')
                response += data
                
            time.sleep(0.1)
            
        return response
    
    def get_status(self) -> dict:
        """Get device status"""
        self.send_command("status")
        response = self.read_response()
        
        # Parse status response (simplified)
        status = {}
        for line in response.split('\n'):
            if ':' in line:
                key, value = line.split(':', 1)
                status[key.strip()] = value.strip()
                
        return status
    
    def set_wifi(self, ssid: str, password: str) -> bool:
        """Set WiFi credentials"""
        self.send_command(f"config wifi {ssid} {password}")
        response = self.read_response()
        return "✓" in response
    
    def set_timezone(self, timezone: str) -> bool:
        """Set timezone"""
        self.send_command(f"config timezone {timezone}")
        response = self.read_response()
        return "✓" in response
    
    def set_time(self, dt: datetime.datetime) -> bool:
        """Set device time"""
        time_str = dt.strftime("%Y-%m-%d %H:%M:%S")
        self.send_command(f"time {time_str}")
        response = self.read_response()
        return "✓" in response
    
    def sync_time(self) -> bool:
        """Sync time with computer"""
        current_time = datetime.datetime.now()
        return self.set_time(current_time)
    
    def get_help(self) -> str:
        """Get device help"""
        self.send_command("help")
        return self.read_response()

def main():
    parser = argparse.ArgumentParser(description="ProxiTof USB Configuration Tool")
    parser.add_argument("--port", help="Serial port (auto-detect if not specified)")
    parser.add_argument("--wifi", nargs=2, metavar=("SSID", "PASSWORD"), help="Set WiFi credentials")
    parser.add_argument("--timezone", help="Set timezone (e.g., UTC0, EST5EDT)")
    parser.add_argument("--sync-time", action="store_true", help="Sync time with computer")
    parser.add_argument("--status", action="store_true", help="Show device status")
    parser.add_argument("--interactive", action="store_true", help="Interactive mode")
    
    args = parser.parse_args()
    
    # Create device interface
    device = ProxiTofUSB(args.port)
    
    if not device.connect():
        sys.exit(1)
    
    try:
        if args.status:
            print("\n=== Device Status ===")
            status = device.get_status()
            for key, value in status.items():
                print(f"{key}: {value}")
                
        if args.wifi:
            ssid, password = args.wifi
            print(f"Setting WiFi: {ssid}")
            if device.set_wifi(ssid, password):
                print("✓ WiFi credentials updated")
            else:
                print("❌ Failed to update WiFi credentials")
                
        if args.timezone:
            print(f"Setting timezone: {args.timezone}")
            if device.set_timezone(args.timezone):
                print("✓ Timezone updated")
            else:
                print("❌ Failed to update timezone")
                
        if args.sync_time:
            print("Syncing time with computer...")
            if device.sync_time():
                print("✓ Time synchronized")
            else:
                print("❌ Failed to sync time")
                
        if args.interactive:
            print("\n=== Interactive Mode ===")
            print("Type commands or 'quit' to exit")
            print("Available commands: status, help, config, time, sync")
            
            while True:
                try:
                    command = input("ProxiTof> ").strip()
                    if command.lower() in ['quit', 'exit']:
                        break
                    elif command:
                        device.send_command(command)
                        response = device.read_response()
                        print(response)
                except KeyboardInterrupt:
                    break
                    
        if not any([args.status, args.wifi, args.timezone, args.sync_time, args.interactive]):
            # Default: show status
            print("\n=== ProxiTof Device Status ===")
            status = device.get_status()
            for key, value in status.items():
                print(f"{key}: {value}")
            print("\nUse --help for more options")
            
    finally:
        device.disconnect()

if __name__ == "__main__":
    main()