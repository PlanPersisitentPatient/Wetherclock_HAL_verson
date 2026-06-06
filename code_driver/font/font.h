#ifndef __font_h
#define __font_h



#include "headfile.h"

typedef struct
{
	const char *name;
	const uint8_t *model;
}font_chinese_t;

typedef struct
{
	uint16_t size;
    const char *ascii_map;
    const uint8_t *ascii_model;
  const font_chinese_t *chinese;
}font_t;

extern const font_t font32_maple_bold;
extern const font_t font20_maple_bold;
extern const font_t font24_maple_semibold;
extern const font_t font76_maple_extrabold;
extern const font_t font16_maple;

#endif