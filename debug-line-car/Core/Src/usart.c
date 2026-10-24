/**
  ******************************************************************************
  * @file    usart.c
  * @brief   赛题 3b / 3c：上位机修改 / 读取变量 a
  *
  * 协议（一行一条，回车结尾，大小写不敏感）：
  *   发 "111"    → 变量 a = 111（不回复）
  *   发 "-45"    → 变量 a = -45
  *   发 "GET A"  → 回复当前 a 的值，例如 "111\r\n"
  *
  * 接收走中断 + 环形缓冲，主循环 Usart_Task() 里拼行、解析。
  * 只在收到 "GET A" 时才发送，避免多余数据干扰上位机读取。
  ******************************************************************************
  */

#include "usart.h"
#include "info.h"
#include "remote.h"

#include <string.h>

#define RX_BUF_SIZE   64
#define LINE_MAX      24

extern UART_HandleTypeDef huart1;

static uint8_t           rx_byte;
static volatile uint8_t  rx_buf[RX_BUF_SIZE];
static volatile uint16_t rx_head;
static volatile uint16_t rx_tail;

static char    line[LINE_MAX];
static uint8_t line_len;

/* ---------------------------------------------------------------- 初始化 */

void Usart_Init(void)
{
    rx_head  = 0;
    rx_tail  = 0;
    line_len = 0;
    line[0]  = '\0';

    HAL_UART_Receive_IT(&huart1, &rx_byte, 1);   /* 收第一个字节，之后在回调里续 */
}

/* ------------------------------------------------------------ 中断接收 */

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
        uint16_t next = (uint16_t)((rx_head + 1u) % RX_BUF_SIZE);

        if (next != rx_tail)            /* 缓冲没满才写，满了丢弃 */
        {
            rx_buf[rx_head] = rx_byte;
            rx_head = next;
        }

        HAL_UART_Receive_IT(&huart1, &rx_byte, 1);
    }
}

/* ---------------------------------------------------------------- 发送 */

static void SendString(const char *s)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)s, (uint16_t)strlen(s), 100);
}

/* 发送一个有符号十进制数 + 回车换行 */
static void SendNumber(int32_t v)
{
    char    buf[14];
    uint8_t n = 0;
    uint8_t i;
    char    tmp[12];
    uint8_t t = 0;
    uint32_t u;

    if (v < 0)
    {
        buf[n++] = '-';
        u = (uint32_t)(-v);
    }
    else
    {
        u = (uint32_t)v;
    }

    do
    {
        tmp[t++] = (char)('0' + (u % 10u));
        u /= 10u;
    } while (u && (t < 12u));

    for (i = t; i > 0u; i--)
    {
        buf[n++] = tmp[i - 1u];
    }

    buf[n++] = '\r';
    buf[n++] = '\n';

    HAL_UART_Transmit(&huart1, (uint8_t *)buf, n, 100);
}

/* ---------------------------------------------------------------- 解析 */

/* 是不是一个合法整数：可选负号 + 至少一位数字 */
static uint8_t IsNumber(const char *s)
{
    uint8_t digits = 0;

    if (*s == '-')
    {
        s++;
    }

    while (*s)
    {
        if ((*s < '0') || (*s > '9'))
        {
            return 0;
        }
        digits++;
        s++;
    }

    return (digits > 0) ? 1u : 0u;
}

static int32_t ParseInt(const char *s)
{
    int32_t v     = 0;
    uint8_t neg   = 0;

    if (*s == '-')
    {
        neg = 1;
        s++;
    }

    while ((*s >= '0') && (*s <= '9'))
    {
        v = (int32_t)(v * 10 + (int32_t)(*s - '0'));
        s++;
    }

    return neg ? -v : v;
}

/* 转大写，便于比较 "get a" */
static void ToUpper(char *s)
{
    while (*s)
    {
        if ((*s >= 'a') && (*s <= 'z'))
        {
            *s = (char)(*s - 'a' + 'A');
        }
        s++;
    }
}

/* ------------------------------------------------------------ 主循环 */

void Usart_Task(void)
{
    int c;

    while ((c = (rx_head == rx_tail) ? -1 : rx_buf[rx_tail]) >= 0)
    {
        rx_tail = (uint16_t)((rx_tail + 1u) % RX_BUF_SIZE);

        /* 遥控模式：任何可见字符都交给遥控处理，不参与拼行。
         * 这样发个 X 也能收到 ST:STOP，能用来判断链路是否通。 */
        if (Remote_IsActive() && (c >= 0x20) && (c <= 0x7E))
        {
            Remote_Handle((char)c);
            line_len = 0;
            continue;
        }

        if ((c == '\n') || (c == '\r'))
        {
            if (line_len > 0)
            {
                line[line_len] = '\0';

                ToUpper(line);

                if (strcmp(line, "GET A") == 0)
                {
                    SendNumber(g_var_a);          /* 3c：把当前 a 发回去 */
                }
                else if (IsNumber(line))
                {
                    g_var_a = ParseInt(line);     /* 3b：改写 a，不回复 */
                }

                line_len = 0;
            }
        }
        else if ((c == ' ') && (line_len == 0))
        {
            /* 行首空格忽略 */
        }
        else if (line_len < (LINE_MAX - 1u))
        {
            line[line_len++] = (char)c;
        }
        else
        {
            line_len = 0;                        /* 太长，丢掉这一行 */
        }
    }
}
