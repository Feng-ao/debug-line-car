#ifndef __KEY_H
#define __KEY_H

#include "main.h"

/* 四键按键板驱动（2026-10-17 接线）
 * 接线：KCOM -> GND，P/V- -> PA0，N/V+ -> PA1，PLAY -> PA2，MODE -> PA3
 * 引脚 PA0~PA3 内部上拉，按下读到低电平
 *
 * 用法：Key_Scan() 每 10ms 调一次，返回本次"刚按下"的键号（1~4），无按键返回 0
 */

void    Key_Init(void);
uint8_t Key_Scan(void);

#endif
