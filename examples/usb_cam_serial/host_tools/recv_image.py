import serial
import struct
import time
import sys
import argparse
import os

def receive_image(ser):
    """Receive a single image from the serial port"""
    # Magic marker to look for: 0xBE, 0xEF, 0xCA, 0xFE
    MARKER = b'\xBE\xEF\xCA\xFE'
    buffer = b''
    
    # Wait for marker
    start_time = time.time()
    while True:
        if time.time() - start_time > 30: # 30s timeout
            print("Error: Timeout waiting for image start marker")
            return None
            
        byte = ser.read(1)
        if len(byte) == 0:
            continue # Timeout, retry
            
        buffer += byte
        if buffer.endswith(MARKER):
            # Print logs
            logs = buffer[:-4]
            if logs:
                 try:
                    print(logs.decode('utf-8', errors='ignore'), end='', flush=True)
                 except: pass
            break
        
        # Buffer text cleanup
        if len(buffer) > 1000:
             # Find last newline
             last_nl = buffer.rfind(b'\n')
             if last_nl != -1:
                 to_print = buffer[:last_nl+1]
                 try:
                    print(to_print.decode('utf-8', errors='ignore'), end='', flush=True)
                 except: pass
                 buffer = buffer[last_nl+1:]

    # Read size (4 bytes, Little Endian)
    size_data = ser.read(4)
    if len(size_data) != 4:
        print(f"Error: Failed to read size (got {len(size_data)} bytes)")
        return None

    image_size = struct.unpack('<I', size_data)[0]
    print(f"  Image size: {image_size} bytes")

    if not (0 < image_size < 10 * 1024 * 1024):  # Sanity check (10MB)
        print(f"  Error: Invalid image size: {image_size}")
        return None

    # Read image data using readinto for efficiency
    print("  Receiving image data...", end='', flush=True)
    buf = bytearray(image_size)
    view = memoryview(buf)
    
    # Increase timeout for large data transfer
    old_timeout = ser.timeout
    ser.timeout = 10
    
    bytes_read = 0
    CHUNK_SIZE = 32768  # 32KB chunks
    failed = False

    while bytes_read < image_size:
        chunk_size = min(CHUNK_SIZE, image_size - bytes_read)
        n = ser.readinto(view[bytes_read:bytes_read + chunk_size])
        
        if not n:  # None or 0 means timeout
            print(f"\n  Error: Timeout receiving data at {bytes_read}/{image_size} bytes")
            failed = True
            break
            
        bytes_read += n
    
    ser.timeout = old_timeout  # Restore timeout

    if not failed and bytes_read == image_size:
        print(" Done!")
        return bytes(buf)
    else:
        print(f"\n  Transfer failed: received {bytes_read}/{image_size} bytes")
        return None

def receive_image_loop(port, baudrate, output_base):
    try:
        ser = serial.Serial(port, baudrate, timeout=5)
        print(f"Opened {port} at {baudrate} baud")
    except serial.SerialException as e:
        print(f"Error opening serial port: {e}")
        return

    # Wait for board to boot up / stabilize
    print("Waiting for board initialization (2s)...")
    time.sleep(2)
    ser.reset_input_buffer()

    image_count = 1
    base_name, ext = os.path.splitext(output_base)
    if not ext:
        ext = ".jpg"

    print("\nCommands:")
    print("  's' - Start automatic rotation and capture sequence")
    print("  'c' - Capture single image (no motor movement)")
    print("  'q' - Quit")
    print("\nReady to capture.")
    
    try:
        while True:
            try:
                cmd = input(f"\nEnter command (s/c/q): ").strip().lower()
            except EOFError:
                break
            
            if cmd == 'q':
                print("Sending quit command...")
                ser.write(b'q')
                break
            elif cmd == 's':
                print("\n=== Starting Automatic Sequence ===")
                print("Sending start command...")
                ser.reset_input_buffer()
                ser.write(b's')
                
                # Receive multiple images (16 positions for ±180°)
                num_images = 16
                for i in range(num_images):
                    print(f"\nWaiting for image {i+1}/{num_images}...")
                    
                    image_data = receive_image(ser)
                    if image_data:
                        output_filename = f"{base_name}_{image_count:03d}{ext}"
                        with open(output_filename, 'wb') as f:
                            f.write(image_data)
                        print(f"  Saved to {output_filename}")
                        image_count += 1
                    else:
                        print(f"  Failed to receive image {i+1}")
                        # Continue trying to receive remaining images
                
                print("\n=== Sequence Complete ===")
                
            elif cmd == 'c':
                print("\nSending capture command...")
                ser.reset_input_buffer()
                ser.write(b'c')
                
                image_data = receive_image(ser)
                if image_data:
                    output_filename = f"{base_name}_{image_count:03d}{ext}"
                    with open(output_filename, 'wb') as f:
                        f.write(image_data)
                    print(f"Image saved to {output_filename}")
                    image_count += 1
            else:
                print("Invalid command. Use 's', 'c', or 'q'.")

    except KeyboardInterrupt:
        print("\nExiting...")
        ser.write(b'q')
    finally:
        ser.close()

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Receive JPEG images from Spresense via Serial')
    parser.add_argument('-p', '--port', required=True, help='Serial port (e.g., COM3 or /dev/ttyUSB0)')
    parser.add_argument('-b', '--baud', type=int, default=115200, help='Baud rate (default: 115200)')
    parser.add_argument('-o', '--output', default='image.jpg', help='Output file base name (default: image.jpg)')

    args = parser.parse_args()

    receive_image_loop(args.port, args.baud, args.output)
