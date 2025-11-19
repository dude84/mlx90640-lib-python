#!/usr/bin/env python3
"""
Real-time ASCII Thermal Display with Inferno Colormap

Displays thermal camera output in the terminal with ANSI 24-bit color.
Similar to the C++ test.cpp example but in Python.

Features:
- Inferno colormap (perceptually uniform)
- FPS counter
- Temperature statistics
- Self-paced frame capture (blocking until sensor ready)

Controls:
- Ctrl+C to exit
"""

import mlx90640
import time
import sys


# Configuration
TEMP_MIN = 15.0  # Minimum temperature for colormap (°C)
TEMP_MAX = 35.0  # Maximum temperature for colormap (°C)
SCALE = 1        # Horizontal scaling (1-4, larger = wider display)
BLOCK = "██"     # Unicode block character for pixels


def inferno_colormap(value):
    """
    Convert normalized value [0.0-1.0] to RGB using Inferno colormap

    Args:
        value: Normalized value between 0.0 (coldest) and 1.0 (hottest)

    Returns:
        Tuple of (r, g, b) integers in range [0-255]
    """
    # Clamp value to [0, 1]
    value = max(0.0, min(1.0, value))

    # Inferno colormap control points
    colors = [
        (0.001462, 0.000466, 0.013866),  # 0.0   - dark purple/black
        (0.087411, 0.044556, 0.224813),  # 0.125 - deep purple
        (0.258234, 0.038571, 0.406485),  # 0.25  - purple
        (0.416331, 0.090203, 0.432943),  # 0.375 - purple-red
        (0.645581, 0.133503, 0.392508),  # 0.5   - red
        (0.798216, 0.280197, 0.469538),  # 0.625 - orange-red
        (0.924870, 0.517763, 0.295662),  # 0.75  - orange
        (0.987622, 0.809330, 0.145357),  # 0.875 - yellow-orange
        (0.988362, 0.998364, 0.644924)   # 1.0   - bright yellow
    ]

    # Interpolate between control points
    scaled = value * (len(colors) - 1)
    idx1 = int(scaled)
    idx2 = min(idx1 + 1, len(colors) - 1)
    frac = scaled - idx1

    # Linear interpolation
    r = colors[idx1][0] + (colors[idx2][0] - colors[idx1][0]) * frac
    g = colors[idx1][1] + (colors[idx2][1] - colors[idx1][1]) * frac
    b = colors[idx1][2] + (colors[idx2][2] - colors[idx1][2]) * frac

    # Convert to [0-255] range
    return (int(r * 255), int(g * 255), int(b * 255))


def main():
    """Run real-time ASCII thermal display"""

    # Initialize camera
    print("Initializing MLX90640 camera...")
    camera = mlx90640.MLX90640Camera()

    try:
        camera.init()
    except Exception as e:
        print(f"Failed to initialize camera: {e}")
        print("\nCheck:")
        print("  - I2C connection (i2cdetect -y 1)")
        print("  - MLX90640 library installed")
        print("  - I2C permissions")
        sys.exit(1)

    # Configure
    camera.set_refresh_rate(16)
    camera.set_resolution(3)
    camera.set_emissivity(1.0)
    camera.set_rotation(0)

    # Display header
    print(f"\nMLX90640 Real-time ASCII Display")
    print(f"Refresh rate: 16 Hz")
    print(f"Temperature range: {TEMP_MIN}°C - {TEMP_MAX}°C")
    print(f"Resolution: 24x32 pixels")
    print(f"Press Ctrl+C to exit\n")

    # Wait a moment before starting
    time.sleep(1)

    frame_count = 0
    start_time = time.time()

    try:
        while True:
            # Get frame (blocking, self-paced by sensor)
            frame = camera.get_frame()

            # Calculate FPS
            frame_count += 1
            elapsed = time.time() - start_time
            fps = frame_count / elapsed if elapsed > 0 else 0

            # Calculate statistics
            min_temp = frame.min()
            max_temp = frame.max()
            avg_temp = frame.mean()

            # Print header with stats
            print(f"FPS: {fps:5.2f} | Min: {min_temp:5.2f}°C | Max: {max_temp:5.2f}°C | Avg: {avg_temp:5.2f}°C")

            # Print thermal image with Inferno colormap
            for y in range(24):
                for x in range(32):
                    # Get temperature (flip Y axis to match test.cpp orientation)
                    temp = frame[32 * (23 - y) + x]

                    # Normalize to [0, 1] for colormap
                    normalized = (temp - TEMP_MIN) / (TEMP_MAX - TEMP_MIN)

                    # Get RGB from Inferno colormap
                    r, g, b = inferno_colormap(normalized)

                    # Print with ANSI 24-bit true color
                    # ESC[38;2;R;G;Bm sets foreground color
                    print(f"\x1b[38;2;{r};{g};{b}m", end="")

                    # Print character(s) for this pixel
                    for _ in range(SCALE):
                        print(BLOCK, end="")

                    # Reset color
                    print("\x1b[0m", end="")

                # End of row
                print()

            # Move cursor up to overwrite previous frame
            # 24 lines of image + 1 line of header = 25 lines total
            print("\x1b[25A", end="", flush=True)

            # Reset FPS counter periodically to get recent average
            if frame_count >= 100:
                frame_count = 0
                start_time = time.time()

    except KeyboardInterrupt:
        # Clear the display area
        print("\n" * 25)
        print("Stopped")

    except Exception as e:
        # Clear the display area
        print("\n" * 25)
        print(f"Error: {e}")
        sys.exit(1)

    finally:
        camera.cleanup()


if __name__ == "__main__":
    main()
