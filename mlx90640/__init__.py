"""
MLX90640 Thermal Camera Python Wrapper

Provides easy-to-use Python interface for the MLX90640 thermal camera sensor.

Features:
- Chess mode only (sensor calibrated for this mode)
- Self-pacing frame capture (blocking until sensor ready)
- Configurable refresh rate (1-64 Hz)
- Configurable ADC resolution (16-19 bit)
- Adjustable emissivity for different materials
- Optional outlier interpolation and bad pixel correction
- Configurable image rotation (0, 90, 180, 270 degrees)

Example:
    >>> import mlx90640
    >>> camera = mlx90640.MLX90640Camera()
    >>> camera.init()
    >>> camera.set_refresh_rate(16)
    >>> camera.set_emissivity(0.95)
    >>> camera.set_rotation(180)  # Default rotation for upside-down mounting
    >>> frame = camera.get_frame()
    >>> print(f"Max temp: {max(frame):.2f}°C")
"""

import numpy as np
from ._camera import MLX90640Camera as _MLX90640CameraBase

class MLX90640Camera:
    """
    Python wrapper for MLX90640 thermal camera with rotation support.

    The camera produces 24x32 pixel thermal images. By default, the image
    is rotated 180 degrees to account for typical upside-down mounting.
    """

    def __init__(self, addr=0x33, rotation=180):
        """
        Create camera instance.

        Args:
            addr: I2C address (default: 0x33)
            rotation: Image rotation in degrees (0, 90, 180, 270). Default: 180
        """
        self._camera = _MLX90640CameraBase(addr)
        self._rotation = 0
        self.set_rotation(rotation)

    def set_rotation(self, degrees):
        """
        Set image rotation.

        Args:
            degrees: Rotation angle (0, 90, 180, 270)

        Raises:
            ValueError: If degrees is not 0, 90, 180, or 270
        """
        if degrees not in (0, 90, 180, 270):
            raise ValueError(f"Rotation must be 0, 90, 180, or 270 degrees, got {degrees}")
        self._rotation = degrees

    def get_rotation(self):
        """
        Get current image rotation.

        Returns:
            Rotation angle in degrees (0, 90, 180, or 270)
        """
        return self._rotation

    def init(self):
        """Initialize camera (reads EEPROM, configures chess mode)"""
        return self._camera.init()

    def cleanup(self):
        """Cleanup camera resources"""
        self._camera.cleanup()

    def set_refresh_rate(self, fps):
        """Set refresh rate in Hz (1, 2, 4, 8, 16, 32, 64)"""
        return self._camera.set_refresh_rate(fps)

    def set_resolution(self, resolution):
        """Set ADC resolution (0=16bit, 1=17bit, 2=18bit, 3=19bit)"""
        return self._camera.set_resolution(resolution)

    def set_emissivity(self, emissivity):
        """Set emissivity (0.1-1.0, 1.0=blackbody, 0.95=skin)"""
        self._camera.set_emissivity(emissivity)

    def get_frame(self, interpolate_outliers=True, correct_bad_pixels=True):
        """
        Capture frame (blocking, self-paced by sensor).

        Args:
            interpolate_outliers: Apply outlier interpolation (default: True)
            correct_bad_pixels: Apply bad pixel correction (default: True)

        Returns:
            NumPy array of 768 floats representing temperatures in °C.
            Layout: 24 rows x 32 columns, row-major order.
            Use .reshape((24, 32)) for 2D access.
        """
        frame = self._camera.get_frame(interpolate_outliers, correct_bad_pixels)

        # Apply rotation if needed
        if self._rotation != 0:
            # Reshape to 2D (24x32), apply rotation, flatten back to 1D
            frame_2d = frame.reshape(24, 32)
            k = self._rotation // 90  # Number of 90-degree rotations
            rotated = np.rot90(frame_2d, k)
            frame = rotated.flatten()

        return frame

    def get_refresh_rate(self):
        """Get current refresh rate register value"""
        return self._camera.get_refresh_rate()

    def get_resolution(self):
        """Get current ADC resolution (0-3)"""
        return self._camera.get_resolution()

    def get_emissivity(self):
        """Get current emissivity (0.1-1.0)"""
        return self._camera.get_emissivity()

    def is_initialized(self):
        """Check if camera is initialized"""
        return self._camera.is_initialized()

    def get_subpage_number(self):
        """Get subpage number from last captured frame (0, 1, or -1)"""
        return self._camera.get_subpage_number()

__version__ = '1.0.0'
__all__ = ['MLX90640Camera']
