#ifndef SENSOR_SOUND_H
#define SENSOR_SOUND_H

#include "driver/i2s.h"

void sound_init(void);
void sound_send_chunk(void);

#endif
