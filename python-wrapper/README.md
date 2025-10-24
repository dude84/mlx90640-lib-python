# MLX90640 Python Wrapper

Standalone Python wrapper for the MLX90640 thermal camera sensor. Self-contained package that uses the local MLX90640 library build (no system-wide installation needed).

## Features

- **Chess mode only** - Sensor is calibrated for chess mode measurement pattern
- **Self-pacing frame capture** - Blocking calls that wait for sensor (no manual timing needed)
- **Zero-copy NumPy arrays** - High-performance frame capture (~15.8 FPS at 16 Hz)
- **Configurable parameters**:
  - Refresh rate: 1, 2, 4, 8, 16, 32, 64 Hz
  - ADC resolution: 16, 17, 18, 19 bit
  - Emissivity: 0.1 - 1.0
- **Optional processing**:
  - Outlier interpolation (fix outlier pixels)
  - Bad pixel correction (compensate for broken/dead pixels)
- **ASCII terminal display** - Real-time thermal display with Inferno colormap
- **Optimized builds** - Release mode with -O3 optimization by default

## Prerequisites

### 1. Build Modes

The library supports two build modes:

- **Release mode (default)**: Optimized with `-O3 -march=native` for maximum performance
- **Debug mode**: Built with debug symbols for development

```bash
# Release build (recommended)
make all

# Debug build (for development)
make all DEBUG=1
```

### 2. MLX90640 C++ Library

The Python wrapper automatically builds the main library when you run `make all`. The library creates `libMLX90640_API.so` and `libMLX90640_API.a` which the Python wrapper links against.

### 3. System Requirements

- Python 3.6 or higher
- Python venv support
- C++ compiler (g++)
- I2C enabled on Raspberry Pi

### 4. I2C Configuration

Enable I2C and set baudrate for high-speed operation:

Edit `/boot/config.txt`:
```
dtparam=i2c_arm=on
dtparam=i2c1_baudrate=1000000
```

Reboot and verify:
```bash
i2cdetect -y 1
# Should show device at address 0x33
```

Add user to i2c group (if needed):
```bash
sudo usermod -a -G i2c $USER
# Log out and back in for this to take effect
```

## Installation

### Quick Start

The simplest way to build and install everything:

```bash
cd python-wrapper
make all

# Activate virtual environment
source venv/bin/activate

# Run example
python examples/ascii_display.py
```

That's it! The `make all` command:
1. Builds the C++ library with optimizations
2. Creates a Python virtual environment
3. Installs all dependencies
4. Installs the wrapper in development mode

### Build Distributable Wheel

To create a wheel package for deployment:

```bash
make wheel

# Result: dist/mlx90640-1.0.0-*.whl
```

## Deployment

### What You Need to Deploy

To deploy the wrapper to another system, you need:

1. **The wheel file**: `dist/mlx90640-1.0.0-*.whl` (contains Python wrapper + compiled extension)
2. **The C++ library**: `libMLX90640_API.so` (from parent directory)

### Deployment Options

**Option 1: Deploy with wheel file** (recommended for Python projects)

```bash
# On target system
# 1. Copy both files to target
scp dist/mlx90640-*.whl target:/tmp/
scp ../libMLX90640_API.so target:/tmp/

# 2. Install wheel
pip3 install /tmp/mlx90640-*.whl

# The wrapper is configured to find libMLX90640_API.so using rpath,
# so the .so must be in the same directory as the wheel or in system paths
```

**Option 2: System-wide library installation**

```bash
# On target system
# 1. Install C++ library system-wide
cd /path/to/pimoroni-mlx90640-library
make I2C_MODE=LINUX
sudo make install  # Installs to /usr/local/lib

# 2. Install Python wrapper
pip3 install mlx90640-*.whl
```

**Option 3: Development/source deployment**

```bash
# On target system - deploy entire source tree
git clone <repo>
cd pimoroni-mlx90640-library/python-wrapper
make all
source venv/bin/activate
```

### Important Notes

- The wrapper extension is compiled for the target architecture (ARM for Raspberry Pi)
- Ensure I2C is enabled on the target system
- The wheel includes the compiled C++ extension, so no compiler needed on target
- For production, use release builds (default) for best performance

## Makefile Targets

| Target | Description |
|--------|-------------|
| `make all` | **Build C++ library + install wrapper** (recommended) |
| `make help` | Show all available targets and explanations |
| `make install-dev` | Install wrapper in development mode (auto-creates venv) |
| `make build` | Build C++ extension in-place (for testing C++ changes) |
| `make wheel` | Build distributable wheel package |
| `make dist` | Build wheel and show deployment information |
| `make install` | Install wheel into venv (production-style install) |
| `make clean` | Remove all build artifacts and venv |

### Target Explanations

- **`build`**: Compiles the C++ extension only. Use this when you modify camera.cpp and want to test changes without reinstalling the package.
- **`install-dev`**: Editable installation (`pip install -e .`). Python file changes are immediately visible without rebuild. C++ changes still require `make build`.
- **`install`**: Builds a wheel and installs it as an isolated package. Use this to test the final package or for production deployment.

## Usage

### Basic Example

```python
import mlx90640
import numpy as np

# Initialize camera
camera = mlx90640.MLX90640Camera()
camera.init()

# Configure
camera.set_refresh_rate(16)      # 16 Hz
camera.set_emissivity(0.95)      # For human skin
camera.set_resolution(3)         # 19-bit (highest quality)

# Capture frame (blocking, returns when ready)
frame = camera.get_frame()

# frame is a read-only NumPy array of 768 floats (24 rows x 32 cols)
print(f"Frame shape: {frame.shape}")  # (768,)
print(f"Frame dtype: {frame.dtype}")  # float32
print(f"Max temperature: {frame.max():.2f}°C")
print(f"Min temperature: {frame.min():.2f}°C")
print(f"Average: {frame.mean():.2f}°C")

# Cleanup
camera.cleanup()
```

### Real-time Processing Loop

```python
import mlx90640
import numpy as np
import time

camera = mlx90640.MLX90640Camera()
camera.init()
camera.set_refresh_rate(16)

frame_count = 0
start_time = time.time()

try:
    while True:
        # Blocking call - returns when sensor has new frame
        # Zero-copy NumPy array for maximum performance
        frame = camera.get_frame()

        # Process frame using NumPy operations
        max_temp = frame.max()

        # Reshape to 2D for image processing
        frame_2d = frame.reshape(24, 32)

        frame_count += 1
        fps = frame_count / (time.time() - start_time)
        print(f"Frame {frame_count}, FPS: {fps:.2f}, Max: {max_temp:.2f}°C")

except KeyboardInterrupt:
    pass
finally:
    camera.cleanup()
```

### Frame Layout

The `get_frame()` method returns a read-only NumPy array of 768 floats representing a 24x32 pixel thermal image:

```python
import numpy as np

frame = camera.get_frame()

# Frame is 1D array, shape (768,)
print(frame.shape)  # (768,)

# Access pixel at row=10, col=15 (1D indexing)
pixel = frame[10 * 32 + 15]

# Reshape to 2D for easier access
frame_2d = frame.reshape(24, 32)
pixel = frame_2d[10, 15]

# Iterate all pixels (NumPy style)
for row in range(24):
    for col in range(32):
        temp = frame_2d[row, col]
        print(f"({row},{col}): {temp:.2f}°C")

# Note: The array is read-only (zero-copy from C++)
# To modify, create a copy:
frame_copy = frame.copy()
```

## API Reference

### MLX90640Camera Class

#### Constructor

```python
camera = mlx90640.MLX90640Camera(addr=0x33)
```

- `addr` (int, optional): I2C address, default 0x33

#### Methods

**`init()`**

Initialize camera (reads EEPROM, configures chess mode).

- Returns: 0 on success
- Raises: `RuntimeError` on failure

**`cleanup()`**

Cleanup camera resources.

**`set_refresh_rate(fps)`**

Set frame rate.

- `fps` (int): Frame rate in Hz (1, 2, 4, 8, 16, 32, 64)
- Note: Rates ≥16 Hz require 1MHz I2C baudrate
- Raises: `ValueError` for invalid FPS, `RuntimeError` on I2C failure

**`set_resolution(resolution)`**

Set ADC resolution.

- `resolution` (int): 0=16bit, 1=17bit, 2=18bit, 3=19bit
- Note: Higher resolution = better accuracy but slower
- Raises: `ValueError` for invalid resolution, `RuntimeError` on I2C failure

**`set_emissivity(emissivity)`**

Set emissivity for temperature calculation.

- `emissivity` (float): 0.1-1.0
  - 1.0 = Perfect blackbody
  - 0.95 = Human skin
  - 0.90 = Matte surfaces
  - 0.80 = Wood
- Raises: `ValueError` for out of range value

**`get_frame(interpolate_outliers=True, correct_bad_pixels=True)`**

Capture thermal frame (blocking, self-paced).

- `interpolate_outliers` (bool): Apply outlier interpolation
- `correct_bad_pixels` (bool): Apply bad pixel correction
- Returns: **Read-only NumPy array** of 768 floats (temperatures in °C)
- Shape: `(768,)` - can be reshaped to `(24, 32)` for 2D access
- Layout: 24 rows × 32 columns, row-major order
- Performance: Zero-copy design for maximum speed (~15.8 FPS at 16 Hz)
- Note: Array is read-only. Use `.copy()` if you need to modify values
- Raises: `RuntimeError` if not initialized or capture fails

**`get_refresh_rate()`**

Get current refresh rate register value.

- Returns: int

**`get_resolution()`**

Get current ADC resolution.

- Returns: int (0-3)

**`get_emissivity()`**

Get current emissivity.

- Returns: float (0.1-1.0)

**`is_initialized()`**

Check if camera is initialized.

- Returns: bool

## Examples

### simple_capture.py

Basic frame capture and statistics display.

```bash
python examples/simple_capture.py
```

### ascii_display.py

Real-time ASCII thermal display with Inferno colormap (like C++ test.cpp).
Includes embedded Inferno colormap function.

```bash
python examples/ascii_display.py
```

Press Ctrl+C to exit.

### configure_params.py

Demonstrates all configuration parameters and validates input.

```bash
python examples/configure_params.py
```

## Troubleshooting

### Import Error: `No module named 'mlx90640'`

Solution:
```bash
cd python-wrapper
make install-dev
source venv/bin/activate
```

### Import Error: `No module named '_camera'`

The C++ extension didn't build. Check:
```bash
make clean
make venv deps
make install-dev
```

Look for compilation errors.

### Runtime Error: "Failed to initialize camera"

Possible causes:

1. **I2C not enabled**
   ```bash
   # Check if I2C is enabled
   lsmod | grep i2c
   # Should show i2c_dev and i2c_bcm2835
   ```

2. **Camera not connected**
   ```bash
   i2cdetect -y 1
   # Should show device at address 0x33
   ```

3. **Permission denied**
   ```bash
   # Add user to i2c group
   sudo usermod -a -G i2c $USER
   # Log out and back in
   ```

4. **Library not built**
   ```bash
   cd ..
   make I2C_MODE=LINUX
   ls -l libMLX90640_API.so
   ```

### Build Error: "MLX90640_API.h: No such file or directory"

The main library is not built.

Solution:
```bash
cd /home/maciej/_dev/pimoroni-mlx90640-library
make I2C_MODE=LINUX
```

### Low FPS or Dropped Frames

1. **Check I2C baudrate**
   ```bash
   # Add to /boot/config.txt
   dtparam=i2c1_baudrate=1000000
   ```

2. **Reduce resolution**
   ```python
   camera.set_resolution(1)  # Use 17-bit instead of 19-bit
   ```

3. **Check system load**
   ```bash
   top
   # Make sure CPU isn't overloaded
   ```

## Performance Notes

- **Achievable Frame Rate**:
  - At 16 Hz refresh rate: ~**15.8 FPS** sustained
  - At 32 Hz refresh rate: requires 1MHz I2C (see below)
  - Zero-copy NumPy arrays eliminate Python overhead
  - Perfect subpage alternation (0→1→0→1) for complete frames

- **Refresh Rate vs I2C Speed**:
  - 1-8 Hz: 400kHz I2C is sufficient
  - 16-64 Hz: **Requires 1MHz I2C baudrate** (configure in `/boot/config.txt`)

- **Resolution vs Speed**:
  - Higher resolution = more accurate but slower ADC conversion
  - For fast motion: use 16-17 bit (resolution=0 or 1)
  - For accuracy: use 18-19 bit (resolution=2 or 3)

- **Frame Capture Timing**:
  - `get_frame()` is blocking and self-paced by the sensor
  - No need for manual delays (sensor controls timing)
  - Actual FPS may be slightly lower than configured rate (~15.7-15.8 at 16Hz)

- **Zero-Copy Design**:
  - Returns read-only NumPy array wrapping internal C++ buffer
  - No data copying between C++ and Python
  - Minimal latency for real-time processing
  - Use `.copy()` only if you need to modify the data

- **Optimization**:
  - Built with `-O3 -march=native` for maximum performance
  - Release builds are significantly faster than debug builds
  - Avoid background I2C processes that can cause bus contention

## Technical Details

### Chess Mode

The sensor operates in chess mode only, where pixels are read in a checkerboard pattern alternating between two **subpages** (0 and 1). This is the mode the sensor is calibrated for and provides the best accuracy.

**How it works**:
- Each frame captures only half the pixels (one subpage)
- Subpages alternate: 0 → 1 → 0 → 1 → ...
- A complete thermal image requires both subpages
- The sensor alternates automatically at the configured refresh rate

**Why this matters**:
- The wrapper ensures perfect subpage alternation (verified in testing)
- If subpage alternation breaks, you'll see "ghosting" in the thermal image
- The wrapper's zero-copy design maintains consistent ~15.8 FPS without skipping subpages

You can check subpage alternation using:
```python
subpage = camera.get_subpage_number()  # Returns 0 or 1
```

### Self-Pacing

The `get_frame()` call blocks until the sensor has new data ready. This is implemented via polling the sensor's status register. No artificial delays are needed - the sensor's refresh rate controls the timing.

**Implementation details**:
- `MLX90640_GetFrameData()` blocks until the dataReady bit is set
- Returns subpage number (0 or 1) on success, negative on error
- Typical frame time at 16 Hz: ~63ms (31.25ms per subpage)
- Zero-copy design ensures minimal overhead

### Library Linking

The Python extension links against the local `libMLX90640_API.so` in the parent directory. This means:
- No system-wide installation needed
- The .so file must be accessible when running Python code
- Using `$ORIGIN` in rpath to find the library relative to the extension

### Temperature Calculation

Temperature calculation formula:
```
T_object = f(ADC, calibration_params, emissivity, T_ambient)
```

The emissivity setting affects how reflected temperature is compensated. Higher emissivity (closer to 1.0) means the object radiates more efficiently.

## Directory Structure

```
python-wrapper/
├── Makefile               # Build system
├── README.md              # This file
├── requirements.txt       # Python dependencies
├── setup.py               # Python package configuration
├── pyproject.toml         # PEP 517/518 build config
├── MANIFEST.in            # Source distribution includes
├── mlx90640/
│   ├── __init__.py        # Python package
│   ├── camera.h           # C++ header
│   └── camera.cpp         # C++ implementation (pybind11)
└── examples/
    ├── simple_capture.py      # Basic usage
    ├── ascii_display.py       # Real-time display (with embedded colormap)
    └── configure_params.py    # Configuration demo
```

## License

Apache License 2.0 (matches MLX90640 library)

## Credits

- Based on Pimoroni MLX90640 library
- Melexis MLX90640 sensor
- Inferno colormap from matplotlib

## Version

1.0.0
