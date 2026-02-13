// #include <stdio.h>
// #include "driver/uart.h"
// #include "sound.h"
// #include "sensor_vibe.h"
// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "esp_timer.h"

// #define UART_PORT UART_NUM_0
// #define UART_BAUD 921600

// // ---------- 音频任务 ----------
// void audio_task(void *arg)
// {
//     while (1)
//     {
//         sound_send_chunk();
//     }
// }


// // ---------- 振动任务 ----------
// void vibe_task(void *arg)
// {
//     float amp, freq;

//     while (1)
//     {
//         int64_t timestamp = esp_timer_get_time();

//         vibe_get_data(&amp, &freq);

//         char line[96];
//         int len = snprintf(line, sizeof(line),
//                            "VIB,%lld,%.2f,%.2f\n",
//                            timestamp, amp, freq);

//         uart_write_bytes(UART_PORT, line, len);

//         vTaskDelay(pdMS_TO_TICKS(100));
//     }
// }


// void app_main(void)
// {
//     uart_config_t uart_config = {
//         .baud_rate = UART_BAUD,
//         .data_bits = UART_DATA_8_BITS,
//         .parity = UART_PARITY_DISABLE,
//         .stop_bits = UART_STOP_BITS_1,
//         .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
//     };

//     uart_driver_install(UART_PORT, 8192, 0, 0, NULL, 0);
//     uart_param_config(UART_PORT, &uart_config);

//     sound_init();
//     vibe_init();

//     xTaskCreate(audio_task, "audio_task", 4096, NULL, 5, NULL);
//     xTaskCreate(vibe_task,  "vibe_task",  4096, NULL, 3, NULL);
// }

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "sound.h"
#include "sensor_vibe.h"

#define UART_PORT UART_NUM_0

void audio_task(void *arg)
{
    int16_t sample;
    int counter = 0;

    while (1)
    {
        // 确保 sound_read_sample 是非阻塞的，或者能让出 CPU
        if (sound_read_sample(&sample))
        {
            counter++;
            // 降低 Teleplot 的刷新压力，降采样到约 500Hz (16000 / 32)
            if (counter >= 32) 
            {
                counter = 0;
                // Teleplot 格式: >变量名:数值
                printf(">audio:%d\n", (int)sample);
            }
        }
        else 
        {
            // 如果读不到数据，必须让出 CPU，否则会卡死 Log
            vTaskDelay(pdMS_TO_TICKS(1));
        }
    }
}

void vibe_task(void *arg)
{
    float amp, freq;
    while (1)
    {
        vibe_get_data(&amp, &freq);

        // 使用 Teleplot 格式输出
        printf(">vibe_amp:%.2f\n", amp);
        printf(">vibe_freq:%.2f\n", freq);

        // 振动不需要太快，20Hz 足够看趋势
        vTaskDelay(pdMS_TO_TICKS(50)); 
    }
}

void app_main(void)
{
    // 注意：ESP-IDF 默认已经初始化了 UART0 用于 printf
    // 如果你要改波特率，建议在项目的 sdkconfig 里改，或者手动重设
    
    sound_init();
    vibe_init();

    // 创建任务
    // 给 audio_task 稍微低一点的优先级 (或者确保它内部有延时)
    xTaskCreate(audio_task, "audio_task", 4096, NULL, 5, NULL);
    xTaskCreate(vibe_task,  "vibe_task",  4096, NULL, 3, NULL);
}