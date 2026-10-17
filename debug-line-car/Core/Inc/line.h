#ifndef __LINE_H
#define __LINE_H

#include "main.h"

/* 巡线偏差计算
 * 传感器实测：黑线压住时该位读 0，白底读 1
 * 权重：X1 = -7 ... X8 = +7（X1 在车的左侧）
 * 偏差为正表示线偏右，为负表示线偏左；全部白底时沿用上一次的值
 */

#define LINE_REVERSED   0      /* 装反了（X1 在右）就改成 1 */

void    Line_Init(void);
uint8_t Line_OnBits(void);     /* 压线的位图，bit0 = X1 */
uint8_t Line_OnCount(void);    /* 几路压在线上 */
int16_t Line_Error(void);      /* -700 ~ +700 */
int16_t Line_LastError(void);

#endif
