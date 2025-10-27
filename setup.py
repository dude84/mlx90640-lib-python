#!/usr/bin/env python3
"""
MLX90640 Python Wrapper Setup
Builds pybind11 extension for MLX90640 thermal camera
"""

from setuptools import setup, Extension
from setuptools.command.build_ext import build_ext
import sys
import os

class get_pybind_include(object):
    """Helper class to determine the pybind11 include path"""
    def __str__(self):
        import pybind11
        return pybind11.get_include()

import os

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

with open('README.md', 'r') as f:
    long_description = f.read()

setup(
    name='mlx90640',
    version='1.0.0',
    author='MLX90640 Python Wrapper',
    author_email='',
    description='Python wrapper for MLX90640 thermal camera (chess mode only)',
    long_description=long_description,
    long_description_content_type='text/markdown',
    url='',
    packages=['mlx90640'],
    ext_modules=ext_modules,
    install_requires=['pybind11>=2.6.0', 'numpy>=1.19.0'],
    python_requires='>=3.6',
    classifiers=[
        'Development Status :: 4 - Beta',
        'Intended Audience :: Developers',
        'License :: OSI Approved :: Apache Software License',
        'Programming Language :: Python :: 3',
        'Programming Language :: Python :: 3.6',
        'Programming Language :: Python :: 3.7',
        'Programming Language :: Python :: 3.8',
        'Programming Language :: Python :: 3.9',
        'Programming Language :: C++',
        'Topic :: Scientific/Engineering',
        'Topic :: System :: Hardware',
    ],
)
