#include "headfile.h"

extern RTC_HandleTypeDef hrtc;


// ==================== 单次设置时间（内部函数，对应原 _rtc_set_time_once）====================
static void _rtc_set_time_once(const rtc_date_time_t *date_time)
{
    RTC_DateTypeDef sDate = {0};
    RTC_TimeTypeDef sTime = {0};

    // 配置日期
    sDate.Year    = date_time->year - 2000;
    sDate.Month   = date_time->month;
    sDate.Date    = date_time->day;
    sDate.WeekDay = date_time->weekday;

    // 配置时间
    sTime.Hours   = date_time->hour;
    sTime.Minutes = date_time->minute;
    sTime.Seconds = date_time->second;

    // HAL库设置日期+时间（二进制格式）

		HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
    HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN);


}

// ==================== 单次读取时间（内部函数，对应原 _rtc_get_time_once）====================
static void _rtc_get_time_once(rtc_date_time_t *date_time)
{
    RTC_DateTypeDef sDate = {0};
    RTC_TimeTypeDef sTime = {0};

    // HAL库读取日期+时间
    HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);


    // 转换为自定义结构体
    date_time->year    = 2000 + sDate.Year;
    date_time->month   = sDate.Month;
    date_time->day     = sDate.Date;
    date_time->weekday = sDate.WeekDay;
    date_time->hour    = sTime.Hours;
    date_time->minute  = sTime.Minutes;
    date_time->second  = sTime.Seconds;

}

// ==================== 安全设置时间（对应原 rtc_set_time）====================
void rtc_set_time(const rtc_date_time_t *date_time)
{
    rtc_date_time_t rtime;
    do {
        _rtc_set_time_once(date_time);
        _rtc_get_time_once(&rtime);
    } while (date_time->second != rtime.second);
}

// ==================== 安全读取时间（对应原 rtc_get_time）====================
void rtc_get_time(rtc_date_time_t *date_time)
{
    rtc_date_time_t time1, time2;
    do {
        _rtc_get_time_once(&time1);
        _rtc_get_time_once(&time2);
    } while (memcmp(&time1, &time2, sizeof(rtc_date_time_t)) != 0);

    memcpy(date_time, &time1, sizeof(rtc_date_time_t));
}