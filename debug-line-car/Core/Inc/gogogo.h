#ifndef __GOGOGO_H
#define __GOGOGO_H

#include "main.h"

/* EXTENSION -> GOGOGO
 * 流程：蜂鸣器长鸣 3 秒 -> 两轮同速前进（中等速度）
 * 全程非阻塞，随时按键 4 可以中断
 */

typedef enum
{
    GOGOGO_IDLE = 0,
    GOGOGO_BEEP,        /* 蜂鸣 3 秒 */
    GOGOGO_RUN,         /* 直行 */
    GOGOGO_DONE         /* 已停止 */
} GogogoState;

void         Gogogo_Init(void);
void         Gogogo_Start(void);
void         Gogogo_Stop(void);
void         Gogogo_Task(void);          /* 每 10ms 调一次 */
GogogoState  Gogogo_GetState(void);
uint32_t     Gogogo_GetMs(void);         /* 当前阶段已经过去的毫秒数 */

#endif
