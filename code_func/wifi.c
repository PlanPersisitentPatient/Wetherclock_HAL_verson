#include "headfile.h"


/**
 * @brief 无线系统初始化（AT通信、WiFi、SNTP时间）
 * @note 任意模块初始化失败，统一执行错误处理
 */
void wifi_init(void)
{
    if (!esp_at_init())
    {
        printf("[AT] init failed\n");
        goto err;
    }
    printf("[AT] inited\n");
    
    if (!esp_at_wifi_init())
    {
        printf("[WIFI] init failed\n");
        goto err;
    }
    printf("[WIFI] inited\n");
    


		if(!esp_at_sntp_init())
		{
			printf("[SNTP] init failed\n");
			goto err;		
		}
		
		printf("[SNTP] init success\n");
		
//		esp_date_time_t date;
//		
//		if(!esp_at_sntp_get_time(&date))
//		{
//			printf("[SNTP] get time failed\n");
//			goto err;	
//		}
		
//		printf("[SNTP] get time success\n");
//		printf("[SNTP] %04u-%02u-%02u %02u:%02u:%02u (%s)\n",
//        date.year, date.month, date.day, date.hour, date.minute, date.second,
//        date.weekday == 1 ? "Monday" :
//        date.weekday == 2 ? "Tuesday" :
//        date.weekday == 3 ? "Wednesday" :
//        date.weekday == 4 ? "Thursday" :
//        date.weekday == 5 ? "Friday" :
//        date.weekday == 6 ? "Saturday" :
//        date.weekday == 7 ? "Sunday" : "Unknown");
		
    return;
    
err:
    error_page_display("wireless init failed");
    while (1)
    {
        ;
    }
}


/**
 * @brief 等待WiFi连接，超时10秒，连接失败则报错并停机
 */
void wifi_wait_connect(void)
{
    printf("[WIFI] connecting\n");
    
    esp_at_connect_wifi(WIFI_SSID, WIFI_PASSWD, NULL);
    
	//循环等待：总时长10秒，每100ms检查一次连接状态
    for (uint32_t t = 0; t < 10 * 1000; t += 100)
    {
				HAL_Delay(100); //TODO:注意1：看看这里用HAL库的有没有可能有问题
				//cpu_delay_ms(1);
        esp_wifi_info_t wifi = { 0 };
        if (esp_at_get_wifi_info(&wifi) && wifi.connected)
        {
            printf("[WIFI] Connected\n");
            printf("[WIFI] SSID: %s, BSSID: %s, Channel: %d, RSSI: %d\n",
                wifi.ssid, wifi.bssid, wifi.channel, wifi.rssi);
            return;
        }
    }
    
    printf("[WIFI] Connection Timeout\n");
    error_page_display("wireless connect failed");
    while (1)
    {
        ;
    }
}