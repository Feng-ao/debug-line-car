#include "oled.h"
#include "oled_font.h"

/* SSD1306 128x64 驱动：软件模拟 I2C（SCL=PB10，SDA=PB11）
 * I2C 层逐字节复刻商家已验证例程（每字节一个完整 I2C 事务，2026-10-17 实测点亮）
 * 上层保留显存缓冲 + 分页刷新 + ShowString/ShowNum */

#define PAGE_NUM    8
#define PAGE_W      128
#define OLED_ADDR   0x78

static uint8_t oled_gram[PAGE_NUM][PAGE_W];

/* ---- GPIO 操作（与商家 oled.h 宏完全一致） ---- */
#define SCL_HIGH()  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_SET)
#define SCL_LOW()   HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_RESET)
#define SDA_HIGH()  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_11, GPIO_PIN_SET)
#define SDA_LOW()   HAL_GPIO_WritePin(GPIOB, GPIO_PIN_11, GPIO_PIN_RESET)

/* ---- I2C 时序（逐行复刻商家 oled.c，已实测点亮） ---- */
static void IIC_Start(void)
{
    SCL_HIGH();
    SDA_HIGH();
    SDA_LOW();
    SCL_LOW();
}

static void IIC_Stop(void)
{
    SCL_HIGH();
    SDA_LOW();
    SDA_HIGH();
}

static void IIC_Wait_Ack(void)
{
    SCL_HIGH();
    SCL_LOW();
}

static void IIC_WriteByte(uint8_t b)
{
    uint8_t i;
    SCL_LOW();
    for (i = 0; i < 8; i++)
    {
        if (b & 0x80) SDA_HIGH(); else SDA_LOW();
        b <<= 1;
        SCL_HIGH();
        SCL_LOW();
    }
}

static void OLED_WriteCmd(uint8_t cmd)
{
    IIC_Start();
    IIC_WriteByte(OLED_ADDR);   /* 从机地址 */
    IIC_Wait_Ack();
    IIC_WriteByte(0x00);        /* 命令 */
    IIC_Wait_Ack();
    IIC_WriteByte(cmd);
    IIC_Wait_Ack();
    IIC_Stop();
}

static void OLED_WriteDataByte(uint8_t dat)
{
    IIC_Start();
    IIC_WriteByte(OLED_ADDR);
    IIC_Wait_Ack();
    IIC_WriteByte(0x40);        /* 显示数据 */
    IIC_Wait_Ack();
    IIC_WriteByte(dat);
    IIC_Wait_Ack();
    IIC_Stop();
}

void OLED_Init(void)
{
    static const uint8_t init_seq[] = {
        0xAE,                   /* 关显示 */
        0x00,                   /* 列低地址 */
        0x10,                   /* 列高地址 */
        0x40,                   /* 起始行 0 */
        0x81, 0xCF,             /* 对比度 */
        0xA1,                   /* 段重映射 */
        0xC8,                   /* COM 扫描方向 */
        0xA6,                   /* 正常显示 */
        0xA8, 0x3F,             /* 复用率 64 行 */
        0xD3, 0x00,             /* 显示偏移 0 */
        0xD5, 0xF0,             /* 时钟分频/帧率（商家值） */
        0xD9, 0xF1,             /* 预充电 */
        0xDA, 0x12,             /* COM 引脚配置（64 行） */
        0xDB, 0x40,             /* VCOMH */
        0x20, 0x02,             /* 页寻址模式 */
        0x8D, 0x14,             /* 电荷泵开启 */
        0xA4,                   /* 显示跟随 RAM */
        0xA6,                   /* 正常显示 */
        0xAF                    /* 开显示 */
    };
    uint16_t i;
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* PB10/PB11 配为推挽输出（与商家一致） */
    __HAL_RCC_GPIOB_CLK_ENABLE();
    GPIO_InitStruct.Pin = GPIO_PIN_10 | GPIO_PIN_11;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    SCL_HIGH();
    SDA_HIGH();

    HAL_Delay(200);
    for (i = 0; i < sizeof(init_seq); i++)
    {
        OLED_WriteCmd(init_seq[i]);
    }
    OLED_Clear();
    OLED_Update();
}

void OLED_Fill(uint8_t dat)
{
    uint8_t i, j;
    for (i = 0; i < PAGE_NUM; i++)
        for (j = 0; j < PAGE_W; j++)
            oled_gram[i][j] = dat;
}

void OLED_Clear(void)
{
    OLED_Fill(0x00);
}

void OLED_Update(void)
{
    uint8_t page, col;
    for (page = 0; page < PAGE_NUM; page++)
    {
        OLED_WriteCmd(0xB0 | page);
        OLED_WriteCmd(0x00);
        OLED_WriteCmd(0x10);
        for (col = 0; col < PAGE_W; col++)
        {
            OLED_WriteDataByte(oled_gram[page][col]);
        }
    }
}

void OLED_ShowChar(uint8_t x, uint8_t page, char ch)
{
    uint8_t i;
    if (ch < 0x20 || ch > 0x7E) ch = ' ';
    if (x > PAGE_W - 6 || page > PAGE_NUM - 1) return;
    for (i = 0; i < 6; i++)
        oled_gram[page][x + i] = F6x8[ch - 0x20][i];
}

void OLED_ShowString(uint8_t x, uint8_t page, const char *str)
{
    while (*str)
    {
        OLED_ShowChar(x, page, *str++);
        x += 6;
    }
}

void OLED_ShowNum(uint8_t x, uint8_t page, int32_t num, uint8_t len)
{
    char buf[12];
    uint8_t i = 0, pos = 0;
    uint32_t n;
    char tmp[11];
    uint8_t t = 0;

    if (num < 0) { buf[pos++] = '-'; n = (uint32_t)(-num); } else { n = (uint32_t)num; }

    do { tmp[t++] = '0' + (n % 10); n /= 10; } while (n && t < 10);
    while (t < len && pos < 11) { buf[pos++] = '0'; len--; }
    while (t) buf[pos++] = tmp[--t];
    buf[pos] = '\0';

    for (i = 0; buf[i] && i < 12; i++)
    {
        OLED_ShowChar(x, page, buf[i]);
        x += 6;
    }
}
