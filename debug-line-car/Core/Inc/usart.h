#ifndef __USART_H
#define __USART_H

#include "main.h"

/* 赛题 3b / 3c：上位机通过串口读写变量 a
 *
 * 串口：USART1（PA9 = TX，PA10 = RX），115200-8-N-1
 * 外接 CH340 或 HC-05 都用这组引脚（二选一，不能同时插）
 *
 * 协议：
 *   发 "111"    → a = 111（不回复）
 *   发 "-45"    → a = -45
 *   发 "GET A"  → 回复 "111\r\n"
 */

void Usart_Init(void);   /* main 里 MX_USART1_UART_Init 之后调用 */
void Usart_Task(void);   /* 主循环里反复调用 */

#endif
