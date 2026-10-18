#include "menu.h"
#include "oled.h"
#include "key.h"
#include "led.h"
#include "info.h"
#include "sensor.h"
#include "line.h"
#include "motor.h"
#include "buzzer.h"
#include "track.h"
#include "gogogo.h"

/* 赛题 1：OLED 多级菜单（主菜单 3 分 + 子菜单 3 分）
 *
 * 主菜单：LED 控制 / 信息显示 / 巡线功能 / 拓展功能
 *   键1 = 箭头上移    键2 = 箭头下移
 *   键3 = 进入子菜单  键4 = 返回主菜单
 *
 * 显示资源：128x64，F6x8 字库 => 每行 21 字符、共 8 行（page 0~7）
 */

#define MAIN_ITEM_NUM   5
#define MENU_TICK_MS    10u   /* 按键扫描周期 */

/* 主菜单 5 项（英文短标签，字库只有 ASCII） */
static const char *const main_items[MAIN_ITEM_NUM] = {
    "LED CONTROL",
    "INFO SHOW",
    "LINE TRACK",
    "EXTENSION",
    "DEBUG"
};

/* 页面状态：0 = 主菜单，1~5 = 对应子菜单 */
typedef enum
{
    PAGE_MAIN  = 0,
    PAGE_LED   = 1,
    PAGE_INFO  = 2,
    PAGE_TRACK = 3,
    PAGE_EXT   = 4,
    PAGE_DEBUG = 5
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
        OLED_ShowChar(18, (uint8_t)(2 + i), (i == menu_cursor) ? '>' : ' ');
        OLED_ShowString(30, (uint8_t)(2 + i), main_items[i]);
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

/* ---------------- 赛题 4a / 4b：巡线页 ---------------- */

static void FmtTime(char *b, uint32_t ms)
{
    uint32_t s = ms / 1000u;
    uint32_t f = (ms % 1000u) / 10u;
    uint8_t  p = 0;

    b[p++] = 'T'; b[p++] = '='; b[p++] = ' ';
    if (s >= 100u) b[p++] = (char)('0' + (s / 100u) % 10u);
    if (s >= 10u)  b[p++] = (char)('0' + (s / 10u) % 10u);
    b[p++] = (char)('0' + s % 10u);
    b[p++] = '.';
    b[p++] = (char)('0' + (f / 10u) % 10u);
    b[p++] = (char)('0' + f % 10u);
    b[p++] = 's';
    b[p]   = '\0';
}

static void Track_DrawBits(void)
{
    uint8_t mask = Sensor_Read();
    uint8_t i;

    for (i = 0; i < 8; i++)
    {
        OLED_ShowChar((uint8_t)(10u + i * 14u), 3, ((mask >> i) & 1u) ? '1' : '0');
    }
}

static void Track_DrawInfo(void)
{
    char buf[10];
    char lap[8];

    lap[0] = 'L'; lap[1] = 'A'; lap[2] = 'P'; lap[3] = ' ';
    lap[4] = (char)('0' + (Track_GetLapDone() % 10u));
    lap[5] = '/';
    lap[6] = (char)('0' + (Track_GetLapTarget() % 10u));
    lap[7] = '\0';
    OLED_ShowStringCenter(1, lap);

    FmtTime(buf, Track_GetTimeMs());
    OLED_ShowStringCenter(5, buf);

    OLED_ShowString(4, 6, "E=");
    OLED_ShowNum(16, 6, Track_GetError(), 1);
    OLED_ShowString(62, 6, "L=");
    OLED_ShowNum(74, 6, Track_GetTurn(), 1);

    OLED_ShowString(4, 4, "J:");
    OLED_ShowNum(10, 4, Track_GetJuncIdx(), 1);

    /* M: 最后一次压线位图；E: 触发最近一次拐角/路口时的位图（停车后看） */
    {
        uint8_t m = Track_GetLastMask();
        uint8_t e = Track_GetEventMask();
        char    hx[3];

        OLED_ShowString(30, 4, "M:");
        hx[0] = (m >> 4) < 10u ? (char)('0' + (m >> 4)) : (char)('A' + (m >> 4) - 10u);
        hx[1] = (m & 0x0Fu) < 10u ? (char)('0' + (m & 0x0Fu)) : (char)('A' + (m & 0x0Fu) - 10u);
        hx[2] = '\0';
        OLED_ShowString(42, 4, hx);

        OLED_ShowString(64, 4, "E:");
        hx[0] = (e >> 4) < 10u ? (char)('0' + (e >> 4)) : (char)('A' + (e >> 4) - 10u);
        hx[1] = (e & 0x0Fu) < 10u ? (char)('0' + (e & 0x0Fu)) : (char)('A' + (e & 0x0Fu) - 10u);
        OLED_ShowString(76, 4, hx);
    }
}

static void Draw_Track(void)
{
    uint8_t i;

    OLED_Clear();
    OLED_ShowStringCenter(0, "----- TRACK -----");
    for (i = 0; i < 8; i++)
    {
        OLED_ShowChar((uint8_t)(10u + i * 14u), 2, (char)('1' + i));
    }

    if (Track_GetState() == TRACK_RUN)
    {
        ShowCenter(7, "K4:STOP");
    }
    else if (Track_GetState() == TRACK_DONE)
    {
        ShowCenter(7, "K3:AGAIN K4:BACK");
    }
    else
    {
        ShowCenter(7, "K3:GO   K4:BACK");
    }

    Track_DrawBits();
    Track_DrawInfo();
    OLED_Update();
}

/* ---------------- 拓展页：GOGOGO 演示 ---------------- */

static void Ext_DrawStatus(void)
{
    char        buf[14];
    const char *label;
    uint32_t    ms = Gogogo_GetMs();
    uint32_t    s;
    uint8_t     p = 0;

    switch (Gogogo_GetState())
    {
        case GOGOGO_BEEP: label = "BEEP";  break;
        case GOGOGO_RUN:  label = "RUN";   break;
        case GOGOGO_DONE: label = "DONE";  break;
        default:          label = "READY"; break;
    }

    while ((*label != '\0') && (p < 11u))
    {
        buf[p++] = *label++;
    }

    if ((Gogogo_GetState() == GOGOGO_BEEP) || (Gogogo_GetState() == GOGOGO_RUN))
    {
        s = ms / 1000u;
        buf[p++] = ' ';
        if (s >= 10u) buf[p++] = (char)('0' + (s / 10u) % 10u);
        buf[p++] = (char)('0' + s % 10u);
        buf[p++] = 's';
    }

    buf[p] = '\0';
    OLED_ShowStringCenter(4, buf);
}

static void Draw_Ext(void)
{
    OLED_Clear();
    OLED_ShowStringCenter(0, "----- EXTEND -----");
    OLED_ShowString(6, 2, "> GOGOGO");
    Ext_DrawStatus();
    OLED_ShowStringCenter(7, "K3:START K4:BACK");
    OLED_Update();
}

/* ---------------- 台架自检页 ---------------- */

static void Draw_Debug(void)
{
    OLED_Clear();
    OLED_ShowStringCenter(0, "----- DEBUG -----");
    OLED_ShowString(2, 2, "K1:L  K2:R  K3:BOTH");
    OLED_ShowStringCenter(4, "HOLD TO RUN");
    OLED_ShowStringCenter(6, "K4: BACK");
    OLED_ShowString(4, 7, "L=");
    OLED_ShowString(62, 7, "R=");
    OLED_Update();
}

static void Debug_DrawLive(int16_t l, int16_t r)
{
    OLED_ShowString(4, 7, "L=");
    OLED_ShowNum(16, 7, l, 1);
    OLED_ShowString(62, 7, "R=");
    OLED_ShowNum(74, 7, r, 1);
    OLED_UpdatePage(7);
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
    else if (menu_page == PAGE_INFO)
    {
        Info_Draw();
    }
    else if (menu_page == PAGE_TRACK)
    {
        Draw_Track();
    }
    else if (menu_page == PAGE_EXT)
    {
        Draw_Ext();
    }
    else if (menu_page == PAGE_DEBUG)
    {
        Draw_Debug();
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

                if (menu_page == PAGE_DEBUG)
                {
                    Buzzer_Beep(120);             /* 顺便验一下蜂鸣器 */
                }
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
    else if (menu_page == PAGE_TRACK)
    {
        switch (key)
        {
            case 1:     /* 圈数 1 <-> 2（跑动中不允许改） */
                if (Track_GetState() != TRACK_RUN)
                {
                    Track_SetLapTarget((Track_GetLapTarget() == 1u) ? 2u : 1u);
                    menu_dirty = 1;
                }
                break;

            case 3:     /* 发车（或重跑） */
                Track_Start();
                menu_dirty = 1;
                break;

            case 4:     /* 跑动中：紧急停车；停止状态：返回主菜单 */
                if (Track_GetState() == TRACK_RUN)
                {
                    Track_Stop();
                    menu_dirty = 1;
                }
                else
                {
                    Motor_Stop();
                    menu_page  = PAGE_MAIN;
                    menu_dirty = 1;
                }
                break;

            default:
                break;
        }
    }
    else if (menu_page == PAGE_EXT)
    {
        switch (key)
        {
            case 3:     /* 启动 GOGOGO */
                Gogogo_Start();
                menu_dirty = 1;
                break;

            case 4:     /* 中断并返回主菜单 */
                Gogogo_Stop();
                menu_page  = PAGE_MAIN;
                menu_dirty = 1;
                break;

            default:
                break;
        }
    }
    else if (menu_page == PAGE_DEBUG)
    {
        if (key == 4)
        {
            Motor_Stop();
            menu_page  = PAGE_MAIN;
            menu_dirty = 1;
        }
    }
    else
    {
        if (key == 4)       /* 其余子菜单：键4 返回主菜单 */
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
    Info_Init();
    Sensor_Init();
    Line_Init();
    Buzzer_Init();
    Motor_Init();
    Track_Init();
    Gogogo_Init();
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

    /* 巡线控制：10ms 一拍（内部不在跑就直接返回） */
    Track_Task();

    /* 巡线状态变化（自动计圈停车 / 丢线保护停车）时，整页重绘一次，
     * 否则底部提示还停留在“K4:STOP”，会让人以为车卡住了 */
    {
        static TrackState last_track_state = TRACK_IDLE;

        if (Track_GetState() != last_track_state)
        {
            last_track_state = Track_GetState();
            menu_dirty = 1;
        }
    }

    /* 拓展页 GOGOGO：蜂鸣 3 秒 -> 直行 */
    Gogogo_Task();

    if (menu_page == PAGE_EXT)
    {
        static uint32_t ext_tick = 0;

        if ((now - ext_tick) >= 100u)
        {
            ext_tick = now;
            Ext_DrawStatus();
            OLED_UpdatePage(4);
        }
    }

    /* 巡线页：8 路状态变化即时刷；时间/偏差每 100ms 刷一次 */
    if (menu_page == PAGE_TRACK)
    {
        static uint32_t track_tick = 0;
        static uint8_t  last_mask  = 0xFF;
        uint8_t         m          = Sensor_Read();

        if (m != last_mask)
        {
            last_mask = m;
            Track_DrawBits();
            OLED_UpdatePage(3);
        }

        if ((now - track_tick) >= 100u)
        {
            track_tick = now;
            Track_DrawInfo();
            OLED_UpdatePage(1);
            OLED_UpdatePage(4);
            OLED_UpdatePage(5);
            OLED_UpdatePage(6);
        }
    }

    /* 台架自检：按住哪个键就转哪个轮子 */
    if (menu_page == PAGE_DEBUG)
    {
        static uint32_t dbg_tick = 0;
        int16_t l = 0;
        int16_t r = 0;

        if (Key_IsDown(1) || Key_IsDown(3)) l = 300;
        if (Key_IsDown(2) || Key_IsDown(3)) r = 300;

        Motor_Set(l, r);

        if ((now - dbg_tick) >= 100u)
        {
            dbg_tick = now;
            Debug_DrawLive(l, r);
        }
    }

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
