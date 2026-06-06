#ifndef __st7789_h
#define __st7789_h

#include "font.h"
#include "image.h"
#include "headfile.h"


#define mkcolor(r, g, b) (((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3))  //把24位的RGB格式转换为RGB565的格式
//具体做法是红色保留高5位、绿色高6位、蓝色高5位，并按照rgb的顺序拼起来得到16位的RGB565格式

#define ST7789_WIDTH    240
#define ST7789_HEIGHT   320

void st7789_init_display(void);
void st7789_fill_color(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
void st7789_write_ascii(uint16_t x, uint16_t y, char *str, uint16_t color, uint16_t bg_color, const font_t *font);
void st7789_write_string(uint16_t x, uint16_t y, const char *str, uint16_t color, uint16_t bg_color, const font_t *font);
void st7789_draw_image(uint16_t x, uint16_t y, const image_t *image);

#endif