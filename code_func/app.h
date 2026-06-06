#ifndef __app_h
#define __app_h


#include "stdbool.h"


//用于weather.c，存储调用api后所获取城市的天气信息
typedef struct
{
	char city[32];
	char loaction[128];
	char weather[16];
	int weather_code;
	float temperature;
} weather_info_t;


//用于wifi.c
#define WIFI_SSID   "706"
#define WIFI_PASSWD "706706706"


void wifi_init(void);
void wifi_wait_connect(void);
bool parse_seniverse_response(const char *response, weather_info_t *info);
void main_loop(void);
void main_loop_init(void);




#endif