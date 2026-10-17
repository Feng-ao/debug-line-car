#ifndef __LED_H
#define __LED_H

#include "main.h"

/* 赛题 2：LED 控制
 * 硬件：LED1 = PB0、LED2 = PB1，各串 220Ω 到 GND，高电平点亮
 *
 * 三种模式：常灭 / 常亮 / 交替闪烁（每 500ms 翻转一次，两个灯永远一明一灭）
 */

typedef enum
{
    LED_MODE_OFF = 0,   /* 两颗都灭 */
    LED_MODE_ON,        /* 两颗都亮 */
    LED_MODE_ALT        /* 交替闪烁 */
} LedMode;

void    LED_Init(void);
void    LED_SetMode(LedMode mode);
LedMode LED_GetMode(void);
uint8_t LED_Task(void);     /* 主循环里反复调用；状态有变化返回 1 */
uint8_t LED1_IsOn(void);
uint8_t LED2_IsOn(void);

#endif
