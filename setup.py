#!/usr/bin/env python3
"""
MLX90640 Python Wrapper Setup
Builds pybind11 extension for MLX90640 thermal camera
All metadata is in pyproject.toml - this file only handles C++ extension building
"""

from setuptools import setup, Extension
import os

class get_pybind_include(object):
    """Helper class to determine the pybind11 include path"""
    def __str__(self):
        import pybind11
        return pybind11.get_include()

# Get absolute path to current directory (where libMLX90640_API.so is)
current_dir = os.path.abspath(os.path.dirname(__file__))

ext_modules = [
    Extension(
        'mlx90640._camera',
        sources=['mlx90640/camera.cpp'],
        include_dirs=[
            os.path.join(current_dir, 'mlx90640/lib'),  # Local headers from mlx90640/lib/
            str(get_pybind_include()),
        ],
        libraries=['MLX90640_API'],
        library_dirs=[current_dir],  # Local library build directory (root)
        language='c++',
        extra_compile_args=['-std=c++11', '-fPIC', '-O3', '-march=native'],
        extra_link_args=[f'-Wl,-rpath,{current_dir}'],
    ),
]

setup(
    ext_modules=ext_modules,
)
