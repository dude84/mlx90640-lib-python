/**
 * MLX90640 Thermal Camera C++ Wrapper Implementation
 *
 * Uses pybind11 for Python bindings
 */

#include "camera.h"
#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <stdexcept>
#include <cstring>

namespace py = pybind11;

// Constructor
MLX90640Camera::MLX90640Camera(uint8_t addr)
    : i2c_addr(addr),
      emissivity(1.0f),
      initialized(false) {

    // Initialize buffers to zero
    std::memset(&params, 0, sizeof(params));
    std::memset(eeprom, 0, sizeof(eeprom));
    std::memset(frame_buffer, 0, sizeof(frame_buffer));
    std::memset(temp_buffer, 0, sizeof(temp_buffer));
}

// Destructor
MLX90640Camera::~MLX90640Camera() {
    cleanup();
}

// Initialize camera
int MLX90640Camera::init() {
    // Configure device FIRST (same order as test.cpp)
    // Set continuous measurement mode
    int status = MLX90640_SetDeviceMode(i2c_addr, 0);
    if (status != 0) {
        throw std::runtime_error("Failed to set device mode (error " + std::to_string(status) + ")");
    }

    // Disable subpage repeat
    status = MLX90640_SetSubPageRepeat(i2c_addr, 0);
    if (status != 0) {
        throw std::runtime_error("Failed to set subpage repeat (error " + std::to_string(status) + ")");
    }

    // Set default refresh rate (16 Hz = 0b101, matching test.cpp)
    status = MLX90640_SetRefreshRate(i2c_addr, 0b101);
    if (status != 0) {
        throw std::runtime_error("Failed to set refresh rate (error " + std::to_string(status) + ")");
    }

    // Set chess mode (sensor calibrated for this)
    status = MLX90640_SetChessMode(i2c_addr);
    if (status != 0) {
        throw std::runtime_error("Failed to set chess mode (error " + std::to_string(status) + ")");
    }

    // NOW read EEPROM (calibration data)
    status = MLX90640_DumpEE(i2c_addr, eeprom);
    if (status != 0) {
        throw std::runtime_error("Failed to read EEPROM (error " + std::to_string(status) + ")");
    }

    // Extract calibration parameters
    status = MLX90640_ExtractParameters(eeprom, &params);
    if (status != 0) {
        throw std::runtime_error("Failed to extract parameters (error " + std::to_string(status) + ")");
    }

    initialized = true;
    return 0;
}

// Cleanup
void MLX90640Camera::cleanup() {
    initialized = false;
}

// Set refresh rate
int MLX90640Camera::set_refresh_rate(int fps) {
    uint8_t rate_code;

    // Map FPS to register value (based on datasheet)
    switch(fps) {
        case 1:  rate_code = 0b001; break;
        case 2:  rate_code = 0b010; break;
        case 4:  rate_code = 0b011; break;
        case 8:  rate_code = 0b100; break;
        case 16: rate_code = 0b101; break;
        case 32: rate_code = 0b110; break;
        case 64: rate_code = 0b111; break;
        default:
            throw std::invalid_argument(
                "Invalid FPS: " + std::to_string(fps) +
                ". Must be 1, 2, 4, 8, 16, 32, or 64"
            );
    }

    int status = MLX90640_SetRefreshRate(i2c_addr, rate_code);
    if (status != 0) {
        throw std::runtime_error("Failed to set refresh rate (error " + std::to_string(status) + ")");
    }

    return status;
}

// Set resolution
int MLX90640Camera::set_resolution(uint8_t res) {
    if (res > 3) {
        throw std::invalid_argument(
            "Invalid resolution: " + std::to_string(res) +
            ". Must be 0-3 (0=16bit, 1=17bit, 2=18bit, 3=19bit)"
        );
    }

    int status = MLX90640_SetResolution(i2c_addr, res);
    if (status != 0) {
        throw std::runtime_error("Failed to set resolution (error " + std::to_string(status) + ")");
    }

    return status;
}

// Set emissivity
void MLX90640Camera::set_emissivity(float emis) {
    if (emis < 0.1f || emis > 1.0f) {
        throw std::invalid_argument(
            "Invalid emissivity: " + std::to_string(emis) +
            ". Must be 0.1-1.0 (1.0=blackbody, 0.95=human skin)"
        );
    }
    emissivity = emis;
}

// Capture frame
float* MLX90640Camera::get_frame(bool interpolate_outliers,
                                  bool correct_bad_pixels) {
    if (!initialized) {
        throw std::runtime_error("Camera not initialized. Call init() first.");
    }

    // Get frame data (blocking call - waits until sensor has data ready)
    // MLX90640_GetFrameData blocks until dataReady bit is set
    // In chess mode, sensor alternates between subpages 0 and 1
    // Returns subpage number (0 or 1) on success, negative on error
    int status = MLX90640_GetFrameData(i2c_addr, frame_buffer);
    if (status < 0) {
        throw std::runtime_error("Failed to get frame data (error " + std::to_string(status) + ")");
    }

    // Optionally interpolate outlier pixels
    if (interpolate_outliers) {
        MLX90640_InterpolateOutliers(frame_buffer, eeprom);
    }

    // Get ambient temperature
    float eTa = MLX90640_GetTa(frame_buffer, &params);

    // Calculate object temperatures for all 768 pixels
    MLX90640_CalculateTo(frame_buffer, &params, emissivity, eTa, temp_buffer);

    // Optionally correct bad pixels
    if (correct_bad_pixels) {
        // Correct broken pixels
        MLX90640_BadPixelsCorrection(params.brokenPixels, temp_buffer, 1, &params);
        // Correct outlier pixels
        MLX90640_BadPixelsCorrection(params.outlierPixels, temp_buffer, 1, &params);
    }

    // Return raw pointer (will be copied to NumPy array by pybind11 lambda)
    return temp_buffer;
}

// Get refresh rate
int MLX90640Camera::get_refresh_rate() const {
    return MLX90640_GetRefreshRate(i2c_addr);
}

// Get resolution
int MLX90640Camera::get_resolution() const {
    return MLX90640_GetCurResolution(i2c_addr);
}

// Get emissivity
float MLX90640Camera::get_emissivity() const {
    return emissivity;
}

// Get subpage number from last captured frame
int MLX90640Camera::get_subpage_number() const {
    if (!initialized) {
        return -1;
    }
    // API expects non-const pointer but doesn't modify the data
    return MLX90640_GetSubPageNumber(const_cast<uint16_t*>(frame_buffer));
}

// pybind11 module binding
PYBIND11_MODULE(_camera, m) {
    m.doc() = "MLX90640 thermal camera wrapper (chess mode only)";

    py::class_<MLX90640Camera>(m, "MLX90640Camera")
        .def(py::init<uint8_t>(), py::arg("addr") = 0x33,
             "Create camera instance\n\n"
             "Args:\n"
             "    addr: I2C address (default: 0x33)")

        .def("init", &MLX90640Camera::init,
             "Initialize camera (reads EEPROM, configures chess mode)\n\n"
             "Returns:\n"
             "    0 on success\n\n"
             "Raises:\n"
             "    RuntimeError: If initialization fails")

        .def("cleanup", &MLX90640Camera::cleanup,
             "Cleanup camera resources")

        .def("set_refresh_rate", &MLX90640Camera::set_refresh_rate,
             py::arg("fps"),
             "Set refresh rate\n\n"
             "Args:\n"
             "    fps: Frame rate in Hz (1, 2, 4, 8, 16, 32, 64)\n\n"
             "Note:\n"
             "    Rates >=16 Hz require 1MHz I2C baudrate\n"
             "    Configure in /boot/config.txt: dtparam=i2c1_baudrate=1000000\n\n"
             "Raises:\n"
             "    ValueError: If FPS is not valid\n"
             "    RuntimeError: If I2C communication fails")

        .def("set_resolution", &MLX90640Camera::set_resolution,
             py::arg("resolution"),
             "Set ADC resolution\n\n"
             "Args:\n"
             "    resolution: 0=16bit, 1=17bit, 2=18bit, 3=19bit\n\n"
             "Note:\n"
             "    Higher resolution = better accuracy but slower\n\n"
             "Raises:\n"
             "    ValueError: If resolution is not 0-3\n"
             "    RuntimeError: If I2C communication fails")

        .def("set_emissivity", &MLX90640Camera::set_emissivity,
             py::arg("emissivity"),
             "Set emissivity\n\n"
             "Args:\n"
             "    emissivity: 0.1-1.0\n\n"
             "Common values:\n"
             "    1.0  = Perfect blackbody\n"
             "    0.95 = Human skin\n"
             "    0.90 = Matte surfaces\n"
             "    0.80 = Wood\n\n"
             "Raises:\n"
             "    ValueError: If emissivity is not 0.1-1.0")

        .def("get_frame",
             [](MLX90640Camera &cam, bool interpolate_outliers, bool correct_bad_pixels) {
                 // Call C++ get_frame which returns a raw pointer to temp_buffer
                 float* data = cam.get_frame(interpolate_outliers, correct_bad_pixels);

                 // Create NumPy array wrapping the internal buffer with camera lifetime
                 py::array_t<float> result({768}, {sizeof(float)}, data, py::cast(cam));
                 result.attr("flags").attr("writeable") = false;  // Make read-only for safety
                 return result;
             },
             py::arg("interpolate_outliers") = true,
             py::arg("correct_bad_pixels") = true,
             "Capture frame (blocking, self-paced by sensor)\n\n"
             "This call blocks until the sensor has new data ready.\n"
             "The timing is controlled by the sensor's refresh rate.\n"
             "In chess mode, alternates between subpages 0 and 1.\n\n"
             "Args:\n"
             "    interpolate_outliers: Apply outlier interpolation (default: True)\n"
             "    correct_bad_pixels: Apply bad pixel correction (default: True)\n\n"
             "Returns:\n"
             "    NumPy array of 768 floats representing temperatures in °C\n"
             "    Layout: 24 rows x 32 columns, row-major order\n"
             "    Index formula: pixel = row * 32 + col\n"
             "    Use .reshape((24, 32)) for 2D access\n\n"
             "Raises:\n"
             "    RuntimeError: If not initialized or frame capture fails")

        .def("get_refresh_rate", &MLX90640Camera::get_refresh_rate,
             "Get current refresh rate register value\n\n"
             "Returns:\n"
             "    Refresh rate code (see datasheet)")

        .def("get_resolution", &MLX90640Camera::get_resolution,
             "Get current ADC resolution\n\n"
             "Returns:\n"
             "    Resolution code (0-3)")

        .def("get_emissivity", &MLX90640Camera::get_emissivity,
             "Get current emissivity\n\n"
             "Returns:\n"
             "    Emissivity value (0.1-1.0)")

        .def("is_initialized", &MLX90640Camera::is_initialized,
             "Check if camera is initialized\n\n"
             "Returns:\n"
             "    True if initialized, False otherwise")

        .def("get_subpage_number", &MLX90640Camera::get_subpage_number,
             "Get subpage number from last captured frame\n\n"
             "In chess mode, the sensor alternates between subpage 0 and 1.\n"
             "Each subpage contains half the pixels in a checkerboard pattern.\n\n"
             "Returns:\n"
             "    0 or 1 for subpage number, -1 if no frame captured yet");
}
