#include "ledmux.h"

#include <ch32v00X.h>
#include <ch32v00X_rcc.h>
#include <ch32v00X_gpio.h>
#include <debug.h>

#define CONC_ANIM_EMPTY 0

static const uint8_t k_patterns[LEDMUX_LED_COUNT] = {
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

typedef struct {
    LEDMUX_anim_params_t params; // TODO: optimize "a" out
    uint16_t leds; // track the LEDs to animate, or CONC_ANIM_EMPTY if empty
    uint16_t value; // current brightness value in range (0..LEDMUX_TICK_LED_FOCUS_US)
} conc_anim_t;

static conc_anim_t animations[LEDMUX_MAX_CONCURRENT_ANIMATIONS];

// NOTE: LSB first
static void push8(uint8_t v) {
    for (int i = 0; i < 8; ++i) {
        (v & LEDMUX_ROTL(1u, i)) ? hi_ser() : lo_ser();

        hi_srclk();
        lo_srclk();
    }

    hi_rclk();
    lo_rclk();
}

#define LEDMUX_MIN_ON_US 2
#define LEDMUX_MIN_OFF_US 2
static inline void value_to_timing(uint32_t active_us, uint32_t* ton_us, uint32_t* toff_us) {
    if (!ton_us || !toff_us) 
        return;

    uint32_t slot_len_us = LEDMUX_TICK_LED_FOCUS_US;

    if (slot_len_us < (LEDMUX_MIN_ON_US + LEDMUX_MIN_OFF_US))
        slot_len_us = (LEDMUX_MIN_ON_US + LEDMUX_MIN_OFF_US);
    if (active_us > slot_len_us) 
        active_us = slot_len_us;

    uint32_t ton = active_us;
    uint32_t toff = slot_len_us - ton;

    if (ton < LEDMUX_MIN_ON_US) {
        ton = LEDMUX_MIN_ON_US;
        toff = (slot_len_us > ton) ? (slot_len_us - ton) : 0;
    }

    if (toff < LEDMUX_MIN_OFF_US) {
        ton = (slot_len_us > toff) ? (slot_len_us - toff) : 0;
        toff = LEDMUX_MIN_OFF_US;
    }

    if (ton < LEDMUX_MIN_ON_US) 
        ton = LEDMUX_MIN_ON_US;
    if (toff < LEDMUX_MIN_OFF_US) 
        toff = LEDMUX_MIN_OFF_US;

    *ton_us = ton;
    *toff_us = toff;
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

    for (int i = 0; i < LEDMUX_MAX_CONCURRENT_ANIMATIONS; ++i) {
        animations[i].leds = CONC_ANIM_EMPTY;
        animations[i].value = 0;
    }
}

void LEDMUX_set(uint16_t leds) {
    push8(leds);
}

int LEDMUX_animate(uint16_t leds, const LEDMUX_anim_params_t* params) {
    for (int i = 0; i < LEDMUX_MAX_CONCURRENT_ANIMATIONS; ++i) {
        conc_anim_t* anim = &animations[i];

        if (anim->leds == CONC_ANIM_EMPTY) {
            anim->leds = leds;
            anim->params = *params;
            anim->value = params->a;
            return 0;
        }
    }
    return -1;
}

void LEDMUX_step(void) {
    uint32_t active_us[LEDMUX_LED_COUNT] = {0};

    for (int i = 0; i < LEDMUX_MAX_CONCURRENT_ANIMATIONS; ++i) {
        conc_anim_t* anim = &animations[i];

        if (anim->leds == CONC_ANIM_EMPTY)
            continue;

        for (int i = 0; i < LEDMUX_LED_COUNT; ++i)
            if (anim->leds & LEDMUX_ROTL(1u, i))
                active_us[i] = (active_us[i] > anim->value) ? active_us[i] : anim->value;

        anim->value += anim->params.step;
        if ((anim->params.step > 0 && anim->value >= anim->params.b) || (anim->params.step < 0 && anim->value <= anim->params.b)) {
            if (anim->params.flip > 0) {
                anim->params.step = -anim->params.step;
                uint16_t s = anim->params.a;
                anim->params.a = anim->params.b;
                anim->params.b = s;
                if (anim->params.flip != 255)
                    --anim->params.flip;
            } else
                anim->leds = CONC_ANIM_EMPTY;
        }
    }

    for (int d = 0; d < LEDMUX_LED_COUNT; ++d) {
        uint32_t ton_us, toff_us;
    
        if (active_us[d] == 0) {
            Delay_Us(LEDMUX_TICK_LED_FOCUS_US);
            continue;
        }
    
        value_to_timing(active_us[d], &ton_us, &toff_us);
        push8(k_patterns[d]);
        Delay_Us(ton_us);
        push8(k_blank);
        Delay_Us(toff_us);
    }
    
}

void LEDMUX_clear(void) {
    push8(k_blank);
}
