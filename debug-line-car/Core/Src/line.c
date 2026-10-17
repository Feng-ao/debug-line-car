#include "line.h"
#include "sensor.h"

static int16_t last_error = 0;

void Line_Init(void)
{
    last_error = 0;
}

uint8_t Line_OnBits(void)
{
    return (uint8_t)(~Sensor_Read() & 0xFFu);   /* 1 = 压线 */
}

uint8_t Line_OnCount(void)
{
    uint8_t v = Line_OnBits();
    uint8_t n = 0;
    uint8_t i;

    for (i = 0; i < 8; i++)
    {
        if (v & (1u << i)) n++;
    }
    return n;
}

int16_t Line_Error(void)
{
    uint8_t v   = Line_OnBits();
    uint8_t n   = 0;
    int32_t sum = 0;
    int8_t  i;

    for (i = 0; i < 8; i++)
    {
        if (v & (1u << i))
        {
            sum += (int32_t)(i * 2 - 7);    /* -7 -5 -3 -1 +1 +3 +5 +7 */
            n++;
        }
    }

    if (n == 0)
    {
        return last_error;                  /* 丢线：沿用上次偏差，等下一帧 */
    }

    last_error = (int16_t)((sum * 100) / n);

#if LINE_REVERSED
    last_error = (int16_t)(-last_error);
#endif

    return last_error;
}

int16_t Line_LastError(void)
{
    return last_error;
}
