#include "headfile.h"

#define ESP_AT_DEBUG    1

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

typedef enum
{
    AT_ACK_NONE,
    AT_ACK_OK,
    AT_ACK_ERROR,
    AT_ACK_BUSY,
    AT_ACK_READY,
} at_ack_t;

typedef struct
{
    at_ack_t ack;
    const char *string;
} at_ack_match_t;

static const at_ack_match_t at_ack_matches[] = 
{
    {AT_ACK_OK, "OK\r\n"},
    {AT_ACK_ERROR, "ERROR\r\n"},
    {AT_ACK_BUSY, "busy p¡­\r\n"},
    {AT_ACK_READY, "ready\r\n"},
};


static char rxbuf[1024];


extern UART_HandleTypeDef huart2;



//一、

/**
 * @brief 串口发送AT指令字符串，自动添加AT指令必需的\r\n结尾
 * @param data 要发送的AT指令字符串指针
 */
static void esp_at_usart_write(const char *data)
{
    // 1. 发送主体字符串
    if(data && *data)
    {
        HAL_UART_Transmit(&huart2, (uint8_t*)data, strlen(data), HAL_MAX_DELAY);
    }
    // 2. 发送AT指令必须的结尾 \r\n
    uint8_t end[2] = {'\r', '\n'};
    HAL_UART_Transmit(&huart2, end, 2, HAL_MAX_DELAY);
}

static at_ack_t match_internal_ack(const char *str)
{
    for (uint32_t i = 0; i < ARRAY_SIZE(at_ack_matches); i++)
    {
        if (strcmp(str, at_ack_matches[i].string) == 0)
            return at_ack_matches[i].ack;
    }
    
    return AT_ACK_NONE;
}


/**
 * @brief 轮询接收串口数据，按行匹配ESP32的AT指令响应
 * @param timeout 接收超时时间，单位：毫秒
 * @return 匹配到的响应枚举值，超时/无匹配返回AT_ACK_NONE
 */
static at_ack_t esp_at_usart_wait_receive(uint32_t timeout)
{
    uint32_t rxlen = 0;
    const char *line = rxbuf;
    uint32_t start = HAL_GetTick(); // 对应原cpu_get_ms()
    
    rxbuf[0] = '\0';
    while (rxlen < sizeof(rxbuf) - 1)
    {
        uint8_t ch;
        // 轮询接收1字节，带超时（对应原USART_FLAG_RXNE判断）
        if(HAL_UART_Receive(&huart2, &ch, 1, 10) == HAL_OK)
        {
            rxbuf[rxlen++] = ch;
            rxbuf[rxlen] = '\0';
            
            // 收到换行，匹配响应（逻辑完全不变）
            if (rxbuf[rxlen - 1] == '\n')
            {
                at_ack_t ack = match_internal_ack(line);
                if (ack != AT_ACK_NONE)
                    return ack;
                line = rxbuf + rxlen;
            }
        }
        
        // 超时判断（逻辑完全不变）
        if (HAL_GetTick() - start >= timeout)
            return AT_ACK_NONE;
    }
    return AT_ACK_NONE;
}


/**
 * @brief 等待ESP32模块重启/上电完成，检测ready就绪信号
 * @param timeout 等待超时时间，单位：毫秒
 * @return 成功检测到ready信号返回true，超时返回false
 */
bool esp_at_wait_ready(uint32_t timeout)
{
    return esp_at_usart_wait_receive(timeout) == AT_ACK_READY;
}

/**
 * @brief 循环发送AT指令，检测ESP32模块是否正常在线
 * @param timeout 总检测超时时间，单位：毫秒
 * @return 模块响应AT指令返回true，超时未响应返回false
 */
static bool esp_at_wait_boot(uint32_t timeout)
{
    for (int t = 0; t < timeout; t += 100)
    {
        if (esp_at_write_command("AT", 100))
            return true;
    }
    return false;
}


/**
 * @brief ESP32 AT指令功能初始化总函数
 * 步骤：检测模块在线 → 恢复出厂设置 → 等待模块重启就绪
 * @return 初始化全部成功返回true，任意一步失败返回false
 */
bool esp_at_init(void)
{
    if (!esp_at_wait_boot(3000))
        return false;
    if (!esp_at_write_command("AT+RESTORE", 2000))
        return false;
    if (!esp_at_wait_ready(5000))
        return false;
    return true;
}


/**
 * @brief   ESP32 模块收到 STM32 发的 AT 指令并执行后，原路返回给 STM32 的所有响应数据

 */
const char *esp_at_get_response(void)
{
    return rxbuf;
}




/**
 * @brief 发送AT指令并等待响应，判断指令是否执行成功
 * @param command 要发送的AT指令字符串
 * @param timeout 等待响应的超时时间，单位：毫秒
 * @return 指令执行成功返回true，超时/失败返回false
 */
bool esp_at_write_command(const char *command, uint32_t timeout)
{
#if ESP_AT_DEBUG
    printf("[DEBUG] Send: %s\n", command);
#endif

    esp_at_usart_write(command);
    at_ack_t ack = esp_at_usart_wait_receive(timeout);

#if ESP_AT_DEBUG
    printf("[DEBUG] Response:\n%s\n", rxbuf);
#endif

    return ack == AT_ACK_OK;
}


//二、发送at命令连接wifi等相关函数
/*
1、WiFi连接步骤：(1)esp_at_wifi_init (2)esp_at_connect_wifi
2、判断wifi是否连接成功：wifi_is_connected
3、去访问api/网址获取信息，并将获取的信息返回：esp_at_http_get
4、解析返回到的信息：在weather.c里写了一个解析心知天气api返回信息的函数
*/

/**
 * @brief  ESP32 WiFi初始化，设置为STA模式(客户端模式)
 * @return 初始化成功返回true，失败返回false
 */
bool esp_at_wifi_init(void)
{
    return esp_at_write_command("AT+CWMODE=1", 2000);
}

/**
 * @brief  ESP32连接指定WiFi路由器
 * @param  ssid: WiFi名称
 * @param  pwd:  WiFi密码
 * @param  mac:  可选参数，路由器MAC地址(传NULL则不使用)
 * @return 连接成功返回true，失败返回false
 */
bool esp_at_connect_wifi(const char *ssid, const char *pwd, const char *mac)
{
    if (ssid == NULL || pwd == NULL)
        return false;
    
    char cmd[128];
    int len = snprintf(cmd, sizeof(cmd), "AT+CWJAP=\"%s\",\"%s\"", ssid, pwd);
    if (mac)
        snprintf(cmd + len, sizeof(cmd) - len, ",\"%s\"", mac);
    
    return esp_at_write_command(cmd, 5000);
}


/**
 * @brief  ESP32发送HTTP GET请求
 * @param  url: 要访问的网址/API接口
 * @return 请求成功返回服务器响应数据，失败返回NULL
 */
const char *esp_at_http_get(const char *url)
{
    char *txbuf = rxbuf;
    snprintf(txbuf, sizeof(rxbuf), "AT+HTTPCLIENT=2,1,\"%s\",,,2", url);
    bool ret = esp_at_write_command(txbuf, 5000);
    return ret ? esp_at_get_response() : NULL;
}



/**
 * @brief  解析WiFi状态查询的响应数据
 * @param  response: ESP32返回的原始响应字符串
 * @param  info: 存储解析后的WiFi连接状态和名称
 * @return 解析成功返回true，失败返回false
 */
static bool parse_cwstate_response(const char *response, esp_wifi_info_t *info)
{
//    AT+CWSTATE?
//    +CWSTATE:2,"Xiaomi Mi MIX 3_5577"
//    OK
	// 查找状态关键字，定位有效数据
	response = strstr(response, "+CWSTATE:");
	// 未找到关键字，解析失败
	if (response == NULL)
		return false;
	
	int wifi_state;
	// 格式化解析状态值和WiFi名称
	if (sscanf(response, "+CWSTATE:%d,\"%63[^\"]", &wifi_state, info->ssid) != 2)
		return false;
	
	// 状态2表示WiFi已连接
	info->connected = (wifi_state == 2);
	
	return true;
}

/**
 * @brief  解析已连接WiFi的详细信息响应
 * @param  response: ESP32返回的原始响应字符串
 * @param  info: 存储解析后的WiFi名称、MAC、信道、信号强度
 * @return 解析成功返回true，失败返回false
 */
static bool parse_cwjap_response(const char *response, esp_wifi_info_t *info)
{
//    AT+CWJAP?
//    +CWJAP:"Xiaomi Mi MIX 3_5577","da:b5:3a:e3:2f:60",9,-48,0,1,3,0,1
//    OK
	// 查找详情关键字，定位有效数据
	response = strstr(response, "+CWJAP:");
	// 未找到关键字，解析失败
	if (response == NULL)
		return false;
	
	// 格式化解析WiFi名称、MAC、信道、信号强度
	if (sscanf(response, "+CWJAP:\"%63[^\"]\",\"%17[^\"]\",%d,%d", info->ssid, info->bssid, &info->channel, &info->rssi) != 4)
		return false;
	
	return true;
}

/**
 * @brief  获取WiFi完整信息（状态+连接详情）
 * @param  info: 用于存储所有WiFi信息的结构体指针
 * @return 获取成功返回true，任意步骤失败返回false
 */
bool esp_at_get_wifi_info(esp_wifi_info_t *info)
{
    // 发送指令查询WiFi连接状态
    if (!esp_at_write_command("AT+CWSTATE?", 2000))
        return false;
    // 解析连接状态
    if (!parse_cwstate_response(esp_at_get_response(), info))
        return false;
    
    // 已连接WiFi时，获取详细参数，没成功连接WiFi也会直接返回true
    if (info->connected == true)
    {
        // 发送指令查询WiFi详情
        if (!esp_at_write_command("AT+CWJAP?", 2000))
            return false;
        // 解析WiFi详情
        if (!parse_cwjap_response(esp_at_get_response(), info))
            return false;
    }
    
    return true;
}

/**
 * @brief  判断当前是否成功连接WiFi
 * @return 已连接返回true，未连接/获取失败返回false
 */
bool wifi_is_connected(void)
{
    esp_wifi_info_t info;
    // 获取WiFi信息成功，返回连接状态
    if (esp_at_get_wifi_info(&info))
    {
        return info.connected;
    }
    // 获取信息失败，默认未连接
    return false;
}


//获取SNTP服务得到当前的准确时间
/*
1、一样还是得先连上wifi
2、设置要获取时间的具体地区：esp_at_sntp_init
3、获取具体的时间并做解析：esp_at_sntp_get_time
*/


/**
 * @brief  初始化SNTP网络时间服务
 * @return 初始化成功返回true，失败返回false
 */
bool esp_at_sntp_init(void)
{
	  // 发送AT指令：开启SNTP，设置时区为东8区(北京时间)，超时2秒
    // 指令发送失败则返回false
    if (!esp_at_write_command("AT+CIPSNTPCFG=1,8", 2000))
        return false;

    return true;
}

/**
 * @brief  英文月份缩写转换为数字月份
 * @param  month_str: 英文月份缩写字符串(如Jan)
 * @return 对应数字月份(1-12)，转换失败返回0
 */
static uint8_t month_str_to_num(const char *month_str)
{
	const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
		"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
	for (uint8_t i = 0; i < 12; i++)
	{
		if (strcmp(month_str, months[i]) == 0)
		{
			return i + 1;
		}
	}
	return 0;
}

/**
 * @brief  英文星期缩写转换为数字星期
 * @param  weekday_str: 英文星期缩写字符串(如Mon)
 * @return 对应数字星期(1-7)，转换失败返回0
 */
static uint8_t weekday_str_to_num(const char *weekday_str)
{
	const char *weekdays[] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
	for (uint8_t i = 0; i < 7; i++) {
		if (strcmp(weekday_str, weekdays[i]) == 0)
		{
			return i + 1;
		}
	}
	return 0;
}

/**
 * @brief  解析SNTP时间查询的响应数据
 * @param  response: ESP32返回的原始时间字符串
 * @param  date: 存储解析后时间数据的结构体指针
 * @return 解析成功返回true，失败返回false
 */
static bool parse_cipsntptime_response(const char *response, esp_date_time_t *date)
{
	char weekday_str[8];
	char month_str[4];
	response = strstr(response, "+CIPSNTPTIME:");

	if (sscanf(response, "+CIPSNTPTIME:%3s %3s %hhu %hhu:%hhu:%hhu %hu",
			   weekday_str, month_str,
			   &date->day, &date->hour, &date->minute, &date->second, &date->year) != 7)
		return false;

	date->weekday = weekday_str_to_num(weekday_str);
	date->month = month_str_to_num(month_str);

	return true;
}








/**
 * @brief  获取SNTP同步的网络时间
 * @param  date: 存储获取到的时间数据的结构体指针
 * @return 获取并解析成功返回true，失败返回false
 */
bool esp_at_sntp_get_time(esp_date_time_t *date)
{
	

	  // 发送AT指令：查询当前SNTP同步的时间，超时2秒
    // 指令失败返回false
    if (!esp_at_write_command("AT+CIPSNTPTIME?", 2000))
        return false;

    if (!parse_cipsntptime_response(esp_at_get_response(), date))
        return false;

    return true;
}

