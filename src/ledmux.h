#ifndef LEDMUX_H_
#define LEDMUX_H_

#define LEDMUX_PB RCC_PB2Periph_GPIOC
#define LEDMUX_SRCLK GPIO_Pin_3  // LCLK = SRCLK
#define LEDMUX_SER   GPIO_Pin_6  // LDATA = SER
#define LEDMUX_RCLK  GPIO_Pin_7  // LSET = RCLK

#include <stdint.h>

typedef struct {
    uint8_t a;
    uint8_t b;
    int8_t step;
    uint8_t hold_steps;
} LEDMUX_anim_params_t;

void LEDMUX_init(void);
void LEDMUX_GPIO_set(uint16_t leds);
void LEDMUX_GPIO_animate(uint16_t leds, const LEDMUX_anim_params_t* params);
void LEDMUX_GPIO_clear(void);
// void LEDMUX_GPIOWalk(void);

#endif /* LEDMUX_H_ */