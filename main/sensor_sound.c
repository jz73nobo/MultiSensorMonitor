#include "sensor_sound.h"
#include <stdlib.h>
#include <math.h>      // 必须包含，用于 sqrtf
#include "esp_dsp.h"
#include "esp_log.h"

i2s_chan_handle_t rx_handle = NULL;

// FFT 配置
#define FFT_SIZE 1024
static float fft_input[FFT_SIZE * 2]; // 复数数组 [实部, 虚部, 实部, 虚部...]
static float fft_output[FFT_SIZE];
static float fft_window[FFT_SIZE];

void sound_init(void) {
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    i2s_new_channel(&chan_cfg, NULL, &rx_handle);

    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(16000), 
        .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = MIC_I2S_SCK,
            .ws   = MIC_I2S_WS,
            .din  = MIC_I2S_SD,
            .dout = I2S_GPIO_UNUSED,
        },
    };
    i2s_channel_init_std_mode(rx_handle, &std_cfg);
    i2s_channel_enable(rx_handle);

    // 初始化 DSP 库
    esp_err_t ret = dsps_fft2r_init_fc32(NULL, CONFIG_DSP_MAX_FFT_SIZE);
    if (ret != ESP_OK) {
        ESP_LOGE("DSP", "无法初始化 FFT 缓冲区");
    }
    dsps_wind_hann_f32(fft_window, FFT_SIZE);
}

// 获取主频率（单位：Hz）
float sound_get_frequency(void) {
    // 加上 static，把 4KB 的数组从栈移到静态区
    static int32_t samples[FFT_SIZE];
    size_t bytes_read = 0;
    
    if (i2s_channel_read(rx_handle, samples, sizeof(samples), &bytes_read, 100) == ESP_OK) {
        int count = bytes_read / sizeof(int32_t);
        
        for (int i = 0; i < FFT_SIZE; i++) {
            if (i < count) {
                int32_t s = (samples[i] << 8) >> 8; 
                fft_input[i * 2] = (float)s * fft_window[i];
                fft_input[i * 2 + 1] = 0;
            } else {
                fft_input[i * 2] = 0;
                fft_input[i * 2 + 1] = 0;
            }
        }

        // 1. 执行快速傅里叶变换
        dsps_fft2r_fc32(fft_input, FFT_SIZE);
        dsps_bit_rev_fc32(fft_input, FFT_SIZE);

        // 2. --- 核心修改：手动计算模长 (绕过链接报错) ---
        // 幅度 = sqrt(实部^2 + 虚部^2)
        for (int i = 0; i < FFT_SIZE; i++) {
            float real = fft_input[i * 2];
            float imag = fft_input[i * 2 + 1];
            fft_output[i] = sqrtf(real * real + imag * imag);
        }

        // 3. 寻找最大能量对应的索引
        int max_idx = 1;
        float max_mag = 0;
        for (int i = 1; i < FFT_SIZE / 2; i++) {
            if (fft_output[i] > max_mag) {
                max_mag = fft_output[i];
                max_idx = i;
            }
        }

        return (float)max_idx * 16000.0 / FFT_SIZE;
    }
    return 0;
}

int sound_get_level(void) {
    int32_t samples[128];
    size_t bytes_read = 0;
    
    if (i2s_channel_read(rx_handle, samples, sizeof(samples), &bytes_read, 100) == ESP_OK) {
        int count = bytes_read / sizeof(int32_t);
        if (count == 0) return 0;

        long long energy_sum = 0;
        for (int i = 0; i < count; i++) {
            int32_t s = (samples[i] << 8) >> 8;
            energy_sum += abs(s);
        }
        
        int result = (int)(energy_sum / count);
        int offset = 300000; 
        result = result - offset;
        if (result < 0) result = 0;

        return result / 100;
    }
    return 0;
}