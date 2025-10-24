# MLX90640 Python Wrapper

Standalone Python wrapper for the MLX90640 thermal camera sensor. Self-contained package that uses the local MLX90640 library build (no system-wide installation needed).

## Features

- **Chess mode only** - Sensor is calibrated for chess mode measurement pattern
- **Self-pacing frame capture** - Blocking calls that wait for sensor (no manual timing needed)
- **Configurable parameters**:
  - Refresh rate: 1, 2, 4, 8, 16, 32, 64 Hz
  - ADC resolution: 16, 17, 18, 19 bit
  - Emissivity: 0.1 - 1.0
- **Optional processing**:
  - Outlier interpolation (fix outlier pixels)
  - Bad pixel correction (compensate for broken/dead pixels)
- **ASCII terminal display** - Real-time thermal display with Inferno colormap

## Prerequisites

### 1. MLX90640 C++ Library (Local Build)

The Python wrapper uses the local library build from the parent directory. First build the main library:

```bash
# From the main repository directory
cd /home/maciej/_dev/pimoroni-mlx90640-library
make I2C_MODE=LINUX
```

This creates `libMLX90640_API.so` and `libMLX90640_API.a` in the main directory, which the Python wrapper will link against.

### 2. System Requirements

- Python 3.6 or higher
- Python venv support
- C++ compiler (g++)
- I2C enabled on Raspberry Pi

### 3. I2C Configuration

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

### Quick Start (Development Mode)

```bash
# Make sure the main library is built first
cd /home/maciej/_dev/pimoroni-mlx90640-library
make I2C_MODE=LINUX

# Now build the Python wrapper
cd python-wrapper

# Create venv and install dependencies
make venv deps

# Install in development mode (editable)
make install-dev

# Activate venv
source venv/bin/activate

# Run example
python examples/simple_capture.py
```

### Build Distributable Wheel

```bash
# Build wheel package
make wheel

# Result: dist/mlx90640-1.0.0-*.whl
```

### Ship to Another Project

The wheel file can be copied to another system. On the target system:

1. Build the MLX90640 library locally (same as above)
2. Install the wheel:

```bash
pip3 install mlx90640-*.whl
```

## Makefile Targets

| Target | Description |
|--------|-------------|
| `make help` | Show all available targets |
| `make venv` | Create Python virtual environment |
| `make deps` | Install Python dependencies in venv |
| `make install-dev` | Install in development/editable mode |
| `make build` | Build extension module in-place |
| `make wheel` | Build distributable wheel package |
| `make dist` | Build wheel and show distribution info |
| `make install` | Install wheel into venv |
| `make clean` | Remove all build artifacts and venv |

## Usage

### Basic Example

```python
import mlx90640

# Initialize camera
camera = mlx90640.MLX90640Camera()
camera.init()

# Configure
camera.set_refresh_rate(16)      # 16 Hz
camera.set_emissivity(0.95)      # For human skin
camera.set_resolution(3)         # 19-bit (highest quality)

# Capture frame (blocking, returns when ready)
frame = camera.get_frame()

# frame is a list of 768 floats (24 rows x 32 cols)
print(f"Max temperature: {max(frame):.2f}°C")
print(f"Min temperature: {min(frame):.2f}°C")
print(f"Average: {sum(frame)/len(frame):.2f}°C")

# Cleanup
camera.cleanup()
```

### Real-time Processing Loop

```python
import mlx90640
import time

camera = mlx90640.MLX90640Camera()
camera.init()
camera.set_refresh_rate(16)

frame_count = 0
start_time = time.time()

try:
    while True:
        # Blocking call - returns when sensor has new frame
        frame = camera.get_frame()

        # Process frame here
        max_temp = max(frame)

        frame_count += 1
        fps = frame_count / (time.time() - start_time)
        print(f"Frame {frame_count}, FPS: {fps:.2f}, Max: {max_temp:.2f}°C")

except KeyboardInterrupt:
    pass
finally:
    camera.cleanup()
```

### Frame Layout

The `get_frame()` method returns a list of 768 floats representing a 24x32 pixel thermal image:

```python
frame = camera.get_frame()

# Access pixel at row=10, col=15
pixel = frame[10 * 32 + 15]

# Iterate all pixels
for row in range(24):
    for col in range(32):
        temp = frame[row * 32 + col]
        print(f"({row},{col}): {temp:.2f}°C")
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
- Returns: List of 768 floats (temperatures in °C)
- Layout: 24 rows × 32 columns, row-major order
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

- **Refresh Rate vs I2C Speed**:
  - 1-8 Hz: 400kHz I2C is sufficient
  - 16-64 Hz: Requires 1MHz I2C baudrate

- **Resolution vs Speed**:
  - Higher resolution = more accurate but slower ADC conversion
  - For fast motion: use 16-17 bit
  - For accuracy: use 18-19 bit

- **Frame Capture Timing**:
  - `get_frame()` is blocking and self-paced by the sensor
  - No need for manual delays (sensor controls timing)
  - Actual FPS may be slightly lower than configured rate

## Technical Details

### Chess Mode

The sensor operates in chess mode only, where pixels are read in a checkerboard pattern alternating between two subpages. This is the mode the sensor is calibrated for and provides the best accuracy.

### Self-Pacing

The `get_frame()` call blocks until the sensor has new data ready. This is implemented via polling the sensor's status register. No artificial delays are needed - the sensor's refresh rate controls the timing.

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
