// #include <stdio.h>
// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "esp_log.h"

// // 引入你那四个文件的头文件
// #include "sensor_vibe.h"
// #include "sensor_sound.h"

// static const char *TAG = "MAIN_APP";

// void app_main(void)
// {
//     // 1. 初始化所有传感器
//     ESP_LOGI(TAG, "Initializing sensors...");
    
//     // 初始化震动传感器 (I2C + MPU6050 + DSP)
//     if (vibe_init() != ESP_OK) {
//         ESP_LOGE(TAG, "Vibe sensor init failed!");
//     }

//     // 初始化声音传感器 (I2S + MIC + DSP)
//     sound_init();

//     ESP_LOGI(TAG, "All sensors initialized. Starting Teleplot stream...");

//     // 2. 数据循环
//     float v_amplitude = 0;
//     float v_frequency = 0;
//     float s_frequency = 0;
//     int s_level = 0;

//     while (1) {
//         // --- 采集震动数据 ---
//         // 内部包含 256 点 FFT 计算
//         vibe_get_data(&v_amplitude, &v_frequency);

//         // --- 采集声音数据 ---
//         // 内部包含 1024 点 FFT 计算
//         s_frequency = sound_get_frequency();
//         s_level = sound_get_level();

//         // --- Teleplot 格式输出 ---
//         // 格式说明：>变量名:数值\n
//         // 1. 震动幅度
//         printf(">Vibe_Amp:%.2f\n", v_amplitude);
//         // 2. 震动主频
//         printf(">Vibe_Freq:%.2f\n", v_frequency);
//         // 3. 声音强度
//         printf(">Sound_Level:%d\n", s_level);
//         // 4. 声音主频
//         printf(">Sound_Freq:%.2f\n", s_frequency);

//         // 控制刷新率
//         // 采样和处理已经耗费了一定时间，这里微调延时即可
//         vTaskDelay(pdMS_TO_TICKS(10));
//     }
// }


#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2s_std.h"
#include "driver/uart.h"
#include "sensor_vibe.h"
#include "sensor_sound.h"

void app_main(void) {
    uart_set_baudrate(UART_NUM_0, 921600); // 确保波特率够高
    vibe_init();
    sound_init();

    float v_amp = 0, v_freq = 0;
    int32_t sample_raw;
    size_t rb;
    int count = 0;

    while (1) {
        // --- 1. 音频：每一轮疯狂读 100 个点 ---
        for(int i = 0; i < 100; i++) {
            if (i2s_channel_read(rx_handle, &sample_raw, sizeof(int32_t), &rb, 0) == ESP_OK && rb > 0) {
                printf(">R:%d\n", (int16_t)(sample_raw >> 16));
            }
        }

        // --- 2. 振动：降低打印频率，每 50 轮打一次 ---
        // 这样不会堵塞串口，CSV 的时间戳也会变得密集
        if (count++ % 50 == 0) {
            vibe_get_data(&v_amp, &v_freq);
            printf(">V:%.2f\n", v_amp);
        }

        // --- 3. 极其重要：延时必须是 1ms ---
        vTaskDelay(1); 
    }
}