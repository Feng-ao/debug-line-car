#include "gogogo.h"
#include "motor.h"
#include "buzzer.h"

/* 参数都在这里改 */
#define GOGOGO_BEEP_MS     3000u    /* 蜂鸣时长 */
#define GOGOGO_SPEED        500     /* 中等速度（0~1000，巡线基础速度是 420） */
#define GOGOGO_RUN_LIMIT  15000u    /* 直行安全上限，防止车跑丢 */

static GogogoState state = GOGOGO_IDLE;
static uint32_t    phase_ms;

void Gogogo_Init(void)
{
    state    = GOGOGO_IDLE;
    phase_ms = 0;
    Buzzer_Off();
}

void Gogogo_Start(void)
{
    if ((state == GOGOGO_BEEP) || (state == GOGOGO_RUN))
    {
        return;                 /* 已经在跑，别重入 */
    }

    Buzzer_On();                /* 开始长鸣 */
    phase_ms = HAL_GetTick();
    state    = GOGOGO_BEEP;
}

void Gogogo_Stop(void)
{
    Buzzer_Off();
    Motor_Stop();

    if ((state == GOGOGO_BEEP) || (state == GOGOGO_RUN))
    {
        state = GOGOGO_DONE;
    }
    else
    {
        state = GOGOGO_IDLE;
    }
}

void Gogogo_Task(void)
{
    uint32_t now = HAL_GetTick();

    switch (state)
    {
        case GOGOGO_BEEP:
            if ((now - phase_ms) >= GOGOGO_BEEP_MS)
            {
                Buzzer_Off();
                phase_ms = now;
                state    = GOGOGO_RUN;
                Motor_Set(GOGOGO_SPEED, GOGOGO_SPEED);
            }
            break;

        case GOGOGO_RUN:
            Motor_Set(GOGOGO_SPEED, GOGOGO_SPEED);
            if ((now - phase_ms) >= GOGOGO_RUN_LIMIT)
            {
                Gogogo_Stop();
            }
            break;

        default:
            break;
    }
}

GogogoState Gogogo_GetState(void)
{
    return state;
}

uint32_t Gogogo_GetMs(void)
{
    if ((state == GOGOGO_BEEP) || (state == GOGOGO_RUN))
    {
        return (uint32_t)(HAL_GetTick() - phase_ms);
    }
    return 0;
}
