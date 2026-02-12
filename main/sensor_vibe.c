#include "sensor_vibe.h"
#include "esp_dsp.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include <math.h>

// MPU6050 寄存器地址
#define MPU6050_PWR_MGMT_1          0x6B
#define MPU6050_ACCEL_XOUT_H        0x3B

// FFT 参数：128点采样，250Hz采样率
#define VIBE_FFT_SIZE 128
static float vibe_input[VIBE_FFT_SIZE * 2];
static float vibe_window[VIBE_FFT_SIZE];

/** I2C 读写辅助函数保持不变 **/
static esp_err_t mpu6050_register_write_byte(uint8_t reg_addr, uint8_t data) {
    uint8_t write_buf[2] = {reg_addr, data};
    return i2c_master_write_to_device(I2C_MASTER_NUM, MPU6050_ADDR, write_buf, sizeof(write_buf), pdMS_TO_TICKS(100));
}

static esp_err_t mpu6050_register_read(uint8_t reg_addr, uint8_t *data, size_t len) {
    return i2c_master_write_read_device(I2C_MASTER_NUM, MPU6050_ADDR, &reg_addr, 1, data, len, pdMS_TO_TICKS(100));
}

esp_err_t vibe_init(void) {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 400000, 
    };
    i2c_param_config(I2C_MASTER_NUM, &conf);
    i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);

    // 唤醒并初始化 FFT 窗函数
    dsps_fft2r_init_fc32(NULL, CONFIG_DSP_MAX_FFT_SIZE);
    dsps_wind_hann_f32(vibe_window, VIBE_FFT_SIZE);

    return mpu6050_register_write_byte(MPU6050_PWR_MGMT_1, 0x00);
}

/**
 * 核心功能：一次性计算震动幅度和频率
 * @param out_amplitude 输出幅度指针
 * @param out_frequency 输出频率指针
 */
void vibe_get_data(float *out_amplitude, float *out_frequency) {
    uint8_t data[6];
    float sum_accel = 0;
    float temp_buffer[VIBE_FFT_SIZE];

    // 1. 恒定频率采样数据
    for (int i = 0; i < VIBE_FFT_SIZE; i++) {
        if (mpu6050_register_read(MPU6050_ACCEL_XOUT_H, data, 6) == ESP_OK) {
            int16_t ax = (data[0] << 8) | data[1];
            int16_t ay = (data[2] << 8) | data[3];
            int16_t az = (data[4] << 8) | data[5];

            // 计算合加速度
            float accel_m = sqrtf((float)ax*ax + (float)ay*ay + (float)az*az);
            temp_buffer[i] = accel_m;
            sum_accel += accel_m;
        }
        esp_rom_delay_us(1000); // 1000Hz 采样率
    }

    // 2. 动态去直流 (减去本次采样的平均值，只留震动部分)
    float avg_accel = sum_accel / VIBE_FFT_SIZE;
    float max_val = -999999, min_val = 999999;

    for (int i = 0; i < VIBE_FFT_SIZE; i++) {
        float ac_component = temp_buffer[i] - avg_accel; // 震动分量
        
        // 用于计算幅度 (Peak-to-Peak)
        if (ac_component > max_val) max_val = ac_component;
        if (ac_component < min_val) min_val = ac_component;

        // 用于 FFT 输入
        vibe_input[i * 2] = ac_component * vibe_window[i]; 
        vibe_input[i * 2 + 1] = 0;
    }

    // 幅度计算：峰峰值 (Peak-to-Peak)
    *out_amplitude = max_val - min_val;

    // 3. 执行 FFT
    dsps_fft2r_fc32(vibe_input, VIBE_FFT_SIZE);
    dsps_bit_rev_fc32(vibe_input, VIBE_FFT_SIZE);

    // 4. 计算频谱幅度并找最大频点
    int max_idx = 1;
    float max_mag = 0;
    for (int i = 1; i < VIBE_FFT_SIZE / 2; i++) {
        float re = vibe_input[i * 2];
        float im = vibe_input[i * 2 + 1];
        float mag = sqrtf(re * re + im * im);
        if (mag > max_mag) {
            max_mag = mag;
            max_idx = i;
        }
    }

    // 5. 频率输出：如果能量太小则认为没有震动
    if (max_mag < 100.0f || *out_amplitude < 50.0f) {
        *out_frequency = 0;
    } else {
        *out_frequency = (float)max_idx * 500.0f / VIBE_FFT_SIZE;
    }
}