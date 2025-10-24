#!/usr/bin/env python3
"""
MLX90640 Configuration Parameters Demo

Demonstrates all configurable parameters:
- Refresh rate (1, 2, 4, 8, 16, 32, 64 Hz)
- Resolution (16, 17, 18, 19 bit)
- Emissivity (0.1 - 1.0)

Shows how to change parameters at runtime and query current settings.
"""

import mlx90640
import sys
import time


def print_config(camera, name):
    """Print current camera configuration"""
    print(f"\n{name}")
    print(f"  Refresh rate: {camera.get_refresh_rate()} Hz")
    print(f"  Resolution: {camera.get_resolution()} ({'16,17,18,19'[camera.get_resolution()]} bit)")
    print(f"  Emissivity: {camera.get_emissivity()}")


def capture_frames(camera, count=5):
    """Capture and display stats for multiple frames"""
    temps = []

    for i in range(count):
        frame = camera.get_frame()
        min_temp = frame.min()
        max_temp = frame.max()
        avg_temp = frame.mean()
        temps.append((min_temp, max_temp, avg_temp))
        print(f"    Frame {i+1}: {min_temp:5.2f}°C - {max_temp:5.2f}°C (avg: {avg_temp:5.2f}°C)")

    # Calculate overall statistics
    all_mins = [t[0] for t in temps]
    all_maxs = [t[1] for t in temps]
    all_avgs = [t[2] for t in temps]

    print(f"  Overall: {min(all_mins):5.2f}°C - {max(all_maxs):5.2f}°C (avg: {sum(all_avgs)/len(all_avgs):5.2f}°C)")


def main():
    """Demo all configuration parameters"""

    # Initialize camera
    print("Initializing MLX90640 camera...")
    camera = mlx90640.MLX90640Camera(0x33)

    try:
        camera.init()
    except Exception as e:
        print(f"ERROR: Failed to initialize: {e}")
        sys.exit(1)

    print("Initialized successfully!")

    # Configuration test cases
    configs = [
        {
            "name": "Config 1: High Speed, Lower Quality",
            "fps": 32,
            "res": 1,      # 17-bit
            "emis": 1.0,   # Blackbody
            "desc": "Fast refresh for motion tracking"
        },
        {
            "name": "Config 2: Balanced",
            "fps": 16,
            "res": 2,      # 18-bit
            "emis": 0.95,  # Human skin
            "desc": "Good balance of speed and quality"
        },
        {
            "name": "Config 3: High Quality, Slower",
            "fps": 8,
            "res": 3,      # 19-bit
            "emis": 0.90,  # Matte surfaces
            "desc": "Best accuracy for static scenes"
        },
        {
            "name": "Config 4: Maximum Speed",
            "fps": 64,
            "res": 0,      # 16-bit
            "emis": 1.0,
            "desc": "Fastest possible (requires 1MHz I2C)"
        },
    ]

    # Test each configuration
    for config in configs:
        print(f"\n{'='*60}")
        print(f"{config['name']}")
        print(f"Description: {config['desc']}")
        print(f"{'='*60}")

        # Apply configuration
        print(f"\nApplying settings...")
        try:
            camera.set_refresh_rate(config["fps"])
            camera.set_resolution(config["res"])
            camera.set_emissivity(config["emis"])
        except Exception as e:
            print(f"  ERROR: {e}")
            continue

        # Verify settings
        print_config(camera, "Current settings:")

        # Capture frames
        print(f"\nCapturing 5 frames...")
        capture_frames(camera, count=5)

        # Small delay between configs
        time.sleep(0.5)

    # Test parameter validation
    print(f"\n{'='*60}")
    print("Testing Parameter Validation")
    print(f"{'='*60}")

    print("\n1. Testing invalid refresh rate (should fail):")
    try:
        camera.set_refresh_rate(99)
        print("  ERROR: Should have raised exception!")
    except ValueError as e:
        print(f"  ✓ Correctly rejected: {e}")

    print("\n2. Testing invalid resolution (should fail):")
    try:
        camera.set_resolution(5)
        print("  ERROR: Should have raised exception!")
    except ValueError as e:
        print(f"  ✓ Correctly rejected: {e}")

    print("\n3. Testing invalid emissivity (should fail):")
    try:
        camera.set_emissivity(1.5)
        print("  ERROR: Should have raised exception!")
    except ValueError as e:
        print(f"  ✓ Correctly rejected: {e}")

    print("\n4. Testing boundary values (should succeed):")
    try:
        camera.set_refresh_rate(1)    # Minimum FPS
        camera.set_resolution(0)       # Minimum resolution
        camera.set_emissivity(0.1)     # Minimum emissivity
        print("  ✓ Minimum values accepted")

        camera.set_refresh_rate(64)    # Maximum FPS
        camera.set_resolution(3)       # Maximum resolution
        camera.set_emissivity(1.0)     # Maximum emissivity
        print("  ✓ Maximum values accepted")
    except Exception as e:
        print(f"  ERROR: {e}")

    # Cleanup
    camera.cleanup()
    print(f"\n{'='*60}")
    print("Configuration demo complete!")
    print(f"{'='*60}\n")


if __name__ == "__main__":
    main()
