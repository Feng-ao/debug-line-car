#include "sensor.h"

/* 八路巡线传感器：GPIO 数字输入
 * 全部落在 GPIOB，一次读 IDR 就能拿到 8 路，无时序要求
 * 板上 X 脚是推挽输出，内部上拉只是防止拔线时悬空乱跳
 */

#define SENSOR_NUM   8

static const uint16_t sensor_pins[SENSOR_NUM] = {
    GPIO_PIN_12,    /* X1 */
    GPIO_PIN_13,    /* X2 */
    GPIO_PIN_14,    /* X3 */
    GPIO_PIN_15,    /* X4 */
    GPIO_PIN_8,     /* X5 */
    GPIO_PIN_9,     /* X6 */
    GPIO_PIN_6,     /* X7（原 PB10 让给 OLED） */
    GPIO_PIN_5      /* X8 */
};

void Sensor_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    uint8_t i;

    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* 八路一次配完，覆盖掉 CubeMX 里 PB6/PB7 的 I2C1 复用 */
    GPIO_InitStruct.Pin  = GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15
                         | GPIO_PIN_8  | GPIO_PIN_9  | GPIO_PIN_6  | GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    (void)i;
}

uint8_t Sensor_Read(void)
{
    uint32_t idr = GPIOB->IDR;
    uint8_t  mask = 0;
    uint8_t  i;

    for (i = 0; i < SENSOR_NUM; i++)
    {
        if (idr & sensor_pins[i])
        {
            mask |= (uint8_t)(1u << i);
        }
    }

    return mask;
}
