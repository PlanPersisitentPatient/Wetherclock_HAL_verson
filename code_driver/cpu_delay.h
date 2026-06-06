#ifndef __cpu_delay_h
#define __cpu_delay_h

#include "headfile.h"

typedef void (*cpu_periodic_callback_t)(void);

void cpu_delay(uint32_t us);
void cpu_delay_ms(uint32_t ms);
void cpu_register_periodic_callback(cpu_periodic_callback_t callback);

#endif