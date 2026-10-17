#include "led.h"

/* LED1 = PB0，LED2 = PB1，高电平点亮（220Ω 限流到 GND）
 * 交替闪烁周期：500ms，即 1 秒一个完整循环，保证任何时刻只有一颗亮
 */

#define LED_BLINK_MS   500u

static LedMode  led_mode = LED_MODE_OFF;
static uint8_t  led1_on;
static uint8_t  led2_on;
static uint32_t led_tick;

/* 把当前状态写到引脚 */
static void LED_Apply(void)
{
    HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, led1_on ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, led2_on ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void LED_Init(void)
{
    led_mode = LED_MODE_OFF;
    led1_on  = 0;
    led2_on  = 0;
    LED_Apply();
}

void LED_SetMode(LedMode mode)
{
    led_mode = mode;

    switch (mode)
    {
        case LED_MODE_OFF:
            led1_on = 0;
            led2_on = 0;
            break;

        case LED_MODE_ON:
            led1_on = 1;
            led2_on = 1;
            break;

        case LED_MODE_ALT:
            led1_on  = 1;               /* 从 LED1 亮、LED2 灭开始 */
            led2_on  = 0;
            led_tick = HAL_GetTick();
            break;

        default:
            break;
    }

    LED_Apply();
}

LedMode LED_GetMode(void)
{
    return led_mode;
}

uint8_t LED1_IsOn(void)
{
    return led1_on;
}

uint8_t LED2_IsOn(void)
{
    return led2_on;
}

uint8_t LED_Task(void)
{
    uint32_t now;

    if (led_mode != LED_MODE_ALT)
    {
        return 0;
    }

    now = HAL_GetTick();
    if ((now - led_tick) < LED_BLINK_MS)
    {
        return 0;
    }
    led_tick = now;

    led1_on = (uint8_t)(!led1_on);
    led2_on = (uint8_t)(!led2_on);      /* 永远是反相，不会同时亮或同时灭 */

    /* 先点亮要亮的那颗、再熄灭要灭的那颗，切换瞬间也不会出现两灯同时灭 */
    if (led1_on)
    {
        HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_RESET);
    }
    else
    {
        HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);
    }

    return 1;
}
