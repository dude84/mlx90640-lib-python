#!/usr/bin/env python3
"""
Simple MLX90640 Frame Capture Example

Demonstrates basic usage:
- Initialize camera
- Configure parameters
- Capture single frame
- Display statistics
"""

import mlx90640
import sys


def main():
    """Capture and display a single thermal frame"""

    # Initialize camera
    print("Initializing MLX90640 camera...")
    camera = mlx90640.MLX90640Camera()

    try:
        camera.init()
    except Exception as e:
        print(f"ERROR: Failed to initialize camera: {e}")
        print("\nTroubleshooting:")
        print("1. Check I2C connection (i2cdetect -y 1)")
        print("2. Verify MLX90640 library is installed (ls /usr/local/lib/libMLX90640_API.so)")
        print("3. Check I2C permissions (add user to i2c group)")
        sys.exit(1)

    print("Camera initialized successfully!")

    # Configure camera
    print("\nConfiguring camera...")
    camera.set_refresh_rate(16)      # 16 FPS
    camera.set_emissivity(0.95)      # For human skin
    camera.set_resolution(3)         # 19-bit (highest quality)

    # Display configuration
    print(f"  Refresh rate: {camera.get_refresh_rate()} Hz")
    print(f"  Resolution: {camera.get_resolution()} (19-bit)")
    print(f"  Emissivity: {camera.get_emissivity()}")

    # Capture frame
    print("\nCapturing frame (this may take a moment)...")
    frame = camera.get_frame(
        interpolate_outliers=True,
        correct_bad_pixels=True
    )

    # Display statistics
    print(f"\nFrame Statistics:")
    print(f"  Pixels: {len(frame)}")
    print(f"  Minimum: {min(frame):.2f}°C")
    print(f"  Maximum: {max(frame):.2f}°C")
    print(f"  Average: {sum(frame)/len(frame):.2f}°C")
    print(f"  Range: {max(frame) - min(frame):.2f}°C")

    # Find hottest pixel
    max_temp = max(frame)
    max_idx = frame.index(max_temp)
    max_row = max_idx // 32
    max_col = max_idx % 32
    print(f"\nHottest pixel:")
    print(f"  Temperature: {max_temp:.2f}°C")
    print(f"  Position: row {max_row}, col {max_col}")

    # Find coldest pixel
    min_temp = min(frame)
    min_idx = frame.index(min_temp)
    min_row = min_idx // 32
    min_col = min_idx % 32
    print(f"\nColdest pixel:")
    print(f"  Temperature: {min_temp:.2f}°C")
    print(f"  Position: row {min_row}, col {min_col}")

    # Cleanup
    camera.cleanup()
    print("\nDone!")


if __name__ == "__main__":
    main()
