#include "headfile.h"

extern I2C_HandleTypeDef hi2c2;

/*
开机调一次，AHT20 温湿度传感器初始化：aht20_init()
读数据三步曲：启动 → 等待 → 读取；
向传感器发送指令，启动温湿度测量aht20_start_measurement；	等待传感器测量完成（阻塞等待）aht20_wait_for_measurement； 读取并转换温湿度真实值aht20_read_measurement
*/


// =====================I2C 写数据（对应原 aht20_write）=====================
static bool aht20_write(uint8_t data[], uint32_t length)
{
    // 原：产生START + 发地址 + 发数据 + STOP
    // 现：HAL 库一句完成
    if (HAL_I2C_Master_Transmit(&hi2c2, 0x70, data, length, 100) == HAL_OK)
        return true;
    else
        return false;
}


// ===================== I2C 读数据（对应原 aht20_read）=====================
static bool aht20_read(uint8_t data[], uint32_t length)
{
    if (HAL_I2C_Master_Receive(&hi2c2, 0x70, data, length, 100) == HAL_OK)
        return true;
    else
        return false;
}

// ===================== 读取状态字节 =====================
static bool aht20_read_status(uint8_t *status)
{
    // 发送指令 0x71 → 读取状态
    uint8_t cmd = 0x71;
    if (!aht20_write(&cmd, 1))
        return false;

    if (!aht20_read(status, 1))
        return false;

    return true;
}


// ===================== 判断传感器是否忙 =====================
static bool aht20_is_busy(void)
{
    uint8_t status;
    if (!aht20_read_status(&status))
        return false;

    // Bit7 = 1 表示正在测量
    return (status & 0x80) != 0;
}

// ===================== 判断传感器是否校准完成 =====================
static bool aht20_is_ready(void)
{
    uint8_t status;
    if (!aht20_read_status(&status))
        return false;

    // Bit3 = 1 表示就绪
    return (status & 0x08) != 0;
}



// ===================== 初始化函数 =====================
bool aht20_init(void)
{
    // 1. 等待传感器上电稳定（40ms）
    HAL_Delay(40);

    // 2. 如果已经就绪，直接返回成功
    if (aht20_is_ready())
        return true;

    // 3. 发送 AHT20 初始化指令：0xBE 08 00
    if (!aht20_write((uint8_t[]){0xBE, 0x08, 0x00}, 3))
        return false;

    // 4. 等待最多 100ms，直到传感器校准完成
    for (uint32_t t = 0; t < 100; t++)
    {
        HAL_Delay(1);
        if (aht20_is_ready())
            return true;
    }

    // 超时未就绪
    return false;
}


// ===================== 启动测量 =====================
bool aht20_start_measurement(void)
{
    // 发送测量指令 0xAC 33 00
    return aht20_write((uint8_t[]){0xAC, 0x33, 0x00}, 3);
}

// ===================== 等待测量完成 =====================
bool aht20_wait_for_measurement(void)
{
    for (uint32_t t = 0; t < 200; t++)
    {
        HAL_Delay(1);
        if (!aht20_is_busy())
            return true;
    }
    return false;
}

// ===================== 读取温湿度 =====================
bool aht20_read_measurement(float *temperature, float *humidity)
{
    uint8_t data[6];

    // 读取 6 字节原始数据
    if (!aht20_read(data, 6))
        return false;

    // 拼接湿度原始值（20位）
    uint32_t raw_humidity = ((uint32_t)data[1] << 12) |
                            ((uint32_t)data[2] << 4) |
                            ((uint32_t)(data[3] & 0xF0) >> 4);

    // 拼接温度原始值（20位）
    uint32_t raw_temp = ((uint32_t)(data[3] & 0x0F) << 16) |
                        ((uint32_t)data[4] << 8) |
                        ((uint32_t)data[5]);

    // 公式转换为实际值
    *humidity = (float)raw_humidity * 100.0f / 0x100000;
    *temperature = (float)raw_temp * 200.0f / 0x100000 - 50.0f;

    return true;
}
