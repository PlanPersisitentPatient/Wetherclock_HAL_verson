#include "headfile.h"



#define MILLISECONDS(x) (x)
#define SECONDS(x)      MILLISECONDS((x) * 1000)
#define MINUTES(x)      SECONDS((x) * 60)
#define HOURS(x)        MINUTES((x) * 60)
#define DAYS(x)          HOURS((x) * 24)

#define TIME_SYNC_INTERVAL          DAYS(1)
#define WIFI_UPDATE_INTERVAL        SECONDS(5)
#define TIME_UPDATE_INTERVAL        SECONDS(1)
#define INNER_UPDATE_INTERVAL       SECONDS(3)
#define OUTDOOR_UPDATE_INTERVAL     MINUTES(1)

static uint32_t time_sync_delay = 0;
static uint32_t wifi_update_delay = 0;
static uint32_t time_update_delay = 0;
static uint32_t inner_update_delay = 0;
static uint32_t outdoor_update_delay = 0;

/**
 * @brief  系统周期回调函数（软定时器核心）
 * @note   每1ms被TIM2中断调用一次，对所有延时器进行倒计时
 * @param  无
 * @retval 无
 */
static void cpu_periodic_callback(void)
{
    // 所有软定时器延时变量>0时，自减1（毫秒级倒计时）
    if (time_sync_delay > 0)
        time_sync_delay--;
    if (wifi_update_delay > 0)
        wifi_update_delay--;
    if (time_update_delay > 0)
        time_update_delay--;
    if (inner_update_delay > 0)
        inner_update_delay--;
    if (outdoor_update_delay > 0)
        outdoor_update_delay--;
}

/**
 * @brief  主循环初始化
 * @note   注册软定时器回调函数，启动整个任务调度
 * @param  无
 * @retval 无
 */
void main_loop_init(void)
{
    // 将软定时器函数注册到1ms中断中
    cpu_register_periodic_callback(cpu_periodic_callback);
}


/**
 * @brief  SNTP网络时间同步函数
 * @note   每天执行一次，从网络获取时间并写入RTC
 * @param  无
 * @retval 无
 */
static void time_sync(void)
{
    // 延时未结束，直接退出（未到同步时间）
    if (time_sync_delay > 0)
        return;
    
    // 重置延时：1天后再次执行同步
    time_sync_delay = TIME_SYNC_INTERVAL;
    
    esp_date_time_t esp_date = { 0 };
    // 通过ESP8266获取网络时间
    if (!esp_at_sntp_get_time(&esp_date))
    {
        printf("[SNTP] get time failed\n");
        time_sync_delay = SECONDS(1);  // 获取失败，1秒后重试
        return;
    }
    
    // 校验时间合法性：年份小于2000为无效时间
    if (esp_date.year < 2000)
    {
        printf("[SNTP] invalid date formate\n");
        time_sync_delay = SECONDS(1);  // 时间无效，1秒后重试
        return;
    }
    
    // 打印同步成功的时间信息
    printf("[SNTP] sync time: %04u-%02u-%02u %02u:%02u:%02u (%d)\n",
        esp_date.year, esp_date.month, esp_date.day,
        esp_date.hour, esp_date.minute, esp_date.second, esp_date.weekday);
    
    // 封装RTC时间结构体
    rtc_date_time_t rtc_date = { 0 };
    rtc_date.year = esp_date.year;
    rtc_date.month = esp_date.month;
    rtc_date.day = esp_date.day;
    rtc_date.hour = esp_date.hour;
    rtc_date.minute = esp_date.minute;
    rtc_date.second = esp_date.second;
    rtc_date.weekday = esp_date.weekday;
    
    // 将网络时间写入RTC硬件
    rtc_set_time(&rtc_date);
    
    // 时间同步完成，立即刷新屏幕时间
    time_update_delay = 0;
}


/**
 * @brief WiFi状态更新函数（定时获取WiFi连接状态并刷新屏幕）
 * @note 每5秒执行一次，仅在WiFi连接状态变化时刷新屏幕
 * @param 无
 * @return 无
 */
static void wifi_update(void)
{
    // 静态变量：保存上一次的WiFi信息，程序运行期间永久存储
    static esp_wifi_info_t last_info = { 0 };

    // 软定时器未到时间，直接退出
    if (wifi_update_delay > 0)
        return;
    
    // 重置延时计数器，5秒后再次执行
    wifi_update_delay = WIFI_UPDATE_INTERVAL;
    
    // 定义变量存储当前获取的WiFi信息
    esp_wifi_info_t info = { 0 };
    // 从ESP8266模块获取当前WiFi状态信息
    if (!esp_at_get_wifi_info(&info))
    {
        printf("[AT] wifi info get failed\n");
        return;
    }
    
    // 对比当前WiFi信息与上一次完全一致，不刷新屏幕
    if (memcmp(&info, &last_info, sizeof(esp_wifi_info_t)) == 0)
    {
        return;
    }
    
    // WiFi连接状态未发生变化，不刷新屏幕
    if (last_info.connected == info.connected)
    {
        return;
    }
    
    // WiFi已连接：打印信息并刷新屏幕显示WiFi名称
    if (info.connected)
    {
        printf("[WIFI] connected to %s\n", info.ssid);
        printf("[WIFI] SSID: %s, BSSID: %s, Channel: %d, RSSI: %d\n",
                info.ssid, info.bssid, info.channel, info.rssi);
        main_page_redraw_wifi_ssid(info.ssid);
    }
    // WiFi已断开：打印信息并刷新屏幕显示断开提示
    else
    {
        printf("[WIFI] disconnected from %s\n", last_info.ssid);
        main_page_redraw_wifi_ssid("wifi lost");
    }
    
    // 将当前WiFi信息保存到静态变量，用于下一次对比
    memcpy(&last_info, &info, sizeof(esp_wifi_info_t));
}

/**
 * @brief 系统时间更新函数（读取RTC时间并刷新屏幕）
 * @note 每1秒执行一次，仅在时间变化时刷新屏幕
 * @param 无
 * @return 无
 */
static void time_update(void)
{
    // 静态变量：保存上一次的时间数据
    static rtc_date_time_t last_date = { 0 };
    
    // 软定时器未到时间，直接退出
    if (time_update_delay > 0)
        return;
    
    // 重置延时计数器，1秒后再次执行
    time_update_delay = TIME_UPDATE_INTERVAL;
    
    // 从RTC硬件读取当前时间
    rtc_date_time_t date;
    rtc_get_time(&date);
    
    // 时间校验：年份小于2020为无效时间，直接退出
    if (date.year < 2020)
    {
        return;
    }
    
    // 时间未发生变化，不刷新屏幕
    if (memcmp(&date, &last_date, sizeof(rtc_date_time_t)) == 0)
    {
        return;
    }
    
    // 保存最新时间数据
    memcpy(&last_date, &date, sizeof(rtc_date_time_t));
    // 刷新屏幕显示时间和日期
    main_page_redraw_time(&date);
		//temp_test_main_page_redraw_time(&date);
    main_page_redraw_date(&date);
	  printf("[now_debug]second:%d \r\n", date.second); 

}

/**
 * @brief 室内温湿度更新函数（读取AHT20传感器数据并刷新屏幕）
 * @note 每3秒执行一次，仅在温湿度变化时刷新屏幕
 * @param 无
 * @return 无
 */
static void inner_update(void)
{
    // 静态变量：保存上一次的温湿度数据
    static float last_temperature, last_humidity;
    
    // 软定时器未到时间，直接退出
    if (inner_update_delay > 0)
        return;
    
    // 重置延时计数器，3秒后再次执行
    inner_update_delay = INNER_UPDATE_INTERVAL;
    
    // 启动AHT20传感器开始测量
    if (!aht20_start_measurement())
    {
        printf("[AHT20] start measurement failed\n");
        return;
    }
    
    // 等待传感器测量完成
    if (!aht20_wait_for_measurement())
    {
        printf("[AHT20] wait for measurement failed\n");
        return;
    }
    
    // 定义变量存储读取到的温湿度数据
    float temperature = 0.0f, humidity = 0.0f;
    
    // 从传感器读取温湿度数值
    if (!aht20_read_measurement(&temperature, &humidity))
    {
        printf("[AHT20] read measurement failed\n");
        return;
    }
    
    // 温湿度数据未变化，不刷新屏幕
    if (temperature == last_temperature && humidity == last_humidity)
    {
        return;
    }
    
    // 更新保存最新的温湿度数据
    last_temperature = temperature;
    last_humidity = humidity;
    
    // 打印温湿度信息并刷新屏幕显示
    printf("[AHT20] Temperature: %.1f, Humidity: %.1f\n", temperature, humidity);
    main_page_redraw_inner_temperature(temperature);
    main_page_redraw_inner_humidity(humidity);
}

/**
 * @brief 室外天气更新函数（网络获取天气数据并刷新屏幕）
 * @note 每1分钟执行一次，仅在天气数据变化时刷新屏幕
 * @param 无
 * @return 无
 */
static void outdoor_update(void)
{
    // 静态变量：保存上一次的室外天气数据
    static weather_info_t last_weather = { 0 };
    
    // 软定时器未到时间，直接退出
    if (outdoor_update_delay > 0)
        return;
    
    // 重置延时计数器，1分钟后再次执行
    outdoor_update_delay = OUTDOOR_UPDATE_INTERVAL;
    
    // 定义变量存储天气数据，心知天气API接口地址
    weather_info_t weather = { 0 };
    const char *weather_url = "https://api.seniverse.com/v3/weather/now.json?key=SuroKwGMkMEpgLeUg&location=Guangzhou&language=en&unit=c";
    // 通过ESP8266发送HTTP请求获取天气数据
    const char *weather_http_response = esp_at_http_get(weather_url);
    if (weather_http_response == NULL)
    {
        printf("[WEATHER] http error\n");
        return;
    }
    
    // 解析HTTP返回的JSON天气数据
    if (!parse_seniverse_response(weather_http_response, &weather))
    {
        printf("[WEATHER] parse failed\n");
        return;
    }
    
    // 天气数据未变化，不刷新屏幕
    if (memcmp(&last_weather, &weather, sizeof(weather_info_t)) == 0)
    {
        return;
    }
    
    // 保存最新天气数据并打印
    memcpy(&last_weather, &weather, sizeof(weather_info_t));
    printf("[WEATHER] %s, %s, %.1f\n", weather.city, weather.weather, weather.temperature);
    
    // 刷新屏幕显示室外温度和天气图标
    main_page_redraw_outdoor_temperature(weather.temperature);
    main_page_redraw_outdoor_weather_icon(weather.weather_code);
}


void main_loop(void)
{
    time_sync();
    wifi_update();
    time_update();
    inner_update();
    outdoor_update();
}