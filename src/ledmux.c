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

static void scan_frame_us(uint32_t ton_us, uint32_t toff_us) {
    for (int i = 0; i < 12; ++i) {
        push8(k_patterns[i]);
        Delay_Us(ton_us);
        push8(k_blank);
        Delay_Us(toff_us);
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

    push8(0b01110000);
    Delay_Ms(500);
    push8(k_blank);
    Delay_Ms(250);

    const uint32_t Tslot_us = 200;
    const uint32_t hold_frames = 10;

    for (;;) {
        for (uint32_t duty = 5; duty <= 95; duty += 5) {
            uint32_t ton  = (Tslot_us * duty) / 100u;
            uint32_t toff = Tslot_us - ton;
            if (ton  < 2) ton  = 2;
            if (toff < 2) toff = 2;

            for (uint32_t f = 0; f < hold_frames; ++f)
                scan_frame_us(ton, toff);
        }

        for (int duty = 95; duty >= 5; duty -= 5) {
            uint32_t ton  = (Tslot_us * (uint32_t)duty) / 100u;
            uint32_t toff = Tslot_us - ton;
            if (ton  < 2) ton  = 2;
            if (toff < 2) toff = 2;

            for (uint32_t f = 0; f < hold_frames; ++f)
                scan_frame_us(ton, toff);
        }
    }
}
