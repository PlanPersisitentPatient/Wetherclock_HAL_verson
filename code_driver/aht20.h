#ifndef __aht20_h
#define __aht20_h

#include "stdbool.h"

bool aht20_init(void);
bool aht20_start_measurement(void);
bool aht20_wait_for_measurement(void);
bool aht20_read_measurement(float *temperature, float *humidity);


#endif