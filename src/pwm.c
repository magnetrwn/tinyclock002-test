#include "pwm.h"

#include <debug.h>
#include <ch32v00X.h>
#include <ch32v00X_rcc.h>
#include <ch32v00X_misc.h>
#include <ch32v00X_conf.h>

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
    // TIM_CtrlPWMOutputs(TIM1, ENABLE);
    TIM_Cmd(TIM1, ENABLE);
}

static void tim1_ch4_pwm_init(void) {
    RCC_PB2PeriphClockCmd(RCC_PB2Periph_GPIOC | RCC_PB2Periph_TIM1 | RCC_PB2Periph_AFIO, ENABLE);
    // GPIO_PinRemapConfig(GPIO_PartialRemap1_TIM1, ENABLE);

    // PC4 as AF push-pull
    GPIO_InitTypeDef g = {0};
    g.GPIO_Pin   = GPIO_Pin_4;
    g.GPIO_Mode  = GPIO_Mode_AF_PP;
    g.GPIO_Speed = GPIO_Speed_30MHz;
    GPIO_Init(GPIOC, &g);

    TIM_TimeBaseInitTypeDef tb = {0};
    tb.TIM_Prescaler = 47;   // 48MHz/48 = 1MHz tick
    tb.TIM_Period    = 999;  // 1kHz
    tb.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM1, &tb);

    TIM_OCInitTypeDef oc = {0};
    oc.TIM_OCMode      = TIM_OCMode_PWM1;
    oc.TIM_OutputState = TIM_OutputState_Enable;
    oc.TIM_Pulse       = (tb.TIM_Period + 1) >> 1; // 50%
    oc.TIM_OCPolarity  = TIM_OCPolarity_High;
    TIM_OC4Init(TIM1, &oc);
    TIM_OC4PreloadConfig(TIM1, TIM_OCPreload_Enable);

    TIM_ARRPreloadConfig(TIM1, ENABLE);
    // TIM_CtrlPWMOutputs(TIM1, ENABLE);  // MOE required on TIM1
    TIM_Cmd(TIM1, ENABLE);
}

static inline void tim1_set_period_us(uint16_t period_us) {
    if (period_us < 2) period_us = 2;
    uint16_t arr = (uint16_t)(period_us - 1);
    TIM1->ATRLR = arr;
    TIM1->CH4CVR = (arr + 1) >> 1;
}

static inline void buzzer_enable(void) { TIM_CtrlPWMOutputs(TIM1, ENABLE); }
static inline void buzzer_disable(void) { TIM_CtrlPWMOutputs(TIM1, DISABLE); }
static inline void buzzer_off(void) { TIM1->CH4CVR = 0; }
static inline void buzzer_on_50(void) { TIM1->CH4CVR = (TIM1->ATRLR + 1) >> 1; }

void ring_tone_pattern_demo(void) {
    const uint16_t P_G4 = 2550; // ~392 Hz
    const uint16_t P_C5 = 1910; // ~523 Hz
    const uint16_t P_E5 = 1497; // ~668 Hz
    const uint16_t P_G5 = 1275; // ~784 Hz
    const uint16_t tempo = 146;

    buzzer_enable();

    // tim1_set_period_us(P_E5);
    // Delay_Ms(10);

    tim1_set_period_us(P_E5);
    Delay_Ms(tempo);
    tim1_set_period_us(P_E5);
    Delay_Ms(tempo);
    buzzer_off();
    Delay_Ms(tempo);
    tim1_set_period_us(P_E5);
    Delay_Ms(tempo);
    buzzer_off();
    Delay_Ms(tempo);
    tim1_set_period_us(P_C5);
    Delay_Ms(tempo);
    tim1_set_period_us(P_E5);
    Delay_Ms(tempo);
    buzzer_off();
    Delay_Ms(tempo);
    tim1_set_period_us(P_G5);
    Delay_Ms(2*tempo);
    buzzer_off();
    Delay_Ms(2*tempo);
    tim1_set_period_us(P_G4);
    Delay_Ms(2*tempo);
    buzzer_off();
    Delay_Ms(2*tempo);

    RCC_PB2PeriphClockCmd(RCC_PB2Periph_GPIOC, ENABLE);

    // GPIO_InitTypeDef gpio = {0};
    // gpio.GPIO_Speed = GPIO_Speed_30MHz;
    // gpio.GPIO_Mode = GPIO_Mode_Out_PP;
    // gpio.GPIO_Pin = GPIO_Pin_4;
    // GPIO_Init(GPIOC, &gpio);

    // for (;;) {
    //     GPIO_SetBits(GPIOC, GPIO_Pin_4);
    //     Delay_Ms(3);
    //     GPIO_ResetBits(GPIOC, GPIO_Pin_4);
    //     Delay_Ms(3);
    // }

    buzzer_off();
    buzzer_disable();
}

void PWM_init(void) {
    tim1_ch4_pwm_init();
    Delay_Init();
    buzzer_enable();
    buzzer_off();
    buzzer_disable();
}