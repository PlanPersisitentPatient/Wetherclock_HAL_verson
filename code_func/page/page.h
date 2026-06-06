#ifndef __PAGE_H__
#define __PAGE_H__

#include "stdint.h"
#include "stdbool.h"
#include "rtc_driver.h"


void error_page_display(const char *msg);
void welcome_page_display(void);
void wifi_page_display(void);
void main_page_display(void);


void main_page_redraw_wifi_ssid(const char *ssid);
void main_page_redraw_time(rtc_date_time_t *time);
void main_page_redraw_date(rtc_date_time_t *date);
void main_page_redraw_inner_temperature(float temperature);
void main_page_redraw_inner_humidity(float humidity);
void main_page_redraw_outdoor_city(const char *city);
void main_page_redraw_outdoor_temperature(float temperature);
void main_page_redraw_outdoor_weather_icon(const int code);


//void temp_test_main_page_redraw_time(rtc_date_time_t *time);

#endif /* __PAGE_H__ */