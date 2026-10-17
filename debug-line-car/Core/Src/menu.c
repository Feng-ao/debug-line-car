#include "menu.h"
#include "oled.h"
#include "key.h"

/* 赛题 1：OLED 多级菜单（主菜单 3 分 + 子菜单 3 分）
 *
 * 主菜单：LED 控制 / 信息显示 / 巡线功能 / 拓展功能
 *   键1 = 箭头上移    键2 = 箭头下移
 *   键3 = 进入子菜单  键4 = 返回主菜单
 *
 * 显示资源：128x64，F6x8 字库 => 每行 21 字符、共 8 行（page 0~7）
 */

#define MAIN_ITEM_NUM   4
#define MENU_TICK_MS    10u   /* 按键扫描周期 */

/* 主菜单 4 项（英文短标签，字库只有 ASCII） */
static const char *const main_items[MAIN_ITEM_NUM] = {
    "LED CONTROL",
    "INFO SHOW",
    "LINE TRACK",
    "EXTENSION"
};

/* 页面状态：0 = 主菜单，1~4 = 对应子菜单 */
typedef enum
{
    PAGE_MAIN = 0,
    PAGE_LED  = 1,
    PAGE_INFO = 2,
    PAGE_TRACK= 3,
    PAGE_EXT  = 4
} MenuPage;

static MenuPage menu_page = PAGE_MAIN;
static uint8_t  menu_cursor;      /* 当前箭头指向的行，0~3 */
static uint8_t  menu_dirty;       /* 1 = 画面需要重绘 */

/* ---------------- 小工具 ---------------- */

static uint8_t StrLen(const char *s)
{
    uint8_t n = 0;
    while (s[n] != '\0') n++;
    return n;
}

/* 居中显示（按 6px 字宽估算） */
static void ShowCenter(uint8_t page, const char *s)
{
    uint8_t w = (uint8_t)(StrLen(s) * 6u);
    uint8_t x = 0;

    if (w < 128u)
    {
        x = (uint8_t)((128u - w) / 2u);
    }
    OLED_ShowString(x, page, s);
}

/* ---------------- 画面 ---------------- */

static void Draw_Main(void)
{
    uint8_t i;

    OLED_Clear();
    ShowCenter(0, "DEBUG LINE CAR");
    ShowCenter(1, "----- MAIN -----");

    for (i = 0; i < MAIN_ITEM_NUM; i++)
    {
        OLED_ShowChar(18, (uint8_t)(3 + i), (i == menu_cursor) ? '>' : ' ');
        OLED_ShowString(30, (uint8_t)(3 + i), main_items[i]);
    }

    ShowCenter(7, "1/2:MOVE 3:OK");
    OLED_Update();
}

static void Draw_Sub(void)
{
    OLED_Clear();
    ShowCenter(0, "----- SUB -----");
    ShowCenter(3, main_items[menu_page - 1]);
    ShowCenter(5, "K4: BACK");
    OLED_Update();
}

static void Menu_Draw(void)
{
    if (menu_page == PAGE_MAIN)
    {
        Draw_Main();
    }
    else
    {
        Draw_Sub();
    }
    menu_dirty = 0;
}

/* ---------------- 按键处理 ---------------- */

static void Menu_Key(uint8_t key)
{
    if (menu_page == PAGE_MAIN)
    {
        switch (key)
        {
            case 1:     /* 上移，到头绕回末行 */
                menu_cursor = (uint8_t)((menu_cursor + MAIN_ITEM_NUM - 1u) % MAIN_ITEM_NUM);
                menu_dirty  = 1;
                break;

            case 2:     /* 下移，到尾绕回首行 */
                menu_cursor = (uint8_t)((menu_cursor + 1u) % MAIN_ITEM_NUM);
                menu_dirty  = 1;
                break;

            case 3:     /* 进入子菜单 */
                menu_page  = (MenuPage)(PAGE_LED + menu_cursor);
                menu_dirty = 1;
                break;

            default:
                break;
        }
    }
    else
    {
        if (key == 4)       /* 返回主菜单 */
        {
            menu_page  = PAGE_MAIN;
            menu_dirty = 1;
        }
    }
}

/* ---------------- 对外接口 ---------------- */

void Menu_Init(void)
{
    Key_Init();
    menu_page   = PAGE_MAIN;
    menu_cursor = 0;        /* 上电默认箭头指向首行 */
    menu_dirty  = 1;
    Menu_Draw();
}

void Menu_Task(void)
{
    static uint32_t last_tick = 0;
    uint32_t        now       = HAL_GetTick();
    uint8_t         key;

    if ((now - last_tick) < MENU_TICK_MS)
    {
        return;
    }
    last_tick = now;

    key = Key_Scan();
    if (key != 0)
    {
        Menu_Key(key);
    }

    if (menu_dirty)
    {
        Menu_Draw();
    }
}
