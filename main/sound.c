#include "sound.h"
#include "driver/uart.h"
#include "esp_timer.h"
#include <string.h>

#define I2S_PORT I2S_NUM_0
#define SAMPLE_RATE 16000
#define DMA_BUF_LEN 256

#define UART_PORT UART_NUM_0

static int32_t i2s_buf[DMA_BUF_LEN];
static int16_t audio_buf[DMA_BUF_LEN];

void sound_init(void)
{
    i2s_config_t i2s_config = {
        .mode = I2S_MODE_MASTER | I2S_MODE_RX,
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .dma_buf_count = 8,
        .dma_buf_len = DMA_BUF_LEN,
        .use_apll = false,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1
    };

    i2s_pin_config_t pin_config = {
        .bck_io_num = 4,
        .ws_io_num = 5,
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num = 6
    };

    i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_PORT, &pin_config);
}

void sound_send_chunk(void)
{
    size_t bytes_read;

    i2s_read(I2S_PORT, i2s_buf, sizeof(i2s_buf), &bytes_read, portMAX_DELAY);

    int samples = bytes_read / 4;

    for (int i = 0; i < samples; i++)
    {
        audio_buf[i] = (int16_t)(i2s_buf[i] >> 14);
    }

    int64_t timestamp = esp_timer_get_time();  // 微秒

    // 发送头
    uint8_t header[4] = {'A','U','D','0'};
    uart_write_bytes(UART_PORT, (const char*)header, 4);

    // 发送时间戳
    uart_write_bytes(UART_PORT,
                     (const char*)&timestamp,
                     sizeof(int64_t));

    // 发送音频数据
    uart_write_bytes(UART_PORT,
                     (const char*)audio_buf,
                     samples * sizeof(int16_t));
}

int sound_read_sample(int16_t *out_sample)
{
    size_t bytes_read;

    int32_t raw_sample;

    i2s_read(I2S_PORT,
             &raw_sample,
             sizeof(raw_sample),
             &bytes_read,
             portMAX_DELAY);

    if (bytes_read == sizeof(raw_sample))
    {
        *out_sample = (int16_t)(raw_sample >> 14);
        return 1;
    }

    return 0;
}
