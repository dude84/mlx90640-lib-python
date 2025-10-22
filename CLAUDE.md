# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is a C++ library and Python wrapper for the Melexis MLX90640 thermal camera sensor, designed for Raspberry Pi. The library provides low-level I2C communication with the sensor and high-level APIs for capturing and processing 24x32 thermal images.

## Build System and I2C Modes

The library supports two I2C driver modes, controlled by the `I2C_MODE` variable:

- **LINUX mode** (default): Uses standard Linux I2C drivers (`/dev/i2c-*`), no root access required
- **RPI mode**: Uses bcm2835 library for direct hardware access, requires root

### Build Commands

```bash
# Build library and all examples (LINUX mode)
make clean
make I2C_MODE=LINUX

# Build library and all examples (RPI mode with bcm2835)
make clean
make I2C_MODE=RPI

# Build specific example
make examples/test
make examples/rawrgb

# Install library system-wide
sudo make install  # Installs to /usr/local by default
```

### Python Bindings

The Python bindings depend on the main library being installed system-wide:

```bash
# First, install the main library
make
sudo make install

# Then build/install Python bindings
cd python/library
make build    # Build for default Python 3
make install  # Install for default Python 3

# For specific Python version
make PYTHON=/path/to/python build
```

## Architecture

### Core Components

1. **functions/**: Melexis API implementation and I2C driver abstractions
   - `MLX90640_API.cpp`: Core sensor API from Melexis (EEPROM reading, parameter extraction, temperature calculation)
   - `MLX90640_LINUX_I2C_Driver.cpp`: Linux I2C implementation
   - `MLX90640_RPI_I2C_Driver.cpp`: bcm2835-based implementation
   - The I2C driver is selected at compile time via `I2C_MODE`

2. **headers/**: Public API headers
   - `MLX90640_API.h`: Main sensor API (paramsMLX90640 struct, temperature calculation functions)
   - `MLX90640_I2C_Driver.h`: I2C abstraction layer

3. **examples/src/**: Demonstration programs
   - `test.cpp`: Console output with ANSI colors
   - `rawrgb.cpp`: Outputs raw RGB stream to stdout (for GStreamer pipelines)
   - `fbuf.cpp`: Direct framebuffer rendering
   - `interp.cpp`: Framebuffer with 2x bicubic interpolation
   - `sdlscale.cpp`: SDL2 hardware-accelerated display
   - `video.cpp`: LibAV video encoding
   - `hotspot.cpp`: Hotspot detection
   - `mlx2bin.cpp`: Binary data output
   - `lib/fb.c`: Framebuffer utilities
   - `lib/interpolate.c`: Bicubic interpolation

4. **python/**: Python bindings and examples
   - `library/`: SWIG-based Python wrapper
   - `rgb-to-gif.py`: Example using subprocess to call rawrgb

### Key Data Flow

1. **Sensor Initialization**:
   - Read EEPROM (832 words) via `MLX90640_DumpEE()`
   - Extract calibration parameters into `paramsMLX90640` struct via `MLX90640_ExtractParameters()`
   - Configure refresh rate, resolution, and measurement mode

2. **Frame Acquisition**:
   - Read frame data (834 words) via `MLX90640_GetFrameData()`
   - Interpolate outlier pixels via `MLX90640_InterpolateOutliers()`
   - Calculate ambient temperature via `MLX90640_GetTa()`
   - Convert to temperature array (768 floats) via `MLX90640_CalculateTo()`

3. **Measurement Modes**:
   - **Chess mode** (`MLX90640_SetChessMode`): Alternates pixels in checkerboard pattern
   - **Interleaved mode** (`MLX90640_SetInterleavedMode`): Scans in two halves
   - Most examples use chess mode

## I2C Configuration

The sensor's I2C baudrate directly affects achievable framerate:

- **400 kHz**: Max ~8 FPS (compatible with other I2C devices)
- **1 MHz**: Max ~32 FPS (if MLX90640 is the only high-speed device)

Configure in `/boot/config.txt`:
```
dtparam=i2c1_baudrate=1000000
```

Valid refresh rates: 1, 2, 4, 8, 16, 32, 64 Hz (configured via `MLX90640_SetRefreshRate()`)

## Sensor Constants

- **I2C Address**: 0x33 (fixed, defined as `MLX_I2C_ADDR` in examples)
- **Image Dimensions**: 24x32 pixels (768 total)
- **EEPROM Size**: 832 words (calibration data)
- **Frame Data Size**: 834 words (includes status registers)

## Common Development Patterns

### Building a New Example

1. Add source file to `examples/src/`
2. Add example name to `examples` variable in Makefile (line 7)
3. Add linking rule if needed (see existing examples like `test`, `rawrgb`)
4. Use `libMLX90640_API.a` for linking
5. Include headers: `#include "headers/MLX90640_API.h"`

### Typical Example Structure

```cpp
// Initialize
MLX90640_DumpEE(MLX_I2C_ADDR, eeMLX90640);
MLX90640_ExtractParameters(eeMLX90640, &mlx90640);
MLX90640_SetRefreshRate(MLX_I2C_ADDR, rate_code);
MLX90640_SetChessMode(MLX_I2C_ADDR);

// Main loop
while(1) {
    MLX90640_GetFrameData(MLX_I2C_ADDR, frame);
    MLX90640_InterpolateOutliers(frame, eeMLX90640);
    eTa = MLX90640_GetTa(frame, &mlx90640);
    MLX90640_CalculateTo(frame, &mlx90640, emissivity, eTa, mlx90640To);
    // Process mlx90640To[768] array...
}
```

## Dependencies

- **libi2c-dev**: Required for LINUX mode
- **bcm2835**: Required for RPI mode (`make bcm2835` to auto-install)
- **libavutil-dev, libavcodec-dev, libavformat-dev**: For video example
- **libsdl2-dev**: For sdlscale example
- **swig**: For building Python bindings (optional if pre-generated wrapper exists)
- **Python setuptools**: For Python bindings

## Running Examples

- **LINUX mode**: Run directly (e.g., `./examples/test`)
- **RPI mode**: Requires root (e.g., `sudo ./examples/test`)
