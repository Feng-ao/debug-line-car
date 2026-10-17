#ifndef __SENSOR_H
#define __SENSOR_H

#include "main.h"

/* 八路巡线传感器（YHAI00M YB-1MU04-2.0）—— GPIO 数字读取
 *
 * 板载 16P 排针丝印：5V GND KEY RESET X1 X2 X3 X4 X5 X6 X7 X8 TX RX SCL SDA
 * 接线（只用 X1~X8 + 电源）：
 *   X1 -> PB12   X2 -> PB13   X3 -> PB14   X4 -> PB15
 *   X5 -> PB8    X6 -> PB9    X7 -> PB6    X8 -> PB5
 *   （X7 原配 PB10，被 OLED 占用，改到 PB6）
 *
 * 注意：main.h 里的 X7_Pin 仍写着 PB10，是本模块以外的历史配置，别引用它。
 */

void    Sensor_Init(void);
uint8_t Sensor_Read(void);      /* bit0 = X1 ... bit7 = X8，1 = 高电平 */

#endif
