#include "headfile.h"



/**
 * @brief 解析心知天气API返回的JSON数据，提取天气信息
 * @param response  天气服务器返回的原始JSON字符串
 * @param info      自定义天气结构体，存储解析后的天气数据
 * @return 解析成功返回true，关键数据缺失返回false
 */
bool parse_seniverse_response(const char *response, weather_info_t *info)
{
	// 1. 在JSON数据中查找核心关键字 "\"results\":" (转义后为 "results":)
	// results是心知天气JSON的根数据节点，找不到说明数据格式错误
	response = strstr(response, "\"results\":");
	if (response == NULL)
		return false;
	
	// 2. 查找位置信息节点 "\"location\":"
	// 用于提取城市、地区等位置数据
	const char *location_response = strstr(response, "\"location\":");
	if (location_response == NULL)
		return false;
	
	// 3. 查找城市名称 "\"name\":"
	// 找到则解析双引号内的内容，存入结构体的 city 字段（最多31个字符）
	const char *loaction_name_response = strstr(location_response, "\"name\":");
	if (loaction_name_response)
	{
		sscanf(loaction_name_response, "\"name\": \"%31[^\"]\"", info->city);
	}
	
	// 4. 查找完整位置路径 "\"path\":"
	// 解析后存入结构体的 loaction 字段（最多128个字符）
	const char *loaction_path_response = strstr(location_response, "\"path\":");
	if (loaction_path_response)
	{
		sscanf(loaction_path_response, "\"path\": \"%128[^\"]\"", info->loaction);
	}
	
	// 5. 查找实时天气节点 "\"now\":"
	// 用于提取天气状态、温度等实时数据
	const char *now_response = strstr(response, "\"now\":");
	if (now_response == NULL)
		return false;
	
	// 6. 查找天气描述 "\"text\":" (如：晴、多云、雨)
	// 解析后存入 weather 字段
	const char *now_text_response = strstr(now_response, "\"text\":");
	if (now_text_response)
	{
		sscanf(now_text_response, "\"text\": \"%15[^\"]\"", info->weather);
	}
	
	// 7. 查找天气代码 "\"code\":" (数字，对应天气图标)
	// 解析为整数，存入 weather_code 字段
	const char *now_code_response = strstr(now_response, "\"code\":");
	if (now_code_response)
	{
		sscanf(now_code_response, "\"code\": \"%d\"", &info->weather_code);
	}
	
	// 8. 温度解析（特殊处理：JSON中温度是字符串，需转浮点数）
	char temperature_str[16] = { 0 };
	// 查找温度字段 "\"temperature\":"
	const char *now_temperature_response = strstr(now_response, "\"temperature\":");
	if (now_temperature_response)
	{
		// 先把字符串格式的温度读入字符数组
		if (sscanf(now_temperature_response, "\"temperature\": \"%15[^\"]\"", temperature_str) == 1)
			// 用 atof 把字符串转为浮点数，存入 temperature 字段
			info->temperature = atof(temperature_str);
	}
	
	// 所有关键数据解析完成，返回成功
	return true;
}