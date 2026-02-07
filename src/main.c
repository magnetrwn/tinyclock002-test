#include <ch32v00X.h>
#include <ch32v00X_rcc.h>
#include <ch32v00X_misc.h>
#include <ch32v00X_i2c.h>
#include <debug.h>

#include "ledmux.h"
#include "uart.h"
#include "ds1302.h"
#include "pm.h"
#include "anim.h"

static void tim1_ch3_pwm_init_pc0_1mhz(void) {
    RCC_PB2PeriphClockCmd(RCC_PB2Periph_TIM1 | RCC_PB2Periph_GPIOC | RCC_PB2Periph_AFIO, ENABLE);
    GPIO_PinRemapConfig(GPIO_PartialRemap2_TIM1, ENABLE);

    GPIO_InitTypeDef g = {0};
    g.GPIO_Pin   = GPIO_Pin_0;
    g.GPIO_Mode  = GPIO_Mode_AF_PP;
    g.GPIO_Speed = GPIO_Speed_30MHz;
    GPIO_Init(GPIOC, &g);

    TIM_TimeBaseInitTypeDef tb = {0};
    tb.TIM_Prescaler     = 23;
    tb.TIM_Period        = 999;
    tb.TIM_CounterMode   = TIM_CounterMode_Up;
    tb.TIM_ClockDivision = TIM_CKD_DIV1;
    tb.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM1, &tb);

    TIM_OCInitTypeDef oc = {0};
    oc.TIM_OCMode      = TIM_OCMode_PWM1;
    oc.TIM_OutputState = TIM_OutputState_Enable;
    oc.TIM_Pulse       = (tb.TIM_Period + 1) >> 1;
    oc.TIM_OCPolarity  = TIM_OCPolarity_High;
    TIM_OC3Init(TIM1, &oc);
    TIM_OC3PreloadConfig(TIM1, TIM_OCPreload_Enable);

    TIM_ARRPreloadConfig(TIM1, ENABLE);
    TIM_CtrlPWMOutputs(TIM1, ENABLE);
    TIM_Cmd(TIM1, ENABLE);
}

static inline void tim1_set_period_us(uint16_t period_us) {
    if (period_us < 2) period_us = 2;
    uint16_t arr = (uint16_t)(period_us - 1);

    TIM1->ATRLR = arr;
    TIM1->CH3CVR = (arr + 1) >> 1;
}

int main(void) {
    SystemInit();
    RCC_SYSCLKConfig(RCC_SYSCLKSource_HSI);
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
    PM_sysclk_pll48();
    // __enable_irq();
    Delay_Init();

    UART_Init(230400);

    PM_standby_init(1000);
    PM_standby_enter(ON_STANDBY_EXIT_PLL48_SYSCLK);
    PM_standby_enter(ON_STANDBY_EXIT_PLL48_SYSCLK);
    Delay_Init();

    DS1302_init_basic();
    printf("SystemCoreClock: %d Hz.\r\n", (int) SystemCoreClock);
    printf("Device ID: 0x%08x.\r\n", (uint) DBGMCU_GetDEVID());
    printf("Setting default RTC time.\r\n");

    rtc_time_t t_set = {
        .sec   = 0,
        .min   = 0,
        .hour  = 8,
        .day   = 1,
        .month = 1,
        .year  = 2026,
        .dow   = 4,
    };
    DS1302_set_time(&t_set);

    ANIM_setup();
    tim1_ch3_pwm_init_pc0_1mhz();
    for (;;) {
        tim1_set_period_us(20000);
        ANIM_job();
    }

    return 0;
}