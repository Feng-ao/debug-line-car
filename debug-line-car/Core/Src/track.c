#include "track.h"
#include "line.h"
#include "motor.h"
#include "buzzer.h"

/* 赛题 4b：巡线一圈
 * 控制律：偏差 PD -> 差速转向
 * 计圈：左右轮速差对时间积分，绝对值超过 LAP_UNIT 判定跑满一圈
 */

#define BASE_SPEED        500    /* 直道速度，0~1000 */
#define KP_NUM             90    /* 比例：偏差 * 0.90 */
#define KD_NUM            140    /* 微分：偏差变化 * 1.40 */
#define TURN_LIMIT        900    /* 转向量上限，允许内侧轮反转，急弯才能咬住 */
#define CURVE_SLOW_DIV      6    /* 偏差每 6 点，整体速度降 1 点（弯里自动减速） */
#define SPEED_MIN         260    /* 转向时最低速度 */
#define LAP_UNIT     5000000    /* 一圈的转向积分阈值：待赛道标定！
                                 * 标定方法：跑完一圈按 K4 停，把屏上 L= 的值填进来。
                                 * 暂时给一个很大的值，避免没跑完就自动判圈停车。 */
#define TRACK_INVERT_TURN   0    /* 发现越跑越偏就改成 1 */
#define LOST_STOP_TICKS   100    /* 连续 1 秒看不到线就停车 */

/* 断路处理：线突然"整片消失"且消失前车是正的（偏差小）→ 直行穿过去 */
#define GAP_CONFIRM_TICKS   2    /* 连续 2 帧全白才确认（防抖） */
#define GAP_SPEED         380    /* 断路里直行的速度 */
#define GAP_TIMEOUT_MS   1500    /* 断路里最多直行 1.5 秒，超时说明冲出去了 */
#define GAP_RECOVER_MS    250    /* 出断路后的抑制时间 */
#define GAP_RECOVER_TURN  350    /* 抑制期内最大转向量，防止接上线瞬间画圈 */
#define GAP_ERR_MAX       250    /* 断路判定：丢线前的偏差必须很小 */
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
 * 六边形顶点是个 Y 形岔口：主线继续直行会跳过整个六边形，必须主动左拐 30°。
 * 判据只在"车已经直行一段时间"时才生效——拐角处车在转向或丢线，
 * 直行计数会清零，所以不会误触发（这是上次翻车的教训）。 */
#define HEX_ENABLE           1
#define HEX_STRAIGHT_CNT    15   /* 连续多少帧直行才允许触发（×10ms） */
#define HEX_CONFIRM          2   /* 连续几帧看到左分支才确认 */
#define HEX_ENTER_MS       220   /* 进环时"往左带"的持续时间 */
#define HEX_EXIT_MS        220   /* 出环时"往左带"的持续时间 */
#define HEX_MIN_IN_MS      700   /* 进环后至少待这么久才开始判出环 */
#define HEX_LEFT_SPD       200   /* 进/出环时的左轮速度 */
#define HEX_RIGHT_SPD      700   /* 进/出环时的右轮速度 */
#define HEX_LEFT_MASK     0x03u  /* X1/X2 */
#define HEX_CENTER_MASK   0x18u  /* X4/X5 */

/* 出弯稳定区：拐角之间距离很短（280~400mm），出弯后必须减速，
 * 让 PD 有足够多的控制周期把车身摆正，否则到下一个弯还是歪的 */
#define CORNER_ZONE_MS     700   /* 出弯后这段时间内限速 */
#define CORNER_ZONE_SPEED  330   /* 限速值 */
#define CORNER_FAST_KP     130   /* 出弯后短时间内的强纠偏 KP */
#define CORNER_FAST_KP_MS  280

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

/* 断路状态 */
static uint8_t    in_gap;        /* 1 = 正在断路里直行 */
static uint8_t    gap_cnt;       /* 连续全白帧数 */
static uint32_t   gap_t0;
static uint8_t    last_on_mask;  /* 最后一次压线时的传感器位图 */
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
static uint32_t   corner_t0;
static uint32_t   corner_phase_t;
static int8_t     corner_dir;
static uint8_t    event_mask;    /* 触发拐角/路口那一刻的位图，停车后可以看 */
static uint32_t   corner_end_ms; /* 拐角结束时刻，用于过弯补偿 */
static int8_t     corner_bias_dir;

/* 六边形进环状态 */
static uint8_t    in_hex;        /* 1 = 已在六边形里 */
static uint8_t    hex_cnt;       /* 左分支确认计数 */
static uint8_t    straight_cnt;  /* 连续直行帧数 */
static uint8_t    hex_act;       /* 0=无 1=进环拐 2=出环拐 */
static uint32_t   hex_t0;
static uint32_t   hex_in_ms;     /* 进环时刻 */

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
    in_gap     = 0;
    gap_cnt    = 0;
    last_on_mask = 0;
    gap_exit_ms  = 0;
    in_junc      = 0;
    junc_cnt     = 0;
    junc_idx     = 0;
    junc_cool_ms = 0;
    in_corner      = 0;
    corner_phase   = 0;
    corner_t0      = 0;
    corner_phase_t = 0;
    corner_dir     = 0;
    corner_end_ms  = 0;
    corner_bias_dir = 0;
    in_hex         = 0;
    hex_cnt        = 0;
    straight_cnt   = 0;
    hex_act        = 0;
    hex_t0         = 0;
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
            last_on_mask = on;
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
                (last_err <= GAP_ERR_MAX) &&
                (last_err >= -GAP_ERR_MAX) &&
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
    {
        int16_t e = Line_LastError();
        uint8_t n = Line_OnCount();

        if ((n >= 1) && (n <= CORNER_MAX_DOTS) && (e <= 150) && (e >= -150))
        {
            if (straight_cnt < 255) straight_cnt++;
        }
        else
        {
            straight_cnt = 0;
        }
    }

    if (hex_act != 0)                       /* 正在往左带 */
    {
        Motor_Set(HEX_LEFT_SPD, HEX_RIGHT_SPD);

        if ((HAL_GetTick() - hex_t0) >=
            ((hex_act == 1) ? HEX_ENTER_MS : HEX_EXIT_MS))
        {
            hex_act  = 0;
            last_err = 0;
        }
        return;
    }

    if (!in_hex && (straight_cnt >= HEX_STRAIGHT_CNT))
    {
        uint8_t on = Line_OnBits();

        if (((on & HEX_LEFT_MASK) != 0) && ((on & HEX_CENTER_MASK) != 0))
        {
            if (++hex_cnt >= HEX_CONFIRM)
            {
                hex_cnt      = 0;
                in_hex       = 1;
                hex_act      = 1;
                hex_t0       = HAL_GetTick();
                hex_in_ms    = hex_t0;
                straight_cnt = 0;
            }
        }
        else
        {
            hex_cnt = 0;
        }
    }
    else if (in_hex && ((HAL_GetTick() - hex_in_ms) > HEX_MIN_IN_MS))
    {
        uint8_t on = Line_OnBits();

        /* 绕回顶点：线突然变宽，或又出现左分支 */
        if ((Line_OnCount() >= 5) ||
            (((on & HEX_LEFT_MASK) != 0) && ((on & HEX_CENTER_MASK) != 0)))
        {
            if (++hex_cnt >= HEX_CONFIRM)
            {
                hex_cnt = 0;
                in_hex  = 0;
                hex_act = 2;
                hex_t0  = HAL_GetTick();
            }
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

                    corner_phase = 1;
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
    else if (Line_OnCount() == 0)
    {
        in_corner      = 1;
        corner_phase   = 0;
        corner_dir     = Track_LastDir();
        corner_t0      = HAL_GetTick();
        corner_phase_t = corner_t0;
        event_mask     = last_on_mask;

        Motor_Stop();                           /* 先刹住 */
        return;
    }

    err   = Line_Error();
    delta = (int16_t)(err - last_err);
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

        /* 出弯稳定区：刚拐完的 700ms 内限速，先把姿态摆正再提速 */
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

    /* 每满一圈判定一次：最后一圈停车，前面几圈只响一声继续跑（赛题 4c） */
    {
        int32_t next_lap = (int32_t)LAP_UNIT * (int32_t)(lap_done + 1u);

        if ((turn_acc >= next_lap) || (turn_acc <= -next_lap))
        {
            lap_done++;
            if (lap_done >= lap_target)
            {
                Track_Stop();
            }
            else
            {
                Buzzer_Beep(150);
            }
        }
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
    return in_hex;
}
