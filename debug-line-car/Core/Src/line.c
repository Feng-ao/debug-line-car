#include "line.h"
#include "sensor.h"

static int16_t last_error = 0;
static int8_t  last_dir   = 0;      /* 最后一次看到线偏在哪边：-1 左，+1 右 */

void Line_Init(void)
{
    last_error = 0;
    last_dir   = 0;
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
        /* 丢线：不能沿用"上次偏差"——直角处最后一次偏差往往是 0，
         * 那样车会直冲出去。改成按最后看到的方向给满量程，逼它转回来。 */
        if (last_dir > 0)  return 700;
        if (last_dir < 0)  return -700;
        return last_error;
    }

    last_error = (int16_t)((sum * 100) / n);

#if LINE_REVERSED
    last_error = (int16_t)(-last_error);
#endif

    /* 只在大偏差时记录方向，避免直道上的小抖动把方向带偏 */
    if (last_error > 300)       last_dir = 1;
    else if (last_error < -300) last_dir = -1;

    return last_error;
}

int16_t Line_LastError(void)
{
    return last_error;
}
