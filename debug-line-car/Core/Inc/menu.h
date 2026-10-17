#ifndef __MENU_H
#define __MENU_H

#include "main.h"

/* 赛题 1：OLED 多级菜单
 * 主菜单 4 项，键1/键2 移动箭头，键3 进入子菜单，键4 返回主菜单
 *
 * 用法：Menu_Init() 显示主菜单；主循环里反复调用 Menu_Task()（内部 10ms 调度）
 */

void Menu_Init(void);
void Menu_Task(void);

#endif
