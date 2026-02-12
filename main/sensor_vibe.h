#ifndef SENSOR_VIBE_H
#define SENSOR_VIBE_H

#include "driver/i2c.h"

// 引脚根据之前的建议配置
#define I2C_MASTER_SCL_IO           1      
#define I2C_MASTER_SDA_IO           2      
#define I2C_MASTER_NUM              I2C_NUM_0
#define MPU6050_ADDR                0x68   // AD0接地

esp_err_t vibe_init(void);
void vibe_get_data(float *out_amplitude, float *out_frequency);

#endif