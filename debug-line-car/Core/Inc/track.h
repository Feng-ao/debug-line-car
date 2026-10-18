#ifndef __TRACK_H
#define __TRACK_H

#include "main.h"

/* 赛题 4b：巡线一圈
 * 流程：键3 发车 → 蜂鸣器响一声 → 巡线 → 到圈 → 蜂鸣器响一声 → 停车
 *
 * 计圈方式：左右轮速度差对时间积分（航向累积）
 * 因为车上没有编码器和陀螺仪，只能用"转向累积"近似判断是否绕回一周。
 * LAP_UNIT 需要在赛道上实测标定：先跑一圈按 K4 停下，读屏上的 L 值，
 * 把它填回 track.c 的 LAP_UNIT 即可。
 */

typedef enum
{
    TRACK_IDLE = 0,
    TRACK_RUN,
    TRACK_DONE
} TrackState;

void       Track_Init(void);
void       Track_Start(void);
void       Track_Stop(void);
void       Track_Task(void);        /* 每 10ms 调一次 */
TrackState Track_GetState(void);
uint32_t   Track_GetTimeMs(void);
int16_t    Track_GetError(void);
int32_t    Track_GetTurn(void);
uint8_t    Track_GetJuncIdx(void);   /* 已经过了几个路口，调试用 */
uint8_t    Track_GetLastMask(void);  /* 最后一次压线的位图，调试用 */
uint8_t    Track_GetEventMask(void); /* 触发最近一次拐角/路口时的位图，停车后看 */

/* 圈数：4b 跑一圈（1），4c 连跑两圈（2）；第二圈不停车，只在第一圈末尾响一声 */
void       Track_SetLapTarget(uint8_t n);
uint8_t    Track_GetLapTarget(void);
uint8_t    Track_GetLapDone(void);

#endif
