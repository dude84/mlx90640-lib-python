# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is a Python wrapper for the Melexis MLX90640 thermal camera sensor, designed for Raspberry Pi. It includes a minimal C++ library for low-level I2C communication with the sensor and a high-level Python API for capturing and processing 24x32 thermal images.

## Directory Structure

```
/
├── mlx90640/                     # Python package (all sensor code)
│   ├── lib/                      # C++ library sources (4 files)
│   │   ├── MLX90640_API.cpp
│   │   ├── MLX90640_API.h
│   │   ├── MLX90640_LINUX_I2C_Driver.cpp
│   │   └── MLX90640_I2C_Driver.h
│   ├── __init__.py
│   ├── camera.cpp                # pybind11 wrapper
│   └── camera.h
├── examples/                     # Python examples
│   ├── ascii_display.py
│   ├── configure_params.py
│   └── simple_capture.py
├── Makefile                      # Unified build system
├── setup.py                      # Python package config
├── pyproject.toml                # Build configuration
├── MANIFEST.in                   # Package manifest
├── README.md
├── LICENSE
├── CLAUDE.md
└── .gitignore
```

## Build System

The project uses a single unified Makefile that:
1. Builds the C++ library from lib/
2. Creates Python virtual environment
3. Installs Python wrapper in development mode

### Build Commands

```bash
# Build everything (C++ library + Python wrapper)
make all

# Debug build
make all DEBUG=1

# Build C++ library only
make libMLX90640_API.so libMLX90640_API.a

# Clean all build artifacts
make clean

# Install C++ library system-wide (optional)
sudo make lib-install
```

### Quick Start

```bash
# Build and install
make all

# Activate virtual environment
source venv/bin/activate

# Run examples
python examples/ascii_display.py
python examples/simple_capture.py
```

## Architecture

### C++ Library (mlx90640/lib/)

The C++ library is minimal and focused on sensor communication:

1. **MLX90640_API.cpp/h**: Core sensor API from Melexis
   - EEPROM reading (`MLX90640_DumpEE`)
   - Parameter extraction (`MLX90640_ExtractParameters`)
   - Temperature calculation (`MLX90640_CalculateTo`)
   - Frame data acquisition (`MLX90640_GetFrameData`)

2. **MLX90640_LINUX_I2C_Driver.cpp/h**: Linux I2C implementation
   - Uses standard `/dev/i2c-*` devices
   - No root access required
   - No bcm2835 dependency

### Python Wrapper (mlx90640/)

The Python wrapper uses pybind11 to expose a high-level API:

- **camera.cpp/h**: C++ implementation of MLX90640Camera class
- **__init__.py**: Python package interface

Key features:
- Chess mode only (sensor's calibrated mode)
- Self-pacing frame capture (blocking calls)
- Zero-copy NumPy arrays
- Configurable refresh rate, resolution, emissivity

### Key Data Flow

1. **Sensor Initialization**:
   - Read EEPROM (832 words) via `MLX90640_DumpEE()`
   - Extract calibration parameters into `paramsMLX90640` struct
   - Configure refresh rate and chess mode

2. **Frame Acquisition**:
   - `get_frame()` blocks until sensor has new data
   - Read frame data (834 words) via `MLX90640_GetFrameData()`
   - Optionally interpolate outliers and correct bad pixels
   - Calculate temperatures and return as NumPy array (768 floats)

3. **Measurement Mode**:
   - **Chess mode only**: Alternates pixels in checkerboard pattern (subpages 0 and 1)
   - This is the mode the sensor is calibrated for
   - Provides best accuracy

## I2C Configuration

The sensor's I2C baudrate directly affects achievable framerate:

- **400 kHz**: Max ~8 FPS (compatible with other I2C devices)
- **1 MHz**: Max ~32 FPS (if MLX90640 is the only high-speed device)

Configure in `/boot/firmware/config.txt` (or `/boot/config.txt` on older systems):
```
dtparam=i2c_arm=on
dtparam=i2c_arm_baudrate=1000000
```

### I2C Bus Separation

The Raspberry Pi has multiple I2C buses:
- **i2c-1 (GPIO I2C)**: Controlled by `i2c_arm_baudrate`, exposed on GPIO pins 2/3
- **i2c-0/i2c-10 (Camera I2C)**: Controlled by `i2c_vc_baudrate`, used by CSI camera (e.g., imx219)

The MLX90640 connects to **i2c-1** (GPIO), so setting `i2c_arm_baudrate=1000000` only affects the thermal sensor, not the camera. The two buses can be configured independently:

```
# MLX90640 on i2c-1 (GPIO pins) at 1MHz for high frame rates
dtparam=i2c_arm=on
dtparam=i2c_arm_baudrate=1000000

# imx219 camera on i2c-0/i2c-10 at default 100kHz (optional, explicit config)
dtparam=i2c_vc=on
dtparam=i2c_vc_baudrate=100000
```

This allows using 1MHz for the MLX90640 without affecting other I2C devices.

Valid refresh rates: 1, 2, 4, 8, 16, 32, 64 Hz

## Sensor Constants

- **I2C Address**: 0x33 (fixed)
- **Image Dimensions**: 24x32 pixels (768 total)
- **EEPROM Size**: 832 words (calibration data)
- **Frame Data Size**: 834 words (includes status registers)

## Common Development Patterns

### Adding a Python Example

1. Create new file in `examples/`
2. Import mlx90640 package
3. Initialize camera, configure, and capture frames
4. See existing examples for patterns

Example structure:
```python
import mlx90640

camera = mlx90640.MLX90640Camera()
camera.init()
camera.set_refresh_rate(16)
camera.set_emissivity(0.95)

frame = camera.get_frame()  # NumPy array of 768 floats
# Process frame...

camera.cleanup()
```

### Modifying the C++ Wrapper

1. Edit `mlx90640/camera.cpp` or `mlx90640/camera.h`
2. Rebuild with `make build`
3. Test changes (wrapper is installed in development mode)

### Modifying the C++ Library

1. Edit files in `mlx90640/lib/`
2. Rebuild with `make clean && make all`
3. The Python wrapper will link against the updated library

## Dependencies

### System Dependencies
- **libi2c-dev**: Required for I2C communication
- **Python 3.6+**: For Python wrapper
- **g++**: C++ compiler

### Python Dependencies (auto-installed via setup.py)
- **pybind11>=2.6.0**: C++/Python bindings
- **numpy>=1.19.0**: Zero-copy array interface

## Build Modes

- **Release (default)**: Optimized with `-O3 -march=native`
- **Debug**: Built with `-g` for debugging (use `make all DEBUG=1`)

## Performance Notes

- **Achievable Frame Rate**: ~15.8 FPS at 16 Hz refresh rate
- **Zero-Copy Design**: NumPy arrays wrap C++ buffer (no copying)
- **I2C Speed**: 1MHz required for rates ≥16 Hz
- **Resolution vs Speed**: Higher resolution = better accuracy but slower

## Deployment

The Python wrapper can be deployed as:
1. **Wheel package**: `make wheel` creates distributable .whl file
2. **Source**: Copy entire project and run `make all`
3. **System-wide**: Install C++ library with `sudo make lib-install`, then pip install wheel

See README.md for detailed deployment instructions.
