#include "headfile.h"


extern UART_HandleTypeDef huart1;


// 重定义printf底层输出函数 → 从串口1发送
int fputc(int ch, FILE *f)
{
    // 发送单个字符（HAL库函数）
    HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
    
    // 返回发送的字符（标准格式）
    return ch;
}