#include <stdint.h>
#include <iostream>
#include <cstring>
#include <fstream>
#include <chrono>
#include <thread>
#include "headers/MLX90640_API.h"

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_NONE    "\x1b[30m"
#define ANSI_COLOR_RESET   "\x1b[0m"

//#define FMT_STRING "%+06.2f "
#define FMT_STRING "\u2588\u2588"

#define MLX_I2C_ADDR 0x33

int main(){
    int scale = 1; // Horizontal scaling factor (1-4 recommended)
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
                //std::cout << image[32 * y + x] << ",";
                float val = mlx90640To[32 * (23-y) + x];
                if(val > 99.99) val = 99.99;
                const char* color;
                if(val > 32.0){
                    color = ANSI_COLOR_MAGENTA;
                }
                else if(val > 29.0){
                    color = ANSI_COLOR_RED;
                }
                else if (val > 26.0){
                    color = ANSI_COLOR_YELLOW;
                }
                else if ( val > 20.0 ){
                    color = ANSI_COLOR_NONE;
                }
                else if (val > 17.0) {
                    color = ANSI_COLOR_GREEN;
                }
                else if (val > 10.0) {
                    color = ANSI_COLOR_CYAN;
                }
                else {
                    color = ANSI_COLOR_BLUE;
                }
                printf("%s", color);
                for(int s = 0; s < scale; s++){
                    printf(FMT_STRING, val);
                }
                printf(ANSI_COLOR_RESET);
            }
            std::cout << std::endl;
        }
        //std::this_thread::sleep_for(std::chrono::milliseconds(20));
        printf("\x1b[25A");
    }
    return 0;
}
