#ifndef __OLED_H
#define __OLED_H

#include "main.h"

/* SSD1306 128x64 I2C OLED 驱动（软件模拟 I2C，PB10/PB11）
 * 接线：SCL->PB10  SDA->PB11  VCC->3.3V  GND->GND
 * 说明：软件 I2C 方案移植自商家已验证例程（2026-10-17 实测点亮）
 */

void OLED_Init(void);
void OLED_Clear(void);
void OLED_Fill(uint8_t dat);            /* 全屏填充（0xFF=全亮 0x00=全灭）*/
void OLED_Update(void);                 /* 把显存刷到屏幕 */
void OLED_UpdatePage(uint8_t page);     /* 只刷一页（0-7），用于局部刷新防闪烁 */
void OLED_ShowChar(uint8_t x, uint8_t page, char ch);   /* x:0-127  page:0-7 */
void OLED_ShowString(uint8_t x, uint8_t page, const char *str);
void OLED_ShowNum(uint8_t x, uint8_t page, int32_t num, uint8_t len);

#endif
