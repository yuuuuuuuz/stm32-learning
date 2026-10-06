/**
 ******************************************************************************
 * @file    main.c
 * @brief   STM32F103 舵机 PWM 控制 —— 上电回到 0 度并保持
 *
 * 硬件接线（STM32F103C8T6 最小系统板）
 * ---------------------------------------------------------------------------
 *   舵机 橙/黄/白 (信号)  ->  PA0        (TIM2_CH1)
 *   舵机 红      (电源+)  ->  独立 5V 电源正极   ★ 不要接板子的 3.3V
 *   舵机 棕/黑   (电源-)  ->  独立 5V 电源负极，并且必须和板子 GND 共地
 *   板载 LED              ->  PC13 (低电平点亮，不用接线)
 *
 * 原理
 * ---------------------------------------------------------------------------
 *   TIM2 时钟 = 72MHz，预分频 PSC=71  ->  计数频率 1MHz  ->  1 个计数 = 1us
 *   自动重装 ARR = 19999               ->  周期 20000us = 20ms  ->  50Hz
 *   所以比较寄存器 CCR 的值直接就是脉宽微秒数。
 *
 * ⚠️⚠️  动手前请先确认你的舵机是哪种  ⚠️⚠️
 * ---------------------------------------------------------------------------
 * 【位置型舵机】SG90 / MG996R / 各种 20kg 数字舵机
 *       脉宽代表【角度】，用手转输出轴能感觉到约 180~270 度的机械限位。
 *           1000us -> 0 度
 *           1500us -> 中位 (90 度)
 *           2000us -> 180 度
 *
 * 【连续旋转型舵机】也叫 360 度舵机
 *       脉宽代表【速度】，输出轴没有任何限位，用手可以无限圈地转下去。
 *           1000us -> 全速正转（永不停）
 *           1500us -> 停止          <- 这才是它的"0"
 *           2000us -> 全速反转（永不停）
 *       ★ 这种舵机根本不存在"0 度"这个角度。
 *
 *   判断方法（5 秒）：断电，用手捏住舵盘转。能无限圈地转 = 连续旋转型；
 *   转到某个角度就卡住、转不过去 = 位置型。
 *
 *   如果你的是连续旋转型，把下面 SERVO_IS_CONTINUOUS 改成 1，
 *   程序会变成"上电即输出 1500us 让舵机停住"。
 ******************************************************************************
 */

#include "stm32f10x.h"

/* ============================== 配置区 ============================== */

/* 0 = 位置型舵机（按角度控制）
   1 = 连续旋转型舵机（按速度控制，上电只发 1500us 让它停住） */
#define SERVO_IS_CONTINUOUS     0

/* 1 = 回到 0 度之后，再做一次 0° -> 180° -> 0° 的往返演示
   0 = 回到 0 度后一直保持不动（你现在的目标就是这个） */
#define DEMO_SWEEP              0

/* 舵机信号引脚：PA0 = TIM2_CH1 */
#define SERVO_PORT              GPIOA
#define SERVO_PIN               GPIO_Pin_0
#define SERVO_GPIO_CLK          RCC_APB2Periph_GPIOA
#define SERVO_TIM               TIM2
#define SERVO_TIM_CLK           RCC_APB1Periph_TIM2

/* 板载状态灯：PC13 */
#define LED_PORT                GPIOC
#define LED_PIN                 GPIO_Pin_13
#define LED_GPIO_CLK            RCC_APB2Periph_GPIOC

/* 脉宽定义（单位微秒）
   不同舵机的实际行程略有差异：如果舵机顶到机械限位后持续嗡嗡响、
   发热，就把 0 度 / 180 度这两个值各往中间收 50~100us。 */
#define PULSE_0DEG              1000u
#define PULSE_CENTER            1500u
#define PULSE_180DEG            2000u

/* 硬性安全范围：绝不输出这个范围之外的脉宽 */
#define PULSE_LIMIT_MIN         500u
#define PULSE_LIMIT_MAX         2500u

/* ==================================================================== */

/**
 * @brief  用 SysTick 做精确毫秒延时（不占用中断，避免和 Keil 工程里的
 *         SysTick_Handler 冲突）
 */
static void Delay_ms(uint32_t ms)
{
    while (ms--)
    {
        SysTick->LOAD = (SystemCoreClock / 1000u) - 1u;
        SysTick->VAL  = 0u;
        SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk;
        while ((SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk) == 0u)
        {
            /* 等待计数器归零 */
        }
    }
    SysTick->CTRL = 0u;
}

/**
 * @brief  设置舵机脉宽
 * @param  pulse_us  脉宽，单位微秒。超出 500~2500 会被截断保护。
 */
void Servo_SetPulse(uint16_t pulse_us)
{
    if (pulse_us < PULSE_LIMIT_MIN) pulse_us = PULSE_LIMIT_MIN;
    if (pulse_us > PULSE_LIMIT_MAX) pulse_us = PULSE_LIMIT_MAX;

    /* 因为计数频率是 1MHz，CCR 的值就等于微秒数 */
    TIM_SetCompare1(SERVO_TIM, pulse_us);
}

/**
 * @brief  按角度设置舵机位置（仅对位置型舵机有意义）
 * @param  deg  0 ~ 180
 */
void Servo_SetAngle(uint16_t deg)
{
    uint32_t span;

    if (deg > 180u) deg = 180u;

    span = (uint32_t)(PULSE_180DEG - PULSE_0DEG);
    Servo_SetPulse((uint16_t)(PULSE_0DEG + (span * deg) / 180u));
}

/**
 * @brief  初始化舵机 PWM 和状态灯
 */
static void Servo_Init(void)
{
    GPIO_InitTypeDef        gpio;
    TIM_TimeBaseInitTypeDef tb;
    TIM_OCInitTypeDef       oc;

    /* 1. 开时钟：GPIOA/GPIOA 在 APB2，TIM2 在 APB1 */
    RCC_APB2PeriphClockCmd(SERVO_GPIO_CLK | LED_GPIO_CLK, ENABLE);
    RCC_APB1PeriphClockCmd(SERVO_TIM_CLK, ENABLE);

    /* 2. PA0 配置成复用推挽输出 —— 配错成普通推挽就没有 PWM 输出 */
    gpio.GPIO_Pin   = SERVO_PIN;
    gpio.GPIO_Mode  = GPIO_Mode_AF_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(SERVO_PORT, &gpio);

    /* 3. PC13 状态灯，推挽输出，默认熄灭（高电平灭） */
    gpio.GPIO_Pin   = LED_PIN;
    gpio.GPIO_Mode  = GPIO_Mode_Out_PP;
    gpio.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(LED_PORT, &gpio);
    GPIO_SetBits(LED_PORT, LED_PIN);

    /* 4. TIM2 时基：72MHz / 72 = 1MHz，ARR=19999 -> 20ms -> 50Hz */
    tb.TIM_Prescaler     = 71;
    tb.TIM_Period        = 19999;
    tb.TIM_ClockDivision = TIM_CKD_DIV1;
    tb.TIM_CounterMode   = TIM_CounterMode_Up;
    TIM_TimeBaseInit(SERVO_TIM, &tb);

    /* 5. CH1 配置为 PWM1 模式
          先给中位 1500us 再启动，这样一上电输出就是确定的，不会乱抖。 */
    oc.TIM_OCMode      = TIM_OCMode_PWM1;
    oc.TIM_OutputState = TIM_OutputState_Enable;
    oc.TIM_OCPolarity  = TIM_OCPolarity_High;
    oc.TIM_Pulse       = PULSE_CENTER;
    TIM_OC1Init(SERVO_TIM, &oc);
    TIM_OC1PreloadConfig(SERVO_TIM, TIM_OCPreload_Enable);

    /* 6. 使能 ARR 预装载并启动定时器 */
    TIM_ARRPreloadConfig(SERVO_TIM, ENABLE);
    TIM_Cmd(SERVO_TIM, ENABLE);
}

/**
 * @brief  状态灯心跳：亮 50ms，灭 950ms
 */
static void Led_Heartbeat(void)
{
    GPIO_ResetBits(LED_PORT, LED_PIN);   /* 低电平点亮 */
    Delay_ms(50);
    GPIO_SetBits(LED_PORT, LED_PIN);     /* 熄灭 */
    Delay_ms(950);
}

int main(void)
{
    Servo_Init();

#if SERVO_IS_CONTINUOUS
    /* ------------------------------------------------------------------
     * 连续旋转型舵机：不存在"0 度"，唯一能做的就是让它停住。
     * 1500us = 停止。
     * ---------------------------------------------------------------- */
    Servo_SetPulse(PULSE_CENTER);

    while (1)
    {
        Led_Heartbeat();
    }
#else
    /* ------------------------------------------------------------------
     * 位置型舵机：回到 0 度并保持
     * ---------------------------------------------------------------- */

    /* 第 1 步：先到中位。这一步有两个作用：
     *   (a) 从任意位置平稳落位，不会猛地一甩；
     *   (b) 验证舵机是活的 —— 只要它在动，就说明电位器反馈正常。 */
    Servo_SetPulse(PULSE_CENTER);
    Delay_ms(1500);

    /* 第 2 步：回到 0 度 */
    Servo_SetPulse(PULSE_0DEG);
    Delay_ms(1500);

#if DEMO_SWEEP
    /* 可选：慢速往返一次，用来直观确认行程范围 */
    {
        uint16_t us;
        for (us = PULSE_0DEG; us <= PULSE_180DEG; us += 10u)
        {
            Servo_SetPulse(us);
            Delay_ms(20);
        }
        for (us = PULSE_180DEG; us >= PULSE_0DEG; us -= 10u)
        {
            Servo_SetPulse(us);
            Delay_ms(20);
            if (us < PULSE_0DEG + 10u) break;   /* 防止 uint16_t 下溢死循环 */
        }
        Servo_SetPulse(PULSE_0DEG);
    }
#endif

    /* 第 3 步：保持不动，LED 心跳表示程序还在跑 */
    while (1)
    {
        Led_Heartbeat();
    }
#endif
}
