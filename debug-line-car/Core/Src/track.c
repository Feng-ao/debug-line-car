#include "track.h"
#include "line.h"
#include "motor.h"
#include "buzzer.h"

extern UART_HandleTypeDef huart1;

/* 赛题 4b：巡线一圈
 * 控制律：偏差 PD -> 差速转向
 * 计圈：左右轮速差对时间积分，绝对值超过 LAP_UNIT 判定跑满一圈
 */

#define BASE_SPEED        260    /* 直道速度：先求跑通，不求快 */
#define KP_NUM            110    /* 比例：偏差 * 1.10 */
#define KD_NUM            160    /* 微分：偏差变化 * 1.60 */
#define TURN_LIMIT        900    /* 转向量上限，允许内侧轮反转，急弯才能咬住 */
#define CURVE_SLOW_DIV      4    /* 偏差每 4 点，整体速度降 1 点 */
#define SPEED_MIN         200    /* 转向时最低速度 */

/* 拐角逻辑总开关：0 = 完全交给 PD 跟线。
 * 这张图的角都是 R90 圆角、线是连续的，理论上不需要刹停原地转。
 * 先验证 PD 能不能自己跟过弯。 */
#define CORNER_ENABLE       0
#define LAP_UNIT     5000000    /* 旧的转向积分阈值（已不用，保留备查） */

/* 计圈改用"数全黑路口"：一次十字/丁字 = 一次全黑。
 * 现场数过：第 5 次正好回到起点。 */
#define LAP_FF_PER_LAP     99    /* 已弃用（计数有误差），改用下面的计时法 */
#define LAP_FF_COOL_MS    400    /* 两次全黑之间的最小间隔，防止压黑时重复计数 */

/* 计圈改用"计时法"：车速固定，一圈时长基本恒定。
 * LAP_TIME_MS = 0 表示未标定（不启用）；标定方法：
 *   跑完整一圈，到起点按 K4 停车，读屏上 T= 或日志里 T= 的秒数，
 *   换算成毫秒填进来，再留 1~2% 余量。 */
#define LAP_TIME_MS     178000   /* 实测一圈 178 秒（2026-10-30） */
#define TRACK_INVERT_TURN   0    /* 发现越跑越偏就改成 1 */
#define LOST_STOP_TICKS   100    /* 连续 1 秒看不到线就停车 */

/* 断路处理：线突然"整片消失"且消失前车是正的（偏差小）→ 直行穿过去 */
#define GAP_CONFIRM_TICKS   2    /* 连续 2 帧全白才确认（防抖） */
#define GAP_SPEED         380    /* 断路里直行的速度 */
#define GAP_TIMEOUT_MS   1500    /* 断路里最多直行 1.5 秒，超时说明冲出去了 */
#define GAP_RECOVER_MS    250    /* 出断路后的抑制时间 */
#define GAP_RECOVER_TURN  350    /* 抑制期内最大转向量，防止接上线瞬间画圈 */
#define GAP_ERR_MAX       250    /* 断路判定：丢线前的偏差必须很小 */
#define GAP_STRAIGHT_MS   300    /* 断路判定：丢线前必须已经稳定直行这么久。
                                  * 真断路前面是长直道；小方框拐角是刚拐完就丢线，
                                  * 没有稳定直行 —— 用它把两者分开 */
#define GAP_MAX_BITS        2    /* 断路判定：最后一刻压线的路数上限。
                                  * 18mm 的线正常只覆盖 1~2 路；3 路以上说明是一团黑
                                  * （拐角/路口），不是断口 */

/* 路口（十字/丁字）处理：连续几帧 8 路全黑 = 站在路口上
 *
 * JUNC_TABLE_ENABLE = 1 时才按"第几个路口"查表硬拐。
 * 这张表不通用（起点一挪就得重排），所以默认关闭，走下面的"固定规则"。
 */
#define JUNC_TABLE_ENABLE    0   /* 0 = 通用模式（推荐），1 = 按路线表 */

#define JUNC_CONFIRM_TICKS   3   /* 连续 3 帧全黑确认 */
#define JUNC_COOL_MS       400   /* 出路口后的冷却，防止重复触发 */
#define JUNC_PIVOT_FWD    1000   /* 转弯时外轮速度 */
#define JUNC_PIVOT_BACK    400   /* 转弯时内轮反转速度 */
#define JUNC_MAX             8

/* 通用模式下的固定规则：遇到 8 路全黑一律直行
 *   0 = 直行（十字就是这么过的，能拿 2 分）
 * 丁字口靠"全黑之前的反应式左右修正"来选边——两侧看到的先后顺序不同，
 * 先压到哪边就往哪边拐，和起点位置无关。
 */
#define JUNC_DEFAULT_DIR      0
#define JUNC_DEFAULT_MS     250

/* 直角/急弯：分三段执行 —— 刹停 → 原地转 → 停稳复查 */
#define CORNER_MAX_MS     2500   /* 单次拐弯总超时 */
#define CORNER_SETTLE_MS   150   /* 转到位后停多久复查 */

/* 拐弯前先刹一下：车停稳再原地转，避免带着惯性滑过拐角 */
#define CORNER_BRAKE_MS    120

/* 拐弯"算拐正"的判据：中间（X4/X5）压线，且总压线路数不超过 3 */
#define CORNER_CENTER_MASK 0x18u
#define CORNER_MAX_DOTS      3

/* ---- 六边形进环 ----
 * 关键：这台车直线时是 X3/X4 压线（E≈-200），不能再用 |偏差| 判断直行，
 * 改成用"压线路数"判断：1~2 路 = 干净直线；3~4 路 = 出现分叉。
 * 分叉要连续 3 帧才算 Y 形岔口（环入口）；如果 2 帧内冲到 ≥5 路，
 * 那是直角/丁字的一团黑，放弃，交给拐角逻辑。 */
#define HEX_ENABLE           1
#define HEX_STRAIGHT_MS    150   /* 环入口前必须是这么长的稳定直道 */
#define HEX_ENTER_MS       300   /* 往左带多久 */
#define HEX_COOL_MS      25000   /* 冷却：要盖住绕一圈的时间（实测 >6 秒），
                                  * 但又要短于"第一个环到第二个环"的路程 */
#define HEX_LEFT_SPD       150
#define HEX_RIGHT_SPD      750

/* 出弯稳定区：拐角之间距离很短（280~400mm），出弯后必须减速，
 * 让 PD 有足够多的控制周期把车身摆正，否则到下一个弯还是歪的 */
#define CORNER_ZONE_MS     900   /* 出弯后这段时间内限速（覆盖短间距段） */
#define CORNER_ZONE_SPEED  290   /* 限速值（小方框 225×280 全靠它压住） */
#define CORNER_FAST_KP     130   /* 出弯后短时间内的强纠偏 KP */
#define CORNER_FAST_KP_MS  280

/* 调试遥测：跑的时候每 100ms 从串口吐一行 M/E/R/阶段，
 * 车跑完把手机终端的内容发我，就能知道它到底看到了什么。
 * 只输出，不参与控制，关掉只需改成 0。 */
#define TELEM_ENABLE        1

/* 过弯补偿：拐正之后继续朝拐弯方向多打一点，抵消机械间隙 */
#define CORNER_BIAS         80
#define CORNER_BIAS_MS     200

/* 每个路口的动作表：按"第几个路口"依次执行
 *   dir = +1 右转, -1 左转, 0 直行
 *   ms  = 动作持续时间（要现场标定）
 * 这张表就是这条赛道的路线，按实际顺序改。
 */
typedef struct
{
    int8_t   dir;
    uint16_t ms;
} JunctionAction;

#if JUNC_TABLE_ENABLE
static const JunctionAction junc_actions[JUNC_MAX] = {
    { +1, 600 },   /* 第 1 个路口 */
    { -1, 600 },   /* 第 2 个路口 */
    { +1, 600 },   /* 第 3 个路口 */
    { -1, 600 },   /* 第 4 个路口 */
    {  0, 300 },   /* 第 5 个：十字，直行 */
    { +1, 600 },
    { -1, 600 },
    {  0, 300 }
};
#endif

static TrackState state;
static uint32_t   start_ms;
static uint32_t   elapsed_ms;
static int16_t    last_err;
static int32_t    turn_acc;
static uint16_t   lost_cnt;
static uint8_t    lap_target = 1;
static uint8_t    lap_done   = 0;
static uint8_t    ff_cnt     = 0;   /* 已经过了几个"全黑路口" */

/* 断路状态 */
static uint8_t    in_gap;        /* 1 = 正在断路里直行 */
static uint8_t    gap_cnt;       /* 连续全白帧数 */
static uint32_t   gap_t0;
static uint8_t    last_on_mask;  /* 最后一次压线时的传感器位图 */
static int16_t    last_good_err; /* 最后一次压线时的偏差（丢线后不再更新） */
static uint16_t   straightMs;    /* 已经稳定直行了多少毫秒 */
static uint32_t   gap_exit_ms;   /* 刚出断路的时间，用于短时间内抑制猛打舵 */

/* 路口状态 */
static uint8_t    in_junc;       /* 1 = 正在执行路口动作 */
static uint8_t    junc_cnt;      /* 连续全黑帧数 */
static uint32_t   junc_t0;
static uint32_t   junc_cool_ms;  /* 上次出路口的时间 */
static uint8_t    junc_idx;      /* 已经过了几个路口 */

/* 直角锁定 */
static uint8_t    in_corner;
static uint8_t    corner_phase;  /* 0=刹停 1=原地转 2=停稳复查 */
static uint8_t    edge_cnt;      /* |偏差| ≥ 500 的连续帧数 */
static uint8_t    corner_fix_cnt;/* 本次拐弯已经修正了几次 */
static uint32_t   corner_t0;
static uint32_t   corner_phase_t;
static int8_t     corner_dir;
static uint8_t    event_mask;    /* 触发拐角/路口那一刻的位图，停车后可以看 */
static uint32_t   corner_end_ms; /* 拐角结束时刻，用于过弯补偿 */
static int8_t     corner_bias_dir;

/* 六边形进环状态 */
static uint8_t    in_hex;        /* 1 = 已在六边形里 */
static uint8_t    hex_cnt;       /* 出环确认计数 */
static uint8_t    clean_cnt;     /* 干净直道的衰减计数（1~2 路） */
static uint8_t    probe_cnt;     /* 3~4 路的连续帧数 */
static uint8_t    hex_act;       /* 0=无 1=进环拐 2=出环拐 */
static uint32_t   hex_t0;
static uint32_t   hex_in_ms;     /* 进环时刻 */
static uint32_t   hex_cool_ms;   /* 上次进环动作的时间（冷却用） */

void Track_Init(void){
    state      = TRACK_IDLE;
    elapsed_ms = 0;
    last_err   = 0;
    turn_acc   = 0;
    lost_cnt   = 0;
    Motor_Stop();
}

void Track_Start(void)
{
    if (state == TRACK_RUN)
    {
        return;
    }

    Buzzer_Beep(150);           /* 发车提示音 */

    last_err   = 0;
    turn_acc   = 0;
    lost_cnt   = 0;
    lap_done   = 0;
    ff_cnt     = 0;
    in_gap     = 0;
    gap_cnt    = 0;
    last_on_mask = 0;
    last_good_err = 0;
    gap_exit_ms  = 0;
    in_junc      = 0;
    junc_cnt     = 0;
    junc_idx     = 0;
    junc_cool_ms = 0;
    in_corner      = 0;
    corner_phase   = 0;
    corner_fix_cnt = 0;
    corner_t0      = 0;
    corner_phase_t = 0;
    corner_dir     = 0;
    corner_end_ms  = 0;
    corner_bias_dir = 0;
    in_hex         = 0;
    hex_cnt        = 0;
    clean_cnt      = 0;
    probe_cnt      = 0;
    hex_act        = 0;
    hex_t0         = 0;
    /* 让冷却"已过期"：否则上电后 6 秒内进环判据全部失效，
     * 从六边形前面直接发车时会撞在这个盲区里 */
    hex_cool_ms    = HAL_GetTick() - (uint32_t)HEX_COOL_MS;
    elapsed_ms = 0;
    start_ms   = HAL_GetTick();
    state      = TRACK_RUN;
}

void Track_Stop(void)
{
    Motor_Stop();

    if (state == TRACK_RUN)
    {
        elapsed_ms = HAL_GetTick() - start_ms;
        state      = TRACK_DONE;
        Buzzer_Beep(150);       /* 到点提示音 */
    }
}

/* 判断线最后偏在哪边：先用误差，误差接近 0 时改用"最后压线的位置"平均 */
static int8_t Track_LastDir(void)
{
    int16_t le  = Line_LastError();
    int16_t sum = 0;
    uint8_t n   = 0;
    uint8_t i;

    if (le > 50)  return 1;
    if (le < -50) return -1;

    for (i = 0; i < 8; i++)
    {
        if (last_on_mask & (1u << i))
        {
            sum += (int16_t)i;
            n++;
        }
    }

    if (n == 0) return 0;
    if ((sum * 2) > (int16_t)(7 * n)) return 1;    /* 平均位置 > 3.5 → 偏右 */
    if ((sum * 2) < (int16_t)(7 * n)) return -1;
    return 0;
}

/* 数一个字节里有几位是 1 */
static uint8_t PopCount8(uint8_t v)
{
    uint8_t n = 0;

    while (v)
    {
        n += (uint8_t)(v & 1u);
        v >>= 1;
    }
    return n;
}

void Track_Task(void)
{
    int16_t err;
    int16_t delta;
    int32_t turn;
    int16_t left;
    int16_t right;

    if (state != TRACK_RUN)
    {
        return;
    }

    elapsed_ms = HAL_GetTick() - start_ms;

    /* ---- 路口处理 ----
     * 通用模式下**整段关掉**：PD 在八路全黑时偏差本来就是 0，自然会直行，
     * 不需要额外强制。开着它反而会在"厚黑块"（拐角、起点十字）上反复触发，
     * 把该拐的地方掰成直行。
     * 只有启用路线表（JUNC_TABLE_ENABLE=1）时才按表硬拐。 */
#if JUNC_TABLE_ENABLE
    if (!in_junc)
    {
        /* 只有"八路全黑"才当路口 */
        if ((Line_OnCount() == 8) && ((HAL_GetTick() - junc_cool_ms) > JUNC_COOL_MS))
        {
            if (++junc_cnt >= JUNC_CONFIRM_TICKS)
            {
                in_junc    = 1;
                junc_t0    = HAL_GetTick();
                junc_cnt   = 0;
                event_mask = last_on_mask;
            }
        }
        else
        {
            junc_cnt = 0;
        }
    }

    if (in_junc)
    {
        const JunctionAction *a = &junc_actions[junc_idx % JUNC_MAX];

        if (a->dir > 0)
        {
            Motor_Set(JUNC_PIVOT_FWD, -JUNC_PIVOT_BACK);
        }
        else if (a->dir < 0)
        {
            Motor_Set(-JUNC_PIVOT_BACK, JUNC_PIVOT_FWD);
        }
        else
        {
            Motor_Set(GAP_SPEED, GAP_SPEED);
        }

        if ((HAL_GetTick() - junc_t0) >= a->ms)
        {
            in_junc      = 0;
            junc_idx++;
            junc_cool_ms = HAL_GetTick();
            gap_exit_ms  = HAL_GetTick();
            last_err     = 0;
            last_on_mask = 0;
        }
        return;
    }
#endif

    /* ---- 断路识别：整片线消失，且最后压线的位置在阵列中间 ----
     * 判据不用"偏差大小"，改看"最后是哪几路压着线"：
     *   中间四路（bit2~bit5）压线后消失 → 正对着断口 → 直行
     *   只有最边上两路压线后消失   → 线是从侧面滑走的 → 直角/急弯，要打舵
     */
    {
        uint8_t on = Line_OnBits();
        if (on != 0)
        {
            last_on_mask  = on;
            /* 关键：丢线后 Line_Error() 会返回 ±700，这里只在线还在时更新，
             * 否则断路判据里的"偏差必须很小"第一帧之后就不成立了 */
            last_good_err = Line_Error();

            /* 累计"稳定直行"时长：偏差小、压线少 */
            if ((last_good_err <= GAP_ERR_MAX) && (last_good_err >= -GAP_ERR_MAX) &&
                (PopCount8(on) <= GAP_MAX_BITS))
            {
                if (straightMs < 60000u) straightMs += 10u;
            }
            else
            {
                straightMs = 0;
            }
        }
        else
        {
            /* 丢线时不立刻清零：断路判据要用"丢线前"的直行时长。
             * 让它按帧衰减，2 帧之内仍然有效（恰好够 GAP_CONFIRM_TICKS）。 */
            straightMs = (straightMs > 80u) ? (uint16_t)(straightMs - 80u) : 0u;
        }
    }

    if (!in_gap)
    {
        if (Line_OnCount() == 0)
        {
            /* 四个条件同时满足才是断路：
             *   1) 最后压线在阵列中间（不是从边上滑走的）
             *   2) 丢线前偏差很小（车正对着断口）
             *   3) 最后一刻压线的路数 ≤2（线是窄的；3 路以上是一团黑 = 拐角）
             *   4) 连续几帧全白（防抖） */
            if (((last_on_mask & 0x3Cu) != 0) &&
                (straightMs >= GAP_STRAIGHT_MS) &&
                (last_good_err <= GAP_ERR_MAX) &&
                (last_good_err >= -GAP_ERR_MAX) &&
                (PopCount8(last_on_mask) <= GAP_MAX_BITS))
            {
                if (++gap_cnt >= GAP_CONFIRM_TICKS)
                {
                    in_gap  = 1;
                    gap_t0  = HAL_GetTick();
                    gap_cnt = 0;
                }
            }
            else
            {
                gap_cnt = 0;
            }
        }
        else
        {
            gap_cnt = 0;
        }
    }
    else if (Line_OnCount() > 0)
    {
        in_gap      = 0;                    /* 重新看到线，交还 PD */
        gap_exit_ms = HAL_GetTick();
        last_err    = Line_Error();         /* 刷新旧值，避免猛打舵 */
    }

    if (in_gap)
    {
        Motor_Set(GAP_SPEED, GAP_SPEED);      /* 断路里不看线，直行穿过去 */

        if ((HAL_GetTick() - gap_t0) > GAP_TIMEOUT_MS)
        {
            Track_Stop();                     /* 1.5 秒还没接到线，说明冲出去了 */
        }
        return;
    }

    /* ---- 六边形进环（只在"直行中"生效，绝不干扰拐角）----
     * 六边形顶点是 Y 形岔口：主线直行会跳过整个六边形，必须往左带一下。
     * 判据：连续直行 ≥150ms，且当前同时看到"左边"和"正中"两段黑。 */
#if HEX_ENABLE
    /* 正在往左带（进环动作） */
    if (hex_act != 0)
    {
        Motor_Set(HEX_LEFT_SPD, HEX_RIGHT_SPD);

        if ((HAL_GetTick() - hex_t0) >= HEX_ENTER_MS)
        {
            hex_act     = 0;
            last_err    = 0;
            hex_cool_ms = HAL_GetTick();      /* 冷却，防止在出口又触发一次 */
        }
        return;
    }

    /* 环入口判据（数据实测）：
     *   1) 前面必须是稳定长直道（弯道不可能满足，用 straightMs 当闸门）
     *   2) 左侧（X1/X2）突然有黑 —— 六边形的斜边插进阵列
     *   3) 中间那条主线还在（X4/X5 有黑）
     * 连续 2 帧确认，然后往左带一下就走，不做状态机。 */
    if ((HAL_GetTick() - hex_cool_ms) > HEX_COOL_MS)
    {
        uint8_t on = Line_OnBits();

        /* 实测：M=63 / M=61（两端有黑、中间白）整份日志只出现在环岔口，
         * 共 4 帧，特征本身就足够独特 —— 不再加"直行闸门"，
         * 因为岔口前几帧就是 3~4 路宽，闸门会被清零。 */
        if (((on & 0x07u) != 0) &&      /* 左端有黑（X1~X3） */
            ((on & 0xE0u) != 0) &&      /* 右端有黑（X6~X8） */
            ((on & 0x1Cu) == 0))        /* 中间全白（X3~X5）—— V 形岔口；
                                         * 十字的 0xFF 会被这条排除 */
        {
            /* 实测：整份日志里"X1/X2 有黑"只出现 1 次（M=19），
             * 特征是稀有的，单帧即可确认，不必等 2 帧 */
            hex_cnt = 0;
            hex_act = 1;
            hex_t0  = HAL_GetTick();
        }
        else
        {
            hex_cnt = 0;
        }
    }
#endif

    /* ---- 直角/急弯 ----
     * 触发：整条线消失
     * 结束：线重新出现在阵列"中间"（|偏差| 很小）才算拐正
     *        —— 只看到线不算数，45° 斜着也能看到，车身还是歪的
     */
    if (in_corner)
    {
        uint32_t now     = HAL_GetTick();
        uint8_t  aligned = 0;

        if (Line_OnCount() > 0)
        {
            uint8_t on = Line_OnBits();

            /* 中间压线 + 总路数不超过 3 就算摆正 */
            if (((on & CORNER_CENTER_MASK) != 0) &&
                (PopCount8(on) <= CORNER_MAX_DOTS))
            {
                aligned = 1;
            }
        }

        if (corner_phase == 0)                  /* 第一段：刹停 */
        {
            Motor_Stop();
            if ((now - corner_t0) >= CORNER_BRAKE_MS)
            {
                corner_phase = 1;
            }
        }
        else if (corner_phase == 1)             /* 第二段：原地转 */
        {
            if (aligned)
            {
                corner_phase   = 2;
                corner_phase_t = now;
                Motor_Stop();
            }
            else if (corner_dir > 0)
            {
                Motor_Set(JUNC_PIVOT_FWD, -JUNC_PIVOT_BACK);
            }
            else if (corner_dir < 0)
            {
                Motor_Set(-JUNC_PIVOT_BACK, JUNC_PIVOT_FWD);
            }
            else
            {
                Motor_Set(GAP_SPEED, GAP_SPEED);
            }
        }
        else if (corner_phase == 2)             /* 第三段：停稳复查 */
        {
            Motor_Stop();

            if ((now - corner_phase_t) >= CORNER_SETTLE_MS)
            {
                if (aligned)                    /* 稳住了，交还 PD */
                {
                    in_corner       = 0;
                    last_err        = Line_Error();
                    corner_end_ms   = now;
                    corner_bias_dir = corner_dir;
                }
                else                            /* 甩过头了，按当前偏差重新定方向再转 */
                {
                    int16_t e = Line_Error();

                    if (e > 50)       corner_dir = 1;
                    else if (e < -50) corner_dir = -1;

                    /* 方框这种小环境里会反复修正，最多 3 次就撒手，
                     * 否则会原地打转出不来 */
                    if (corner_fix_cnt < 3)
                    {
                        corner_fix_cnt++;
                        corner_phase = 1;
                    }
                    else
                    {
                        in_corner       = 0;
                        last_err        = e;
                        corner_end_ms   = now;
                        corner_bias_dir = corner_dir;
                        corner_fix_cnt  = 0;
                    }
                }
            }
        }

        if (in_corner && ((now - corner_t0) > CORNER_MAX_MS))
        {
            in_corner = 0;
            Track_Stop();
        }
        return;
    }
    else if (CORNER_ENABLE && ((Line_OnCount() == 0) || (edge_cnt >= 2)))
    {
        in_corner      = 1;
        corner_phase   = 0;
        corner_fix_cnt = 0;
        corner_dir     = Track_LastDir();
        corner_t0      = HAL_GetTick();
        corner_phase_t = corner_t0;
        event_mask     = last_on_mask;

        Motor_Stop();                           /* 先刹住 */
        return;
    }

    err   = Line_Error();
    delta = (int16_t)(err - last_err);

    /* 线快跑到阵列边上了（|偏差| ≥ 500）：
     * 实测这台车"全白"几乎不会出现，靠 n==0 触发拐角等于永远不触发。
     * 改用"偏差饱和"当拐角信号 —— 遥测里所有冲出去之前都是这个特征。 */
    if ((err >= 500) || (err <= -500))
    {
        if (edge_cnt < 200) edge_cnt++;
    }
    else
    {
        edge_cnt = 0;
    }

    /* 每 100ms 吐一行轨迹数据：位图 / 偏差 / 是否在环 / 拐弯阶段 */
#if TELEM_ENABLE
    {
        static uint32_t tele_tick = 0;

        if ((HAL_GetTick() - tele_tick) >= 100u)
        {
            static const char hexc[] = "0123456789ABCDEF";
            char    buf[64];
            uint8_t p   = 0;
            uint8_t on  = Line_OnBits();
            int32_t v   = err;
            char    tmp[8];
            uint8_t tn  = 0;

            tele_tick = HAL_GetTick();

            buf[p++] = 'M'; buf[p++] = '=';
            buf[p++] = hexc[(on >> 4) & 0x0Fu];
            buf[p++] = hexc[on & 0x0Fu];
            buf[p++] = ' ';
            buf[p++] = 'E'; buf[p++] = '=';

            if (v < 0) { buf[p++] = '-'; v = -v; }
            do { tmp[tn++] = (char)('0' + (v % 10)); v /= 10; } while (v && tn < 8u);
            while (tn) { buf[p++] = tmp[--tn]; }

            buf[p++] = ' ';
            buf[p++] = 'R'; buf[p++] = '='; buf[p++] = (char)(0x30 + hex_act);
            buf[p++] = ' ';
            buf[p++] = 'P'; buf[p++] = '='; buf[p++] = (char)('0' + corner_phase);

            /* L = 转向积分（旧方案，留作参考）；F = 已过的全黑路口数（计圈用） */
            buf[p++] = ' ';
            buf[p++] = 'F'; buf[p++] = '=';
            if (ff_cnt >= 100u) buf[p++] = (char)('0' + (ff_cnt / 100u));
            if (ff_cnt >= 10u)  buf[p++] = (char)('0' + ((ff_cnt / 10u) % 10u));
            buf[p++] = (char)('0' + (ff_cnt % 10u));
            buf[p++] = ' ';
            buf[p++] = 'L'; buf[p++] = '=';
            {
                int32_t lv = turn_acc;
                char    lt[12];
                uint8_t ln = 0;

                if (lv < 0) { buf[p++] = '-'; lv = -lv; }
                do { lt[ln++] = (char)('0' + (lv % 10)); lv /= 10; } while (lv && ln < 12u);
                while (ln) { buf[p++] = lt[--ln]; }
            }

            buf[p++] = ' ';
            buf[p++] = 'T'; buf[p++] = '=';
            {
                uint32_t sec = elapsed_ms / 1000u;
                char     st[8];
                uint8_t  sn = 0;

                do { st[sn++] = (char)('0' + (sec % 10u)); sec /= 10u; } while (sec && sn < 8u);
                while (sn) { buf[p++] = st[--sn]; }
            }

            buf[p++] = '\r'; buf[p++] = '\n';

            HAL_UART_Transmit(&huart1, (uint8_t *)buf, p, 50);
        }
    }
#endif
    last_err = err;

    /* 出弯后短时间内用更强的 KP，快速把车身拉正 */
    {
        int32_t kp = KP_NUM;

        if ((HAL_GetTick() - corner_end_ms) < CORNER_FAST_KP_MS)
        {
            kp = CORNER_FAST_KP;
        }

        turn = ((int32_t)err * kp + (int32_t)delta * KD_NUM) / 100;
    }

    /* 偏差大时额外加力：拐角处黑块会偏到一侧（比如 X1~X5 同时亮、偏差只有 −300），
     * 线性 KP 打不够，必须放大 */
    if ((err > 250) || (err < -250))
    {
        turn = turn * 18 / 10;
    }
    else if ((err > 120) || (err < -120))
    {
        turn = turn * 12 / 10;
    }

#if TRACK_INVERT_TURN
    turn = -turn;
#endif

    if (turn >  TURN_LIMIT) turn =  TURN_LIMIT;
    if (turn < -TURN_LIMIT) turn = -TURN_LIMIT;

    /* 过弯补偿：刚拐完的 320ms 内，朝拐弯方向额外多打 250，
     * 让车贴着内侧出去，别带着"外偏姿态"去面对下一个拐角 */
    if ((HAL_GetTick() - corner_end_ms) < CORNER_BIAS_MS)
    {
        turn += (int32_t)corner_bias_dir * CORNER_BIAS;

        if (turn >  TURN_LIMIT) turn =  TURN_LIMIT;
        if (turn < -TURN_LIMIT) turn = -TURN_LIMIT;
    }

    /* 刚出断路：先别猛打舵，否则一接上线就打转画圈 */
    if ((HAL_GetTick() - gap_exit_ms) < GAP_RECOVER_MS)
    {
        if (turn >  GAP_RECOVER_TURN) turn =  GAP_RECOVER_TURN;
        if (turn < -GAP_RECOVER_TURN) turn = -GAP_RECOVER_TURN;
    }

    /* 偏差越大整体越慢，给转向留出时间：直角和急弯的关键 */
    {
        int16_t abs_err = (err < 0) ? (int16_t)(-err) : err;
        int16_t base    = (int16_t)(BASE_SPEED - abs_err / CURVE_SLOW_DIV);

        if (base < SPEED_MIN) base = SPEED_MIN;

        /* 出弯稳定区：刚拐完的 900ms 内限速，先把姿态摆正再提速 */
        if (((HAL_GetTick() - corner_end_ms) < CORNER_ZONE_MS) && (base > CORNER_ZONE_SPEED))
        {
            base = CORNER_ZONE_SPEED;
        }

        left  = (int16_t)(base + turn);
        right = (int16_t)(base - turn);
    }

    Motor_Set(left, right);

    /* 丢线保护：一直看不到线说明冲出赛道了 */
    if (Line_OnCount() == 0)
    {
        lost_cnt++;
        if (lost_cnt >= LOST_STOP_TICKS)
        {
            Track_Stop();
            return;
        }
    }
    else
    {
        lost_cnt = 0;
    }

    /* 转向积分：直道左右轮速差为 0，弯道才累积，绕回一周约等于 360 度 */
    turn_acc += (int32_t)(right - left);

    /* ---- 计圈：数"八路全黑"的路口 ----
     * 每个十字/丁字都会出现一次全黑，现场数过：第 5 次正好回到起点。
     * 必须做边沿计数——压在全黑上会连续出现好几帧，不能每帧都算一次。 */
    {
        static uint32_t ff_cool_ms = 0;
        uint32_t        now_ms     = HAL_GetTick();

        if ((Line_OnCount() >= 7) &&
            ((now_ms - ff_cool_ms) > LAP_FF_COOL_MS))
        {
            ff_cool_ms = now_ms;
            ff_cnt++;

            /* 计时 + 特征双重校验：
             * 全黑事件只有落在"本圈预期时长的 92%~108%"窗口内才算一圈。
             * 方框、七边形那些拐角都在这之前，不会被误计。 */
            if ((LAP_TIME_MS > 0u) && (lap_done < lap_target))
            {
                uint32_t gate_lo = (uint32_t)(((uint32_t)LAP_TIME_MS * (uint32_t)(lap_done + 1u) * 95u) / 100u);
                uint32_t gate_hi = (uint32_t)(((uint32_t)LAP_TIME_MS * (uint32_t)(lap_done + 1u) * 107u) / 100u);

                if ((elapsed_ms >= gate_lo) && (elapsed_ms <= gate_hi))
                {
                    lap_done++;
                    if (lap_done >= lap_target)
                    {
                        Track_Stop();
                    }
                    else
                    {
                        Buzzer_Beep(150);       /* 4c：第一圈只响一声，不停车 */
                    }
                }
            }
        }
    }

    /* 兜底：跑到"目标圈数 × 一圈时长"的 115% 还没计到圈，强制停车 */
    if ((LAP_TIME_MS > 0u) &&
        (elapsed_ms >= (uint32_t)(((uint32_t)LAP_TIME_MS * (uint32_t)lap_target * 115u) / 100u)) &&
        (lap_done < lap_target))
    {
        Track_Stop();
    }
}

void Track_SetLapTarget(uint8_t n)
{
    if (n < 1u) n = 1u;
    if (n > 5u) n = 5u;
    lap_target = n;
}

uint8_t Track_GetLapTarget(void)
{
    return lap_target;
}

uint8_t Track_GetLapDone(void)
{
    return lap_done;
}

TrackState Track_GetState(void)
{
    return state;
}

uint32_t Track_GetTimeMs(void)
{
    if (state == TRACK_RUN)
    {
        return (uint32_t)(HAL_GetTick() - start_ms);
    }
    return elapsed_ms;
}

int16_t Track_GetError(void)
{
    return last_err;
}

int32_t Track_GetTurn(void)
{
    return turn_acc;
}

uint8_t Track_GetJuncIdx(void)
{
    return junc_idx;
}

uint8_t Track_GetLastMask(void)
{
    return last_on_mask;
}

uint8_t Track_GetEventMask(void)
{
    return event_mask;
}

uint8_t Track_GetHexState(void)
{
    return hex_act;      /* 和串口日志里的 R= 保持一致 */
}
