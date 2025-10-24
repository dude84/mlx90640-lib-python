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

Example:
    >>> import mlx90640
    >>> camera = mlx90640.MLX90640Camera()
    >>> camera.init()
    >>> camera.set_refresh_rate(16)
    >>> camera.set_emissivity(0.95)
    >>> frame = camera.get_frame()
    >>> print(f"Max temp: {max(frame):.2f}°C")
"""

from ._camera import MLX90640Camera

__version__ = '1.0.0'
__all__ = ['MLX90640Camera']
