#ifndef LEDMUX_H_
#define LEDMUX_H_

#define LEDMUX_PB RCC_PB2Periph_GPIOC
#define LEDMUX_SRCLK GPIO_Pin_3 // LCLK = SRCLK
#define LEDMUX_SER GPIO_Pin_6 // LDATA = SER
#define LEDMUX_RCLK GPIO_Pin_7 // LSET = RCLK

#define LEDMUX_LED_COUNT 12
#define LEDMUX_MAX_CONCURRENT_ANIMATIONS 32
#define LEDMUX_TICK_LED_FOCUS_US 1000

#define LEDMUX_ROTL(v, n) (((v) << (n)) | ((v) >> (LEDMUX_LED_COUNT - (n))))
#define LEDMUX_ROTR(v, n) (((v) >> (n)) | ((v) << (LEDMUX_LED_COUNT - (n))))

#include <stdint.h>

typedef struct {
    uint16_t a; // starting brightness (us) in range 0..LEDMUX_TICK_LED_FOCUS_US
    uint16_t b; // ending brightness (us) in range 0..LEDMUX_TICK_LED_FOCUS_US
    int16_t step; // step per frame (positive or negative)
    uint8_t flip; // repeat animation in reverse direction this number of times, 0 = no flip, 255 = infinite TODO
} LEDMUX_anim_params_t;

void LEDMUX_init(void);
void LEDMUX_set(uint16_t leds);
int LEDMUX_animate(uint16_t leds, const LEDMUX_anim_params_t* params);
void LEDMUX_step(void);
void LEDMUX_clear(void);

#endif /* LEDMUX_H_ */