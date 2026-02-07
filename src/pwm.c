#include "pwm.h"

#include <debug.h>
#include <ch32v00X.h>
#include <ch32v00X_rcc.h>
#include <ch32v00X_misc.h>

static void tim1_ch3_pwm_init(void) {
    RCC_PB2PeriphClockCmd(RCC_PB2Periph_TIM1 | RCC_PB2Periph_GPIOC | RCC_PB2Periph_AFIO, ENABLE);
    GPIO_PinRemapConfig(GPIO_PartialRemap2_TIM1, ENABLE);

    GPIO_InitTypeDef g = {0};
    g.GPIO_Pin   = GPIO_Pin_0;
    g.GPIO_Mode  = GPIO_Mode_AF_PP;
    g.GPIO_Speed = GPIO_Speed_30MHz;
    GPIO_Init(GPIOC, &g);

    TIM_TimeBaseInitTypeDef tb = {0};
    tb.TIM_Prescaler     = 47;
    tb.TIM_Period        = 999;
    tb.TIM_CounterMode   = TIM_CounterMode_Up;
    tb.TIM_ClockDivision = TIM_CKD_DIV1;
    tb.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM1, &tb); // 48MHz/48/1000 = 1kHz

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

static inline void buzzer_off(void) { TIM1->CH3CVR = 0; }
static inline void buzzer_on_50(void) { uint16_t arr = TIM1->ATRLR; TIM1->CH3CVR = (arr + 1) >> 1; }

static void ring_tone(uint16_t period_us, int ms_total,
                      int on_ms, int off_ms_start, int off_ms_end) {
    int steps = ms_total / (on_ms + off_ms_start);
    if (steps < 1) 
        steps = 1;

    for (int i = 0; i < steps; ++i) {
        int off_ms = off_ms_start + (off_ms_end - off_ms_start) * i / (steps + 1);

        tim1_set_period_us(period_us);
        buzzer_on_50();
        Delay_Ms(on_ms);

        buzzer_off();
        Delay_Ms(off_ms);
    }
}

void ring_tone_pattern_demo(void) {
    const uint16_t P_G4 = 2550; // ~392 Hz
    const uint16_t P_E4 = 3030; // ~330 Hz
    const uint16_t P_C5 = 1910; // ~523 Hz

    buzzer_off();

    for (int i = 0; i < 4; ++i) {
        tim1_set_period_us(P_C5);
        buzzer_on_50();
        Delay_Ms(25);
        buzzer_off();
        Delay_Ms(15);

        tim1_set_period_us(P_G4);
        buzzer_on_50();
        Delay_Ms(30);
        buzzer_off();
        Delay_Ms(20);
    }

    ring_tone(P_E4, 120, /*on*/18, /*off start*/2, /*off end*/18);
    ring_tone(P_G4, 280, /*on*/16, /*off start*/3, /*off end*/20);
    ring_tone(P_C5, 120, /*on*/10, /*off start*/6, /*off end*/26);

    buzzer_off();
}

void PWM_init(void) {
    tim1_ch3_pwm_init();
    Delay_Init();
    buzzer_off();
}