#include "menu.h"
#include "oled.h"
#include "key.h"
#include "led.h"

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
static uint8_t  led_dirty;        /* 1 = 只重绘 LED 状态两行 */

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

/* 只改两行状态文字，不刷屏 */
static void Draw_LedStatusText(void)
{
    OLED_ShowString(40, 3, LED1_IsOn() ? "ON " : "OFF");
    OLED_ShowString(40, 4, LED2_IsOn() ? "ON " : "OFF");
}

/* 只刷新状态所在的两页（整屏刷新会闪） */
static void Draw_LedStatus(void)
{
    Draw_LedStatusText();
    OLED_UpdatePage(3);
    OLED_UpdatePage(4);
}

/* 赛题 2：LED 控制子菜单 */
static void Draw_Led(void)
{
    OLED_Clear();
    ShowCenter(0, "LED CONTROL");
    ShowCenter(1, "----------------");
    OLED_ShowString(4, 3, "LED1:");
    OLED_ShowString(4, 4, "LED2:");
    OLED_ShowString(7, 6, "K1:ON/OFF  K2:BLINK");
    ShowCenter(7, "K4:BACK");
    Draw_LedStatusText();
    OLED_Update();
}

static void Menu_Draw(void)
{
    if (menu_page == PAGE_MAIN)
    {
        Draw_Main();
    }
    else if (menu_page == PAGE_LED)
    {
        Draw_Led();
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
    else if (menu_page == PAGE_LED)
    {
        switch (key)
        {
            case 1:     /* 常亮 / 常灭 来回切 */
                LED_SetMode((LED_GetMode() == LED_MODE_ON) ? LED_MODE_OFF : LED_MODE_ON);
                menu_dirty = 1;
                break;

            case 2:     /* 交替闪烁 */
                LED_SetMode(LED_MODE_ALT);
                menu_dirty = 1;
                break;

            case 4:     /* 返回主菜单，同时熄灭 */
                LED_SetMode(LED_MODE_OFF);
                menu_page  = PAGE_MAIN;
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
    LED_Init();
    menu_page   = PAGE_MAIN;
    menu_cursor = 0;        /* 上电默认箭头指向首行 */
    menu_dirty  = 1;
    led_dirty   = 0;
    Menu_Draw();
}

void Menu_Task(void)
{
    static uint32_t last_tick = 0;
    uint32_t        now       = HAL_GetTick();
    uint8_t         key;

    /* LED 交替闪烁必须一直在跑，不受菜单刷新节奏影响 */
    if (LED_Task())
    {
        led_dirty = 1;
    }

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
        led_dirty = 0;
    }
    else if (led_dirty && menu_page == PAGE_LED)
    {
        Draw_LedStatus();
        led_dirty = 0;
    }
}
