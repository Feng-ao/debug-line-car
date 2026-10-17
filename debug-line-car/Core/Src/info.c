#include "info.h"
#include "oled.h"

/* 赛题 3a：信息显示（2 分）
 * 页面内容：
 *   ----- INFO -----
 *   NAME: FENG AO
 *   ID: 32602156
 *   A = 0
 *   K4: BACK
 *
 * 变量 a 上电由 C 全局变量默认初始化为 0；3b/3c 会从串口改写它
 */

#define VAR_PAGE      5     /* 变量 a 显示在第 5 页 */
#define VAR_NUM_X     64    /* 数字固定起点，位数变化时不左右跳 */

volatile int32_t g_var_a = 0;

void Info_Init(void)
{
    g_var_a = 0;
}

void Info_Draw(void)
{
    OLED_Clear();
    OLED_ShowStringCenter(0, "----- INFO -----");
    OLED_ShowStringCenter(2, "NAME: " INFO_NAME);
    OLED_ShowStringCenter(3, "ID: " INFO_ID);

    OLED_ShowString(34, VAR_PAGE, "A =");
    OLED_ShowNum(VAR_NUM_X, VAR_PAGE, (int32_t)g_var_a, 1);

    OLED_ShowStringCenter(7, "K4: BACK");
    OLED_Update();
}

/* 只刷新变量所在的那一页，用于串口写入后即时更新 */
void Info_DrawVar(void)
{
    OLED_ShowString(34, VAR_PAGE, "A =          ");   /* 先擦掉旧数值 */
    OLED_ShowNum(VAR_NUM_X, VAR_PAGE, (int32_t)g_var_a, 1);
    OLED_UpdatePage(VAR_PAGE);
}
