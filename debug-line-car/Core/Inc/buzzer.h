#ifndef __BUZZER_H
#define __BUZZER_H

#include "main.h"

/* 有源蜂鸣器，PA4，高电平响
 * 若实测是无源蜂鸣器（只"嗒"一声），把 BUZZER_ACTIVE_LOW_DRIVE 打开，
 * 或者直接告诉今儿，改成 PWM 方波驱动
 */

void Buzzer_Init(void);
void Buzzer_On(void);
void Buzzer_Off(void);
void Buzzer_Beep(uint16_t ms);     /* 阻塞式的"响一声"，只在发车/停车时调用 */

#endif
