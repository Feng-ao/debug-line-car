#include "motor.h"

extern TIM_HandleTypeDef htim3;     /* main.c 里 CubeMX 生成的句柄 */

#define PWM_MAX   1000
#define PWM_MIN   (-1000)

static void Motor_ApplyDir(GPIO_TypeDef *port, uint16_t pin1, uint16_t pin2, int16_t spd)
{
    if (spd > 0)
    {
        HAL_GPIO_WritePin(port, pin1, GPIO_PIN_SET);
        HAL_GPIO_WritePin(port, pin2, GPIO_PIN_RESET);
    }
    else if (spd < 0)
    {
        HAL_GPIO_WritePin(port, pin1, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(port, pin2, GPIO_PIN_SET);
    }
    else
    {
        HAL_GPIO_WritePin(port, pin1, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(port, pin2, GPIO_PIN_RESET);
    }
}

void Motor_Init(void)
{
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);
    Motor_Stop();
}

void Motor_Set(int16_t left, int16_t right)
{
    int16_t pwm_l;
    int16_t pwm_r;

#if MOTOR_LEFT_INVERT
    left = (int16_t)(-left);
#endif
#if MOTOR_RIGHT_INVERT
    right = (int16_t)(-right);
#endif

    if (left  > PWM_MAX) left  = PWM_MAX;
    if (left  < PWM_MIN) left  = PWM_MIN;
    if (right > PWM_MAX) right = PWM_MAX;
    if (right < PWM_MIN) right = PWM_MIN;

    Motor_ApplyDir(AIN1_GPIO_Port, AIN1_Pin, AIN2_Pin, left);
    Motor_ApplyDir(BIN1_GPIO_Port, BIN1_Pin, BIN2_Pin, right);

    pwm_l = (left  < 0) ? (int16_t)(-left)  : left;
    pwm_r = (right < 0) ? (int16_t)(-right) : right;

    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, (uint32_t)pwm_l);
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, (uint32_t)pwm_r);
}

void Motor_Stop(void)
{
    Motor_Set(0, 0);
}
