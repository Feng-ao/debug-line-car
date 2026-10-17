#ifndef __INFO_H
#define __INFO_H

#include "main.h"

/* 赛题 3：人机交互与串口通信
 * a 部分已完成：OLED 显示姓名拼音、学号、变量 a（上电初始 0）
 * b / c 部分（上位机写入 a、GET A 回读）后续接入，共用同一个 g_var_a
 */

#define INFO_NAME   "FENG AO"      /* 姓名拼音 */
#define INFO_ID     "32602156"      /* 学号 */

extern volatile int32_t g_var_a;

void Info_Init(void);
void Info_Draw(void);       /* 整页重绘 */
void Info_DrawVar(void);    /* 只刷新变量 a 那一行 */

#endif
