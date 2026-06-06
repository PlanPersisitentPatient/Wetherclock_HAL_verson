#include "headfile.h"
#include "st7789.h"

#define CS_PORT     GPIOE
#define CS_PIN      GPIO_PIN_2
#define RESET_PORT  GPIOE
#define RESET_PIN   GPIO_PIN_3
#define DC_PORT     GPIOE
#define DC_PIN      GPIO_PIN_4
#define BL_PORT     GPIOE
#define BL_PIN      GPIO_PIN_5

#define delay_us(x) cpu_delay(x)
#define delay_ms(x) cpu_delay((x) * 1000)



extern SPI_HandleTypeDef hspi2;


static void st7789_reset(void);
static void st7789_set_backlight(bool on);
static void st7789_write_register(uint8_t reg, uint8_t data[], uint16_t length);
void st7789_fill_color(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);




//屏幕的使用
/*
1、初始化完成屏幕的各种设置：st7789_init_display
2、在屏幕上显示一行的中英文：st7789_write_string
3、在屏幕上显示图片：st7789_draw_image

*/

/**
 * @brief  ST7789 LCD显示屏初始化
 * @note   执行复位、配置寄存器、清屏、开启背光
 */
void st7789_init_display(void)
{
    st7789_reset();
    
    st7789_write_register(0x11, NULL, 0);
    delay_ms(5);
    
    st7789_write_register(0x36, (uint8_t[]){0x00}, 1);
    st7789_write_register(0x3A, (uint8_t[]){0x55}, 1);
    st7789_write_register(0xB7, (uint8_t[]){0x46}, 1);
    st7789_write_register(0xBB, (uint8_t[]){0x1B}, 1);
    st7789_write_register(0xC0, (uint8_t[]){0x2C}, 1);
    st7789_write_register(0xC2, (uint8_t[]){0x01}, 1);
    st7789_write_register(0xC4, (uint8_t[]){0x20}, 1);
    st7789_write_register(0xC6, (uint8_t[]){0x0F}, 1);
    st7789_write_register(0xD0, (uint8_t[]){0xA4,0xA1}, 2);
    st7789_write_register(0xD6, (uint8_t[]){0xA1}, 1);
    st7789_write_register(0xE0, (uint8_t[]){0xF0,0x00,0x06,0x04,0x05,0x05,0x31,0x44,0x48,0x36,0x12,0x12,0x2B,0x34}, 14);
    st7789_write_register(0xE1, (uint8_t[]){0xF0,0x0B,0x0F,0x0F,0x0D,0x26,0x31,0x43,0x47,0x38,0x14,0x14,0x2C,0x32}, 14);
    st7789_write_register(0x21, NULL, 0);
    st7789_write_register(0x29, NULL, 0);
    
    // st7789_fill_color 函数保持不变直接复用
    st7789_fill_color(0, 0, 239, 319, 0x0000);
    st7789_set_backlight(true);
}



/**
 * @brief  向ST7789写入命令/配置数据
 * @param  reg: LCD命令码/寄存器地址
 * @param  data: 要写入的数据数组
 * @param  length: 数据长度（字节数）
 */
static void st7789_write_register(uint8_t reg, uint8_t data[], uint16_t length)
{
    // 片选拉低
    HAL_GPIO_WritePin(CS_PORT, CS_PIN, GPIO_PIN_RESET);
    
    // 发送指令
    HAL_GPIO_WritePin(DC_PORT, DC_PIN, GPIO_PIN_RESET); //0发送的类型是命令
    HAL_SPI_Transmit(&hspi2, &reg, 1, 1000);
    
    // 发送数据
    if(length > 0 && data != NULL)
    {
        HAL_GPIO_WritePin(DC_PORT, DC_PIN, GPIO_PIN_SET);
        HAL_SPI_Transmit(&hspi2, data, length, 1000);
    }
    
    // 片选拉高
    HAL_GPIO_WritePin(CS_PORT, CS_PIN, GPIO_PIN_SET);
}


/**
 * @brief  ST7789硬件复位
 */
static void st7789_reset(void)
{
    HAL_GPIO_WritePin(RESET_PORT, RESET_PIN, GPIO_PIN_RESET);
    delay_us(20);
    HAL_GPIO_WritePin(RESET_PORT, RESET_PIN, GPIO_PIN_SET);
    delay_ms(120);
}

/**
 * @brief  ST7789背光控制
 * @param  on: true-开启背光，false-关闭背光
 */
static void st7789_set_backlight(bool on)
{
    HAL_GPIO_WritePin(DC_PORT, BL_PIN, on ? GPIO_PIN_SET : GPIO_PIN_RESET);
}



/**
 * @brief  校验屏幕坐标是否合法
 * @param  x1: 起始X坐标  y1: 起始Y坐标
 * @param  x2: 结束X坐标  y2: 结束Y坐标
 * @return 坐标合法返回true，非法返回false
 */
static bool in_screen_range(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{

    if (x1 >= ST7789_WIDTH || y1 >= ST7789_HEIGHT)     // 判断x1/y1是否超出屏幕分辨率 → 越界返回假
        return false;

    if (x2 >= ST7789_WIDTH || y2 >= ST7789_HEIGHT)    // 判断x2/y2是否超出屏幕分辨率 → 越界返回假
        return false;

    if (x1 > x2 || y1 > y2)    // 判断起始坐标大于结束坐标 → 非法范围返回假
        return false;

    return true;    // 坐标合法，返回真
}


/**
 * @brief  设置LCD绘图的坐标范围
 * @param  x1: 起始X坐标  y1: 起始Y坐标
 * @param  x2: 结束X坐标  y2: 结束Y坐标
 */
static void st7789_set_range(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{

    st7789_write_register(0x2A, (uint8_t[]){(x1 >> 8) & 0xff, x1 & 0xff, (x2 >> 8) & 0xff, x2 & 0xff}, 4);    // 指令0x2A：设置X轴坐标范围，发送4字节数据(x1高8位、x1低8位、x2高8位、x2低8位)

    st7789_write_register(0x2B, (uint8_t[]){(y1 >> 8) & 0xff, y1 & 0xff, (y2 >> 8) & 0xff, y2 & 0xff}, 4);    // 指令0x2B：设置Y轴坐标范围，发送4字节数据(y1高8位、y1低8位、y2高8位、y2低8位)
}


/**
 * @brief  进入LCD像素数据写入模式（绘图准备）
 */
static void st7789_set_gram_mode(void)
{
    // 指令0x2C：告诉屏幕准备接收像素颜色数据
    st7789_write_register(0x2C, NULL, 0);
}


/**
 * @brief  向LCD指定区域填充纯色
 * @param  x1: 填充起始X坐标  y1: 填充起始Y坐标
 * @param  x2: 填充结束X坐标  y2: 填充结束Y坐标
 * @param  color: 填充的颜色值（RGB565格式）
 */
void st7789_fill_color(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
{
    //坐标校验
    if (!in_screen_range(x1, y1, x2, y2))
        return;
    //设置填充范围
    st7789_set_range(x1, y1, x2, y2);
    //进入绘图模式
    st7789_set_gram_mode();
    
    // 拆分颜色为2字节
    uint8_t color_data[2] = {(color >> 8) & 0xff, color & 0xff};
    // 计算总像素数
    uint32_t pixels = (x2 - x1 + 1) * (y2 - y1 + 1);

    //拉低CS
    HAL_GPIO_WritePin(CS_PORT, CS_PIN, GPIO_PIN_RESET);
    //拉高DC
    HAL_GPIO_WritePin(DC_PORT, DC_PIN, GPIO_PIN_SET);

    //循环发送像素
    for (uint32_t i = 0; i < pixels; i++)
    {
        // 【HAL替换】SPI_SendData+轮询 → HAL_SPI_Transmit（阻塞发送2字节，直至完成or超时）
        HAL_SPI_Transmit(&hspi2, color_data, 2, 1000);
    }

    // 【HAL替换】无需手动判断BSY，HAL函数已自动处理
		
    //拉高CS
    HAL_GPIO_WritePin(CS_PORT, CS_PIN, GPIO_PIN_SET);
}

// ===================== 新增：通用字模获取函数（支持自定义映射+标准模式）=====================
static const uint8_t *ascii_get_model(const char ch, const font_t *font)
{
    uint16_t bytes_per_row = (font->size / 2 + 7) / 8;
    uint16_t bytes_per_char = font->size * bytes_per_row;
    
    if (font->ascii_map)
    {
        const char *map = font->ascii_map;
        do
        {
            if (*map == ch)
            {
                return font->ascii_model + (map - font->ascii_map) * bytes_per_char;
            }
        } while (*(++map) != '\0');
    }
    else
    {
        return font->ascii_model + (ch - ' ') * bytes_per_char;
    }
    return NULL;
}



// ===================== 单个ASCII字符显示（HAL库版）=====================
// 功能：在屏幕(x,y)位置显示1个英文字符/数字/符号
static void st7789_write_single_ascii(uint16_t x, uint16_t y, char ch, uint16_t color, uint16_t bg_color, const font_t *font)
{
    // 1. 获取字符尺寸：高度=字库高度，宽度=高度的一半（ASCII标准配比）
    uint16_t fheight = font->size;
    uint16_t fwidth  = font->size / 2;

    // 2. 越界判断：字符超出屏幕则直接退出
    if (!in_screen_range(x, y, x + fwidth - 1, y + fheight - 1))
        return;
    
    // 3. 【核心关键】设置字符显示矩形区域 + 进入绘图模式
    // LCD会**自动记住区域**，后续按发送顺序填充像素，无需手动指定坐标
    st7789_set_range(x, y, x + fwidth - 1, y + fheight - 1);
		st7789_set_gram_mode();
    
    // 4. 拆分16位颜色 → 2个8位字节（屏幕协议要求，先发高8位）
    uint8_t color_data[2]     = {(color >> 8) & 0xff, color & 0xff};    // 文字颜色
    uint8_t bg_color_data[2]  = {(bg_color >> 8) & 0xff, bg_color & 0xff}; // 背景颜色

    // 5. 计算：字符每一行需要多少字节存储点阵（8个点占1字节）
    uint16_t bytes_per_row = (fwidth + 7) / 8;

//    // 6. 【字模寻址】找到当前字符在字库中的起始地址
//    // ch - ' '：字库从空格开始存储，计算字符偏移量
//    // 总偏移 = 字符序号 × 字符高度 × 每行字节数
//    const uint8_t *model = font->ascii_model + (ch - ' ') * fheight * bytes_per_row;
		
    // 6. 【改动点】调用通用函数获取字模（支持自定义映射+标准模式）
    const uint8_t *model = ascii_get_model(ch, font);

    // 7. HAL库GPIO操作：拉低CS选中屏幕，拉高DC表示发送像素数据
    HAL_GPIO_WritePin(CS_PORT, CS_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(DC_PORT, DC_PIN, GPIO_PIN_SET);

    // 8. 双层循环：逐行、逐列扫描字符点阵（从上到下，从左到右）
    // 循环顺序 = LCD像素填充顺序，硬件自动一一对应
    for (uint16_t row = 0; row < fheight; row++)
    {
        // 9. 【行寻址】找到当前行的点阵数据首地址
        // 行地址 = 字符首地址 + 行号 × 每行字节数
        const uint8_t *row_data = model + row * bytes_per_row;

        
        for (uint16_t col = 0; col < fwidth; col++)   // 逐列遍历当前行的所有像素点
        {
            // 10. 【像素判断】提取点阵的当前位：1=文字，0=背景
            // col/8：定位到字节；7-col%8：定位到字节内的位
            uint8_t pixel = row_data[col / 8] & (1 << (7 - col % 8));

            // 11. HAL库SPI发送：每个像素发送2字节RGB565颜色
            if (pixel)
            {
                
                HAL_SPI_Transmit(&hspi2, color_data, 2, 1000); // 点阵为1 → 发送文字颜色，LCD点亮当前像素
            }
            else
            {
                
                HAL_SPI_Transmit(&hspi2, bg_color_data, 2, 1000); // 点阵为0 → 发送背景颜色，LCD不点亮当前像素
            }
        }
    }

    // 12. 拉高CS，结束屏幕通信
    HAL_GPIO_WritePin(CS_PORT, CS_PIN, GPIO_PIN_SET);
}

//// ===================== 字符串显示（无硬件操作，无需改代码）=====================
//// 功能：循环显示一串字符，每显示一个字符，X坐标右移一个字符宽度
//void st7789_write_ascii(uint16_t x, uint16_t y, char *str, uint16_t color, uint16_t bg_color, const font_t *font)
//{
//    
//    while (*str) // 遍历字符串，直到结束符'\0'
//    {
//        
//        st7789_write_single_ascii(x, y, *str, color, bg_color, font); // 显示单个字符
//        
//        x += font->size / 2; // X坐标右移，准备显示下一个字符
//        
//        str++; // 指针指向下一个字符（指针 ++ 每次跳过它指向类型的大小）
//    }
//}


/**
 * @brief  在ST7789 LCD指定位置显示单个中文字符
 * @param  x: 显示起始X坐标
 * @param  y: 显示起始Y坐标
 * @param  ch: 要显示的中文字符串（单个汉字）
 * @param  color: 汉字字体颜色（RGB565格式）
 * @param  bg_color: 汉字背景颜色（RGB565格式）
 * @param  font: 字体结构体指针（包含字库、尺寸信息）
 */
static void st7789_write_chinese(uint16_t x, uint16_t y, const char *ch, uint16_t color, uint16_t bg_color, const font_t *font)
{
    // 空指针直接返回
    if (ch == NULL || font == NULL)
        return;

    uint16_t w = font->size;
    uint16_t h = font->size;

    // 超出屏幕范围不显示
    if (!in_screen_range(x, y, x + w - 1, y + h - 1))
        return;

    // 指向中文字库数组第一个元素
    const font_chinese_t *c = font->chinese;

    // 遍历字库，查找匹配的中文
    for (; c->name != NULL; c++)
    {
        // 对比中文编码，相同就找到
        if (strcmp(c->name, ch) == 0)
            break;
    }

    // 没找到就返回
    if (c->name == NULL)
        return;

    /*找到后，调用点阵绘制函数显示*/
    // 计算一行需要多少字节
    uint16_t bytes_per_row = (w + 7) / 8;

    // 颜色拆分高低字节（LCD 必须传 2 字节）
    uint8_t color_data[2] = {(color >> 8) & 0xFF, color & 0xFF};
    uint8_t bg_color_data[2] = {(bg_color >> 8) & 0xFF, bg_color & 0xFF};

    // 设置显示区域
    st7789_set_range(x, y, x + w - 1, y + h - 1);
		//进入屏幕绘图模式 
		st7789_set_gram_mode();
		

    // 拉低 CS，开始传输
    HAL_GPIO_WritePin(CS_PORT, CS_PIN, GPIO_PIN_RESET);
    // DC = 1，表示传数据
    HAL_GPIO_WritePin(DC_PORT, DC_PIN, GPIO_PIN_SET);

    // 逐行
    for (uint16_t row = 0; row < h; row++)
    {
        // 指向当前行的点阵数据
        const uint8_t *row_data = c->model + row * bytes_per_row;

        // 逐列
        for (uint16_t col = 0; col < w; col++)
        {
            // 取出当前位：1=显示前景色，0=显示背景色
            uint8_t pixel = row_data[col / 8] & (1 << (7 - col % 8));

            if (pixel)
            {
                // 发送颜色高 8 位
                HAL_SPI_Transmit(&hspi2, &color_data[0], 1, HAL_MAX_DELAY);
                // 发送颜色低 8 位
                HAL_SPI_Transmit(&hspi2, &color_data[1], 1, HAL_MAX_DELAY);
            }
            else
            {
                // 发送背景色
                HAL_SPI_Transmit(&hspi2, &bg_color_data[0], 1, HAL_MAX_DELAY);
                HAL_SPI_Transmit(&hspi2, &bg_color_data[1], 1, HAL_MAX_DELAY);
            }
        }
    }

    // 拉高 CS，结束传输
    HAL_GPIO_WritePin(CS_PORT, CS_PIN, GPIO_PIN_SET);
}

//-----------------------------------------------------------------------------------
// 函数：is_gb2312
// 功能：判断是不是 GB2312 中文的第一个字节
// 返回：1=中文，0=英文
//-----------------------------------------------------------------------------------
static bool is_gb2312(char ch)
{
    // GB2312 中文第一个字节范围：0xA1 ~ 0xF7
    return ((unsigned char)ch >= 0xA1 && (unsigned char)ch <= 0xF7);
}


/**
 * @brief  自动识别中英文，在LCD指定位置显示字符串
 * @param  x: 字符串起始显示X坐标
 * @param  y: 字符串起始显示Y坐标
 * @param  str: 要显示的字符串（可包含中英文）
 * @param  color: 字体颜色（RGB565格式）
 * @param  bg_color: 背景颜色（RGB565格式）
 * @param  font: 字体结构体指针（包含ASCII/中文字库、尺寸信息）
 */
void st7789_write_string(uint16_t x, uint16_t y, const char *str, uint16_t color, uint16_t bg_color, const font_t *font)
{
    // 遍历字符串，直到 \0 结束
    while (*str)
    {
        // 判断：中文占 2 字节，英文占 1 字节
        int len = is_gb2312(*str) ? 2 : 1;

        if (len <= 0)
        {
            str++;
            continue;
        }
        // 英文
        else if (len == 1)
        {
            st7789_write_single_ascii(x, y, *str, color, bg_color, font);
            str++;          // 英文占 1 字节，地址+1
            x += font->size / 2;
        }
        // 中文
        else
        {
            char ch_buf[5];       // 临时存放 2 字节中文
            strncpy(ch_buf, str, len);  // 从字符串里复制 2 字节中文

            // 显示中文
            st7789_write_chinese(x, y, ch_buf, color, bg_color, font);

            str += len;          // 中文占 2 字节，地址+2
            x += font->size;     // 中文宽度 = 字库大小
        }
    }
}

/**
 * @brief  在ST7789上显示RGB565格式的图片
 * @param  x: 起始X坐标  y: 起始Y坐标
 * @param  image: 图片结构体指针(包含宽、高、像素数据)
 */
void st7789_draw_image(uint16_t x, uint16_t y, const image_t *image)
{
    // 1. 屏幕越界判断，超出则直接返回
    if (x >= ST7789_WIDTH || y >= ST7789_HEIGHT || 
        x + image->width - 1 >= ST7789_WIDTH || y + image->height + 1 >= ST7789_HEIGHT)
        return;
    

    // 设置显示区域
    st7789_set_range(x, y, x + image->width - 1, y + image->height - 1);
		//进入屏幕绘图模式 
		st7789_set_gram_mode();
    
    // 3. HAL库GPIO操作：拉低CS(选中屏幕)，拉高DC(表示传输数据)
    HAL_GPIO_WritePin(CS_PORT, CS_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(DC_PORT, DC_PIN, GPIO_PIN_SET);

    // 4. 计算总数据长度：RGB565格式 1个像素=2字节
    uint32_t size = image->width * image->height * 2;
    const uint8_t *data = image->data;

    // 5. HAL库SPI轮询发送像素数据(高低字节反转，和原逻辑一致)
    for (uint32_t i = 0; i < size; i += 2)
    {
        // 发送高字节
        HAL_SPI_Transmit(&hspi2, (uint8_t *)&data[i + 1], 1, HAL_MAX_DELAY);
        // 发送低字节
        HAL_SPI_Transmit(&hspi2, (uint8_t *)&data[i], 1, HAL_MAX_DELAY);
    }

    // 6. 等待SPI总线空闲，拉高CS(取消选中屏幕)
    while (HAL_SPI_GetState(&hspi2) != HAL_SPI_STATE_READY);
    HAL_GPIO_WritePin(CS_PORT, CS_PIN, GPIO_PIN_SET);
}
