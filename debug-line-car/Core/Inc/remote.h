#ifndef __REMOTE_H
#define __REMOTE_H

#include "main.h"

/* 赛题 6.3：遥控小车
 *
 * 指令（单字符，大小写都认）：
 *   F 前进   B 后退   L 左转   R 右转   S 停止
 * 每条指令回一行状态，给上位机显示：
 *   ST:FORWARD / ST:BACK / ST:LEFT / ST:RIGHT / ST:STOP
 *
 * 安全：超过 800ms 没有新指令自动停车，防止蓝牙断开车还在跑
 */

void        Remote_Enter(void);         /* 进入遥控模式 */
void        Remote_Exit(void);          /* 退出，停车 */
uint8_t     Remote_IsActive(void);
void        Remote_Handle(char c);      /* 收到一个字符 */
void        Remote_Task(void);          /* 主循环里调用：超时保护 */
const char *Remote_GetStateStr(void);   /* FORWARD / BACK / ... */

#endif
