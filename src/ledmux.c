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

static const uint8_t k_full  = 0b01110000;
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

static void scan_frame_us(uint32_t ton_us, uint32_t toff_us) {
    for (int i = 0; i < 12; ++i) {
        push8(k_patterns[i]);
        Delay_Us(ton_us);
        push8(k_blank);
        Delay_Us(toff_us);
    }
}

#define DUTY_F_TICK_US 4
static void duty_frame_ticks(int a, int b, int step, int hold_frames) {
    for (int tick = a; (a < b) ? (tick <= b) : (tick >= b); tick += step) {

        int t_on_ticks = tick;
        int t_off_ticks = ((a > b) ? a : b) - t_on_ticks + 1;

        int t_on_us  = t_on_ticks * DUTY_F_TICK_US;
        int t_off_us = t_off_ticks * DUTY_F_TICK_US;

        for (int f = 0; f < hold_frames; ++f)
            scan_frame_us(t_on_us, t_off_us);
    }
}

void LEDMUX_GPIOWalk(void) {
    RCC_PB2PeriphClockCmd(RCC_PB2Periph_GPIOC, ENABLE);

    GPIO_InitTypeDef gpio = {0};
    gpio.GPIO_Speed = GPIO_Speed_30MHz;
    gpio.GPIO_Mode = GPIO_Mode_Out_PP;
    gpio.GPIO_Pin = LEDMUX_SRCLK | LEDMUX_SER | LEDMUX_RCLK;
    GPIO_Init(GPIOC, &gpio);

    lo_srclk(); 
    lo_rclk(); 
    lo_ser();

    push8(k_full);
    Delay_Ms(500);
    push8(k_blank);
    Delay_Ms(500);

    for (;;) {
        duty_frame_ticks(0, 100, 10, 5);
        push8(k_full);
        Delay_Ms(250);
        duty_frame_ticks(100, 0, -10, 5);
        Delay_Ms(250);
    }
}
