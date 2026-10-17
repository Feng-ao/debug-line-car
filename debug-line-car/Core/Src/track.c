#include "track.h"
#include "line.h"
#include "motor.h"
#include "buzzer.h"

/* 赛题 4b：巡线一圈
 * 控制律：偏差 PD -> 差速转向
 * 计圈：左右轮速差对时间积分，绝对值超过 LAP_UNIT 判定跑满一圈
 */

#define BASE_SPEED        420    /* 基础速度，0~1000 */
#define KP_NUM             70    /* 比例：偏差 * 0.70 */
#define KD_NUM            120    /* 微分：偏差变化 * 1.20 */
#define TURN_LIMIT        620    /* 转向量上限，别把一侧车轮压到反转 */
#define LAP_UNIT       300000    /* 一圈的转向积分阈值（待赛道标定） */
#define TRACK_INVERT_TURN   0    /* 发现越跑越偏就改成 1 */
#define LOST_STOP_TICKS   200    /* 连续 2 秒看不到线就停车 */

static TrackState state;
static uint32_t   start_ms;
static uint32_t   elapsed_ms;
static int16_t    last_err;
static int32_t    turn_acc;
static uint16_t   lost_cnt;
static uint8_t    lap_target = 1;
static uint8_t    lap_done   = 0;

void Track_Init(void)
{
    state      = TRACK_IDLE;
    elapsed_ms = 0;
    last_err   = 0;
    turn_acc   = 0;
    lost_cnt   = 0;
    Motor_Stop();
}

void Track_Start(void)
{
    if (state == TRACK_RUN)
    {
        return;
    }

    Buzzer_Beep(150);           /* 发车提示音 */

    last_err   = 0;
    turn_acc   = 0;
    lost_cnt   = 0;
    lap_done   = 0;
    elapsed_ms = 0;
    start_ms   = HAL_GetTick();
    state      = TRACK_RUN;
}

void Track_Stop(void)
{
    Motor_Stop();

    if (state == TRACK_RUN)
    {
        elapsed_ms = HAL_GetTick() - start_ms;
        state      = TRACK_DONE;
        Buzzer_Beep(150);       /* 到点提示音 */
    }
}

void Track_Task(void)
{
    int16_t err;
    int16_t delta;
    int32_t turn;
    int16_t left;
    int16_t right;

    if (state != TRACK_RUN)
    {
        return;
    }

    elapsed_ms = HAL_GetTick() - start_ms;

    err   = Line_Error();
    delta = (int16_t)(err - last_err);
    last_err = err;

    turn = ((int32_t)err * KP_NUM + (int32_t)delta * KD_NUM) / 100;

#if TRACK_INVERT_TURN
    turn = -turn;
#endif

    if (turn >  TURN_LIMIT) turn =  TURN_LIMIT;
    if (turn < -TURN_LIMIT) turn = -TURN_LIMIT;

    left  = (int16_t)(BASE_SPEED + turn);
    right = (int16_t)(BASE_SPEED - turn);

    Motor_Set(left, right);

    /* 丢线保护：一直看不到线说明冲出赛道了 */
    if (Line_OnCount() == 0)
    {
        lost_cnt++;
        if (lost_cnt >= LOST_STOP_TICKS)
        {
            Track_Stop();
            return;
        }
    }
    else
    {
        lost_cnt = 0;
    }

    /* 转向积分：直道左右轮速差为 0，弯道才累积，绕回一周约等于 360 度 */
    turn_acc += (int32_t)(right - left);

    /* 每满一圈判定一次：最后一圈停车，前面几圈只响一声继续跑（赛题 4c） */
    {
        int32_t next_lap = (int32_t)LAP_UNIT * (int32_t)(lap_done + 1u);

        if ((turn_acc >= next_lap) || (turn_acc <= -next_lap))
        {
            lap_done++;
            if (lap_done >= lap_target)
            {
                Track_Stop();
            }
            else
            {
                Buzzer_Beep(150);
            }
        }
    }
}

void Track_SetLapTarget(uint8_t n)
{
    if (n < 1u) n = 1u;
    if (n > 5u) n = 5u;
    lap_target = n;
}

uint8_t Track_GetLapTarget(void)
{
    return lap_target;
}

uint8_t Track_GetLapDone(void)
{
    return lap_done;
}

TrackState Track_GetState(void)
{
    return state;
}

uint32_t Track_GetTimeMs(void)
{
    if (state == TRACK_RUN)
    {
        return (uint32_t)(HAL_GetTick() - start_ms);
    }
    return elapsed_ms;
}

int16_t Track_GetError(void)
{
    return last_err;
}

int32_t Track_GetTurn(void)
{
    return turn_acc;
}
