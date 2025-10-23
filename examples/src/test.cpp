#include <stdint.h>
#include <iostream>
#include <cstring>
#include <fstream>
#include <chrono>
#include <thread>
#include <cmath>
#include "headers/MLX90640_API.h"

#define FMT_STRING "\u2588\u2588"
#define MLX_I2C_ADDR 0x33

// Inferno colormap - perceptually uniform colormap
// Maps value from 0.0 to 1.0 to RGB
void inferno_colormap(float value, int &r, int &g, int &b) {
    // Clamp value to [0, 1]
    if (value < 0.0f) value = 0.0f;
    if (value > 1.0f) value = 1.0f;

    // Inferno colormap control points (sampled at key positions)
    const int NUM_COLORS = 9;
    static const float colors[NUM_COLORS][3] = {
        {0.001462, 0.000466, 0.013866},  // 0.0 - dark purple/black
        {0.087411, 0.044556, 0.224813},  // 0.125 - deep purple
        {0.258234, 0.038571, 0.406485},  // 0.25 - purple
        {0.416331, 0.090203, 0.432943},  // 0.375 - purple-red
        {0.645581, 0.133503, 0.392508},  // 0.5 - red
        {0.798216, 0.280197, 0.469538},  // 0.625 - orange-red
        {0.924870, 0.517763, 0.295662},  // 0.75 - orange
        {0.987622, 0.809330, 0.145357},  // 0.875 - yellow-orange
        {0.988362, 0.998364, 0.644924}   // 1.0 - bright yellow
    };

    // Find the two colors to interpolate between
    float scaled = value * (NUM_COLORS - 1);
    int idx1 = (int)scaled;
    int idx2 = idx1 + 1;

    if (idx1 >= NUM_COLORS - 1) {
        idx1 = idx2 = NUM_COLORS - 1;
    }

    float frac = scaled - idx1;

    // Linear interpolation
    float rf = colors[idx1][0] + (colors[idx2][0] - colors[idx1][0]) * frac;
    float gf = colors[idx1][1] + (colors[idx2][1] - colors[idx1][1]) * frac;
    float bf = colors[idx1][2] + (colors[idx2][2] - colors[idx1][2]) * frac;

    r = (int)(rf * 255.0f);
    g = (int)(gf * 255.0f);
    b = (int)(bf * 255.0f);
}

int main(){
    int scale = 1; // Horizontal scaling factor (1-4 recommended)
    float temp_min = 15.0f; // Minimum temperature for colormap (°C)
    float temp_max = 35.0f; // Maximum temperature for colormap (°C)

    int state = 0;
    printf("Starting...\n");
    static uint16_t eeMLX90640[832];
    float emissivity = 1;
    uint16_t frame[834];
    float eTa;

    std::fstream fs;

    MLX90640_SetDeviceMode(MLX_I2C_ADDR, 0);
    MLX90640_SetSubPageRepeat(MLX_I2C_ADDR, 0);
    MLX90640_SetRefreshRate(MLX_I2C_ADDR, 0b101);
    MLX90640_SetChessMode(MLX_I2C_ADDR);
    //MLX90640_SetSubPage(MLX_I2C_ADDR, 0);
    printf("Configured...\n");

    paramsMLX90640 mlx90640;
    MLX90640_DumpEE(MLX_I2C_ADDR, eeMLX90640);
    MLX90640_ExtractParameters(eeMLX90640, &mlx90640);

    int refresh = MLX90640_GetRefreshRate(MLX_I2C_ADDR);
    (void)refresh;
    printf("EE Dumped...\n");

    int subpage;
    static float mlx90640To[768];

    // FPS calculation variables
    auto last_time = std::chrono::steady_clock::now();
    int frame_count = 0;
    float fps = 0.0;

    while (1){
        state = !state;
        //printf("State: %d \n", state);
        MLX90640_GetFrameData(MLX_I2C_ADDR, frame);
        MLX90640_InterpolateOutliers(frame, eeMLX90640);
        eTa = MLX90640_GetTa(frame, &mlx90640);
        subpage = MLX90640_GetSubPageNumber(frame);
        MLX90640_CalculateTo(frame, &mlx90640, emissivity, eTa, mlx90640To);

        MLX90640_BadPixelsCorrection((&mlx90640)->brokenPixels, mlx90640To, 1, &mlx90640);
        MLX90640_BadPixelsCorrection((&mlx90640)->outlierPixels, mlx90640To, 1, &mlx90640);

        // Calculate FPS
        frame_count++;
        auto current_time = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - last_time).count();
        if(elapsed >= 1000) {
            fps = frame_count * 1000.0 / elapsed;
            frame_count = 0;
            last_time = current_time;
        }

        printf("Subpage: %d | FPS: %.2f\n", subpage, fps);
        // MLX90640_SetSubPage(MLX_I2C_ADDR,!subpage);

        for(int y = 0; y < 24; y++){
            for(int x = 0; x < 32; x++){
                float val = mlx90640To[32 * (23-y) + x];

                // Normalize temperature to 0-1 range
                float normalized = (val - temp_min) / (temp_max - temp_min);

                // Get RGB color from Inferno colormap
                int r, g, b;
                inferno_colormap(normalized, r, g, b);

                // Print using ANSI 24-bit true color
                printf("\x1b[38;2;%d;%d;%dm", r, g, b);
                for(int s = 0; s < scale; s++){
                    printf(FMT_STRING, val);
                }
                printf("\x1b[0m");
            }
            std::cout << std::endl;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        printf("\x1b[25A");
    }
    return 0;
}
