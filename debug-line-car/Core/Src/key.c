#include "key.h"

/* 四键按键板：KCOM 接 GND，P/V-/N/V+/PLAY/MODE 分别接 PA0~PA3
 * 内部上拉 + 按下接地 => 读到 RESET 表示按下
 * 消抖方式：每个键独立计数，连续 KEY_DEBOUNCE 次采样与稳定状态不同才翻转
 */

#define KEY_NUM       4
#define KEY_DEBOUNCE  2u     /* 10ms x 2 = 20ms 消抖 */

static uint8_t key_stable;          /* 稳定状态位图，bit0~3 = 键1~键4，1=按下 */
static uint8_t key_last;            /* 上一轮的稳定状态，用来取下降沿 */
static uint8_t key_cnt[KEY_NUM];

/* 读一次原始电平，打包成位图：1 = 按下 */
static uint8_t Key_ReadRaw(void)
{
    uint8_t raw = 0;

    if (HAL_GPIO_ReadPin(KEY1_GPIO_Port, KEY1_Pin) == GPIO_PIN_RESET) raw |= 0x01;
    if (HAL_GPIO_ReadPin(KEY2_GPIO_Port, KEY2_Pin) == GPIO_PIN_RESET) raw |= 0x02;
    if (HAL_GPIO_ReadPin(KEY3_GPIO_Port, KEY3_Pin) == GPIO_PIN_RESET) raw |= 0x04;
    if (HAL_GPIO_ReadPin(KEY4_GPIO_Port, KEY4_Pin) == GPIO_PIN_RESET) raw |= 0x08;

    return raw;
}

void Key_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    uint8_t i;

    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitStruct.Pin  = KEY1_Pin | KEY2_Pin | KEY3_Pin | KEY4_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(KEY1_GPIO_Port, &GPIO_InitStruct);

    key_stable = 0;
    key_last   = 0;
    for (i = 0; i < KEY_NUM; i++)
    {
        key_cnt[i] = 0;
    }
}

uint8_t Key_IsDown(uint8_t n)
{
    if (n < 1u || n > KEY_NUM)
    {
        return 0;
    }
    return (uint8_t)((key_stable >> (n - 1u)) & 1u);
}

uint8_t Key_Scan(void)
{
    uint8_t raw     = Key_ReadRaw();
    uint8_t changed = (uint8_t)(raw ^ key_stable);
    uint8_t pressed;
    uint8_t i;

    /* 逐键消抖：状态连续变化够久才承认 */
    for (i = 0; i < KEY_NUM; i++)
    {
        if (changed & (1u << i))
        {
            if (++key_cnt[i] >= KEY_DEBOUNCE)
            {
                key_cnt[i]   = 0;
                key_stable  ^= (uint8_t)(1u << i);
            }
        }
        else
        {
            key_cnt[i] = 0;
        }
    }

    /* 只在"松开 -> 按下"的瞬间报一次，按住不放不会连发 */
    pressed  = (uint8_t)(key_stable & ~key_last);
    key_last = key_stable;

    for (i = 0; i < KEY_NUM; i++)
    {
        if (pressed & (1u << i))
        {
            return (uint8_t)(i + 1);
        }
    }

    return 0;
}
