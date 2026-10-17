#ifndef __MOTOR_H
#define __MOTOR_H

#include "main.h"

/* 双路直流电机（TB6612FNG）
 * 左轮：PWMA = PA6（TIM3_CH1）、AIN1 = PA5、AIN2 = PA8
 * 右轮：PWMB = PA7（TIM3_CH2）、BIN1 = PA11、BIN2 = PA12
 * STBY 固定接 3.3V；PWM 1kHz（PSC=71, ARR=999）
 *
 * 速度范围 ±1000：正数前进，负数后退，0 滑行
 * 若某个轮子转向反了，把下面的 INVERT 宏改成 1 即可，不用改接线
 */

#define MOTOR_LEFT_INVERT   0
#define MOTOR_RIGHT_INVERT  0

void Motor_Init(void);
void Motor_Set(int16_t left, int16_t right);
void Motor_Stop(void);

#endif
