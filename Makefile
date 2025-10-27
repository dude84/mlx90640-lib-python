PYTHON := python3
VENV := venv
VENV_PYTHON := $(VENV)/bin/python
VENV_PIP := $(VENV)/bin/pip
WHEEL := $(shell find dist -name "*.whl" 2>/dev/null | head -n1)

# C++ library settings
ifdef DEBUG
	CXXFLAGS+=-DDEBUG -g
else
	CXXFLAGS+=-O3 -march=native -DNDEBUG
endif

ifeq ($(PREFIX),)
	PREFIX = /usr/local
endif

.PHONY: help all build install-dev clean test wheel dist examples libclean lib-install

help:
	@echo "MLX90640 Python Wrapper Build System"
	@echo ""
	@echo "Quick Start:"
	@echo "  make all              - Build C++ library + install Python wrapper"
	@echo "  source venv/bin/activate"
	@echo "  python examples/ascii_display.py"
	@echo ""
	@echo "Targets:"
	@echo "  make all          - Build main library and install wrapper (recommended)"
	@echo "  make install-dev  - Install wrapper in development mode (auto-creates venv)"
	@echo "  make build        - Build extension module in-place"
	@echo "  make wheel        - Build distributable wheel package"
	@echo "  make dist         - Build wheel + show deployment info"
	@echo "  make install      - Install wheel into venv"
	@echo "  make examples     - Run example scripts"
	@echo "  make clean        - Remove all build artifacts and venv"
	@echo ""
	@echo "Build Modes:"
	@echo "  Release (default):  make all             # Optimized (-O3)"
	@echo "  Debug:              make all DEBUG=1     # Debug symbols (-g)"
	@echo ""
	@echo "Target Explanations:"
	@echo "  build       = Compile C++ extension only (for testing C++ changes)"
	@echo "  install-dev = Editable install (Python changes auto-reflected)"
	@echo "  install     = Production install from wheel (isolated copy)"

# C++ library build targets
libMLX90640_API.so: mlx90640/lib/MLX90640_API.o mlx90640/lib/MLX90640_LINUX_I2C_Driver.o
	$(CXX) -fPIC -shared $^ -o $@

libMLX90640_API.a: mlx90640/lib/MLX90640_API.o mlx90640/lib/MLX90640_LINUX_I2C_Driver.o
	ar rcs $@ $^
	ranlib $@

mlx90640/lib/MLX90640_API.o mlx90640/lib/MLX90640_LINUX_I2C_Driver.o : CXXFLAGS+=-fPIC -I mlx90640/lib -shared

# Build everything: main library + Python wrapper
all: libMLX90640_API.so libMLX90640_API.a
	@echo "========================================"
	@echo "MLX90640 C++ Library Built"
	@echo "========================================"
	@echo ""
	@echo "========================================"
	@echo "Installing Python Wrapper"
	@echo "========================================"
	$(MAKE) install-dev
	@echo ""
	@echo "========================================"
	@echo "Build Complete!"
	@echo "========================================"
	@echo "To use:"
	@echo "  source venv/bin/activate"
	@echo "  python examples/ascii_display.py"

# Install in development mode (auto-creates venv and installs deps)
install-dev:
	@if [ ! -d "$(VENV)" ]; then \
		echo "Creating virtual environment..."; \
		$(PYTHON) -m venv $(VENV); \
		echo ""; \
	fi
	@echo "Installing dependencies..."
	$(VENV_PIP) install --upgrade pip setuptools wheel
	$(VENV_PIP) install -r requirements.txt
	@echo ""
	@echo "Installing wrapper in development mode..."
	$(VENV_PIP) install -e .
	@echo ""
	@echo "Setup complete!"

# Build extension in-place (for C++ development)
build: install-dev
	@echo "Building extension module..."
	$(VENV_PYTHON) setup.py build_ext --inplace

# Build distributable wheel
wheel: install-dev
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
clean: libclean
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

# Clean C++ library artifacts
libclean:
	@echo "Cleaning C++ library artifacts..."
	rm -f mlx90640/lib/*.o
	rm -f *.o
	rm -f libMLX90640_API.so
	rm -f libMLX90640_API.a

# Install C++ library system-wide
lib-install: libMLX90640_API.a libMLX90640_API.so
	install -d $(DESTDIR)$(PREFIX)/lib/
	install -m 644 libMLX90640_API.a $(DESTDIR)$(PREFIX)/lib/
	install -m 644 libMLX90640_API.so $(DESTDIR)$(PREFIX)/lib/
	install -d $(DESTDIR)$(PREFIX)/include/MLX90640/
	install -m 644 mlx90640/lib/*.h $(DESTDIR)$(PREFIX)/include/MLX90640/
	ldconfig
