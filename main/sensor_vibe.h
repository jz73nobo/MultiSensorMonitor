#ifndef SENSOR_VIBE_H
#define SENSOR_VIBE_H

#include "driver/i2c.h"

#define I2C_MASTER_SCL_IO 1
#define I2C_MASTER_SDA_IO 2
#define I2C_MASTER_NUM    I2C_NUM_0
#define MPU6050_ADDR      0x68

esp_err_t vibe_init(void);
void vibe_get_data(float *amp, float *freq);

#endif
