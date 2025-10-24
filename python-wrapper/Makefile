PYTHON := python3
VENV := venv
VENV_PYTHON := $(VENV)/bin/python
VENV_PIP := $(VENV)/bin/pip
WHEEL := $(shell find dist -name "*.whl" 2>/dev/null | head -n1)

.PHONY: help venv deps build install install-dev clean test wheel dist examples

help:
	@echo "MLX90640 Python Wrapper Build System"
	@echo ""
	@echo "Prerequisites:"
	@echo "  - MLX90640 C++ library built (make I2C_MODE=LINUX in parent directory)"
	@echo ""
	@echo "Targets:"
	@echo "  make venv         - Create Python virtual environment"
	@echo "  make deps         - Install Python dependencies in venv"
	@echo "  make build        - Build extension module (for development)"
	@echo "  make install-dev  - Install in development mode (editable)"
	@echo "  make wheel        - Build distributable wheel package"
	@echo "  make dist         - Build wheel + display path (for shipping)"
	@echo "  make install      - Install wheel into venv"
	@echo "  make examples     - Run all examples"
	@echo "  make test         - Run tests"
	@echo "  make clean        - Remove build artifacts and venv"
	@echo ""
	@echo "Typical workflow:"
	@echo "  1. cd .. && make I2C_MODE=LINUX  # Build main library"
	@echo "  2. cd python-wrapper"
	@echo "  3. make install-dev      # Setup venv, deps, and install"
	@echo "  4. source venv/bin/activate"
	@echo "  5. python examples/simple_capture.py  # Test"

# Create virtual environment
venv:
	@if [ -d "$(VENV)" ]; then \
		echo "Virtual environment already exists in $(VENV)/"; \
	else \
		echo "Creating virtual environment..."; \
		$(PYTHON) -m venv $(VENV); \
		echo "Virtual environment created in $(VENV)/"; \
		echo "To activate: source $(VENV)/bin/activate"; \
	fi

# Install dependencies
deps: venv
	@echo "Installing dependencies..."
	$(VENV_PIP) install --upgrade pip setuptools wheel
	$(VENV_PIP) install -r requirements.txt

# Build extension in-place (development)
build: deps
	@echo "Building extension module..."
	$(VENV_PYTHON) setup.py build_ext --inplace

# Install in development mode (editable)
install-dev: deps
	@echo "Installing in development mode..."
	$(VENV_PIP) install -e .

# Build distributable wheel
wheel: deps
	@echo "Building wheel package..."
	$(VENV_PYTHON) setup.py bdist_wheel
	@echo ""
	@echo "Wheel built successfully:"
	@ls -lh dist/*.whl

# Build and display distribution info
dist: wheel
	@echo ""
	@echo "=== Distribution Package Ready ==="
	@echo "Wheel file: $(WHEEL)"
	@echo ""
	@echo "To install on another system:"
	@echo "  pip3 install $(WHEEL)"
	@echo ""
	@echo "To ship to another project:"
	@echo "  1. Copy $(WHEEL) to target system"
	@echo "  2. Ensure MLX90640 C++ library is installed"
	@echo "  3. pip3 install mlx90640-*.whl"

# Install wheel into venv
install: wheel
	@echo "Installing wheel..."
	$(VENV_PIP) install --force-reinstall $(WHEEL)

# Run examples
examples: install-dev
	@echo "Running simple_capture.py..."
	$(VENV_PYTHON) examples/simple_capture.py
	@echo ""
	@echo "Running ascii_display.py (Ctrl+C to stop)..."
	$(VENV_PYTHON) examples/ascii_display.py

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	rm -rf build/
	rm -rf dist/
	rm -rf *.egg-info/
	rm -rf mlx90640/*.so
	rm -rf mlx90640/__pycache__
	rm -rf examples/__pycache__
	rm -rf $(VENV)
	find . -name "*.pyc" -delete
	find . -name "__pycache__" -delete
	@echo "Clean complete"
