/**
 * MLX90640 Thermal Camera C++ Wrapper
 *
 * Chess mode only (sensor calibrated for this mode)
 * Self-pacing frame capture via blocking I2C calls
 */

#pragma once

#include <stdint.h>
#include <vector>
#include <MLX90640_API.h>

class MLX90640Camera {
private:
    paramsMLX90640 params;
    uint16_t eeprom[832];
    uint16_t frame_buffer[834];
    float temp_buffer[768];
    float emissivity;
    uint8_t i2c_addr;
    bool initialized;

public:
    /**
     * Create camera instance
     * @param addr I2C address (default: 0x33)
     */
    MLX90640Camera(uint8_t addr = 0x33);

    /**
     * Destructor - cleanup resources
     */
    ~MLX90640Camera();

    /**
     * Initialize camera (reads EEPROM, configures chess mode)
     * @return 0 on success, error code otherwise
     * @throws runtime_error on failure
     */
    int init();

    /**
     * Cleanup camera resources
     */
    void cleanup();

    /**
     * Set refresh rate
     * @param fps Frame rate in Hz (1,2,4,8,16,32,64)
     * @return 0 on success, error code otherwise
     * @throws invalid_argument for invalid FPS
     * @throws runtime_error on I2C failure
     */
    int set_refresh_rate(int fps);

    /**
     * Set ADC resolution
     * @param res Resolution (0=16bit, 1=17bit, 2=18bit, 3=19bit)
     * @return 0 on success, error code otherwise
     * @throws invalid_argument for invalid resolution
     * @throws runtime_error on I2C failure
     */
    int set_resolution(uint8_t res);

    /**
     * Set emissivity
     * @param emis Emissivity value (0.1-1.0, 1.0=blackbody, 0.95=skin)
     * @throws invalid_argument for out of range value
     */
    void set_emissivity(float emis);

    /**
     * Capture frame (blocking, self-paced by sensor) - returns raw pointer for NumPy
     * @param interpolate_outliers Apply outlier interpolation
     * @param correct_bad_pixels Apply bad pixel correction
     * @return Pointer to 768 temperatures in °C (24x32, row-major)
     * @throws runtime_error if not initialized or frame capture fails
     */
    float* get_frame(bool interpolate_outliers = true,
                     bool correct_bad_pixels = true);

    /**
     * Get current refresh rate
     * @return Refresh rate register value
     */
    int get_refresh_rate() const;

    /**
     * Get current resolution
     * @return Resolution register value
     */
    int get_resolution() const;

    /**
     * Get current emissivity
     * @return Emissivity value
     */
    float get_emissivity() const;

    /**
     * Check if camera is initialized
     * @return true if initialized
     */
    bool is_initialized() const { return initialized; }

    /**
     * Get subpage number from last captured frame
     * In chess mode, alternates between 0 and 1
     * @return Subpage number (0 or 1), or -1 if no frame captured yet
     */
    int get_subpage_number() const;
};
