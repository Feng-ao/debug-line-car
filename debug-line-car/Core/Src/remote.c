/**
  ******************************************************************************
  * @file    remote.c
  * @brief   赛题 6.3：蓝牙遥控（前进 / 后退 / 左转 / 右转 / 停止）
  *
  * 单字符指令，收到就执行并回一行状态给上位机显示。
  * 超过 REMOTE_TIMEOUT 没有新指令自动停车（蓝牙掉线的保险）。
  ******************************************************************************
  */

#include "remote.h"
#include "motor.h"

#include <string.h>

extern UART_HandleTypeDef huart1;

#define REMOTE_SPEED     400   /* 前进/后退速度 */
#define REMOTE_TURN      350   /* 原地转时的内轮速度（反向） */
#define REMOTE_TIMEOUT   800   /* 多久没收到指令就停车（ms） */

static uint8_t  active;
static uint8_t  state;         /* 0=停 1=前进 2=后退 3=左转 4=右转 */
static uint32_t last_ms;

static const char *const state_str[5] = {
    "STOP", "FORWARD", "BACK", "LEFT", "RIGHT"
};

static void Send(const char *s)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)s, (uint16_t)strlen(s), 50);
}

void Remote_Enter(void)
{
    active  = 1;
    state   = 0;
    last_ms = HAL_GetTick();
    Motor_Stop();
}

void Remote_Exit(void)
{
    active = 0;
    state  = 0;
    Motor_Stop();
}

uint8_t Remote_IsActive(void)
{
    return active;
}

const char *Remote_GetStateStr(void)
{
    return state_str[state];
}

void Remote_Handle(char c)
{
    if (!active)
    {
        return;
    }

    if ((c >= 'a') && (c <= 'z'))
    {
        c = (char)(c - 'a' + 'A');
    }

    last_ms = HAL_GetTick();

    switch (c)
    {
        case 'F':
            state = 1;
            Motor_Set(REMOTE_SPEED, REMOTE_SPEED);
            break;

        case 'B':
            state = 2;
            Motor_Set(-REMOTE_SPEED, -REMOTE_SPEED);
            break;

        case 'L':
            state = 3;
            Motor_Set(-REMOTE_TURN, REMOTE_SPEED);
            break;

        case 'R':
            state = 4;
            Motor_Set(REMOTE_SPEED, -REMOTE_TURN);
            break;

        default:                        /* 'S' 或其它字符 = 停 */
            state = 0;
            Motor_Stop();
            break;
    }

    /* 回声给上位机，满足"显示当前运动状态"的评分要求 */
    Send("ST:");
    Send(state_str[state]);
    Send("\r\n");
}

void Remote_Task(void)
{
    if (!active || (state == 0))
    {
        return;
    }

    if ((HAL_GetTick() - last_ms) > REMOTE_TIMEOUT)
    {
        state = 0;
        Motor_Stop();                   /* 掉线保险：自动停车 */
    }
}
