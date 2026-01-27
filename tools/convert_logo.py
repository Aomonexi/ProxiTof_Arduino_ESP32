#!/usr/bin/env python3
"""
Convert horizontal bitmap to SSD1306 vertical page format
Input: 128x64 bitmap in horizontal byte format (16 bytes per row, 64 rows)
Output: 128x64 bitmap in vertical page format (128 bytes per page, 8 pages)
"""

# Read the logo file
with open('Proxitof_logo.txt', 'r') as f:
    lines = f.readlines()

# Parse hex bytes from file (skip first line which is comment)
horizontal_bytes = []
for line in lines[1:]:
    # Remove whitespace and split by comma
    line = line.strip().rstrip(',')
    if line:
        bytes_in_line = [int(b.strip(), 16) for b in line.split(',') if b.strip()]
        horizontal_bytes.extend(bytes_in_line)

print(f"Read {len(horizontal_bytes)} bytes from logo file")
print(f"Expected: {128 * 64 // 8} bytes for 128x64 image")

# Convert to vertical page format
# SSD1306 uses 8 pages of 128 bytes each
# Each byte represents 8 vertical pixels
vertical_pages = [[0 for _ in range(128)] for _ in range(8)]

for row in range(64):
    for col in range(128):
        # Get bit from horizontal format
        byte_index = row * 16 + col // 8
        bit_index = 7 - (col % 8)  # MSB first
        
        if byte_index < len(horizontal_bytes):
            bit_value = (horizontal_bytes[byte_index] >> bit_index) & 1
            
            # Set bit in vertical format
            page = row // 8
            bit_in_page = row % 8
            if bit_value:
                vertical_pages[page][col] |= (1 << bit_in_page)

# Output C array
print("\n// Proxitof Logo - 128x64 pixels in SSD1306 vertical page format")
print("// 8 pages × 128 bytes = 1024 bytes")
print("static const uint8_t proxitof_logo_ssd1306[1024] = {")

for page in range(8):
    for i in range(0, 128, 16):
        bytes_str = ", ".join(f"0x{b:02x}" for b in vertical_pages[page][i:i+16])
        print(f"    {bytes_str},")

print("};")
