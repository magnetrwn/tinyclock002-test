#include "ledmux.h"

#include <ch32v00X.h>
#include <ch32v00X_rcc.h>
#include <ch32v00X_gpio.h>
#include <debug.h>

static const uint8_t k_patterns[12] = {
    0b01000111, 0b01001011, 0b01001101, 0b01001110,
    0b00100111, 0b00101011, 0b00101101, 0b00101110,
    0b00010111, 0b00011011, 0b00011101, 0b00011110,
};

static const uint8_t k_blank = 0b00001111;

static inline void hi_srclk(void) { GPIO_SetBits(GPIOC, LEDMUX_SRCLK); }
static inline void lo_srclk(void) { GPIO_ResetBits(GPIOC, LEDMUX_SRCLK); }
static inline void hi_rclk(void) { GPIO_SetBits(GPIOC, LEDMUX_RCLK); }
static inline void lo_rclk(void) { GPIO_ResetBits(GPIOC, LEDMUX_RCLK); }
static inline void hi_ser(void) { GPIO_SetBits(GPIOC, LEDMUX_SER); }
static inline void lo_ser(void) { GPIO_ResetBits(GPIOC, LEDMUX_SER); }

// NOTE: LSB first
static void push8(uint8_t v) {
    for (int i = 0; i < 8; ++i) {
        (v & (1u << i)) ? hi_ser() : lo_ser();

        hi_srclk();
        lo_srclk();
    }

    hi_rclk();
    lo_rclk();
}

static void push8_by_led(uint16_t leds, uint32_t ton_us, uint32_t toff_us) {
    for (int i = 0; i < 12; ++i) {
        if (!(leds & (1u << i))) {
            Delay_Us(ton_us + toff_us);
            continue;
        }

        push8(k_patterns[i]);
        Delay_Us(ton_us);
        push8(k_blank);
        Delay_Us(toff_us);
    }
}

static void push8_animate(uint16_t leds, int a, int b, int step, int hold_steps) {
    for (int tick = a; (a < b) ? (tick <= b) : (tick >= b); tick += step) {

        int t_on_ticks = tick;
        int t_off_ticks = ((a > b) ? a : b) - t_on_ticks;

        int t_on_us = t_on_ticks * 4;
        int t_off_us = t_off_ticks * 4;

        for (int f = 0; f < hold_steps; ++f)
            push8_by_led(leds, t_on_us, t_off_us);
    }
}

void LEDMUX_init(void) {
    RCC_PB2PeriphClockCmd(LEDMUX_PB, ENABLE);

    GPIO_InitTypeDef gpio = {0};
    gpio.GPIO_Speed = GPIO_Speed_30MHz;
    gpio.GPIO_Mode = GPIO_Mode_Out_PP;
    gpio.GPIO_Pin = LEDMUX_SRCLK | LEDMUX_SER | LEDMUX_RCLK;
    GPIO_Init(GPIOC, &gpio);

    lo_srclk(); 
    lo_rclk(); 
    lo_ser();
}

void LEDMUX_GPIO_set(uint16_t leds) {
    push8(leds);
}

void LEDMUX_GPIO_animate(uint16_t leds, const LEDMUX_anim_params_t* params) {
    push8_animate(leds, params->a, params->b, params->step, params->hold_steps);
}

void LEDMUX_GPIO_clear(void) {
    push8(k_blank);
}
