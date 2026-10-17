#include "buzzer.h"

/* PA4 推挽输出，高电平响（有源蜂鸣器模块 I/O 脚高电平触发） */

void Buzzer_Init(void)
{
    Buzzer_Off();
}

void Buzzer_On(void)
{
    HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_SET);
}

void Buzzer_Off(void)
{
    HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_RESET);
}

void Buzzer_Beep(uint16_t ms)
{
    Buzzer_On();
    HAL_Delay(ms);
    Buzzer_Off();
}
