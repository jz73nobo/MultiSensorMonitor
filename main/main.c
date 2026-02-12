#include <stdio.h>
#include "driver/uart.h"
#include "sound.h"
#include "sensor_vibe.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define UART_PORT UART_NUM_0
#define UART_BAUD 921600

// ---------- 音频任务 ----------
void audio_task(void *arg)
{
    while (1)
    {
        sound_send_chunk();
    }
}

// ---------- 振动任务 ----------
void vibe_task(void *arg)
{
    float amp, freq;

    while (1)
    {
        vibe_get_data(&amp, &freq);

        char line[64];
        int len = snprintf(line, sizeof(line),
                           "VIB,%.2f,%.2f\n", amp, freq);

        uart_write_bytes(UART_PORT, line, len);

        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

void app_main(void)
{
    uart_config_t uart_config = {
        .baud_rate = UART_BAUD,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };

    uart_driver_install(UART_PORT, 8192, 0, 0, NULL, 0);
    uart_param_config(UART_PORT, &uart_config);

    sound_init();
    vibe_init();

    xTaskCreate(audio_task, "audio_task", 4096, NULL, 5, NULL);
    xTaskCreate(vibe_task,  "vibe_task",  4096, NULL, 3, NULL);
}
