#ifndef SENSOR_SOUND_H
#define SENSOR_SOUND_H

#include "driver/i2s_std.h"

// 引脚定义
#define MIC_I2S_SCK  GPIO_NUM_39
#define MIC_I2S_WS   GPIO_NUM_40
#define MIC_I2S_SD   GPIO_NUM_41

void sound_init(void);
int  sound_get_level(void); // 返回当前音量强度
float sound_get_frequency(void);

extern i2s_chan_handle_t rx_handle;

#endif