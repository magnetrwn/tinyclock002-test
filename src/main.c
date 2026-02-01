#include <ch32v00X.h>
#include <ch32v00X_rcc.h>
#include <ch32v00X_misc.h>
#include <ch32v00X_i2c.h>
#include <debug.h>

#include "ledmux.h"
#include "uart.h"
#include "ds1302.h"
#include "pm.h"

typedef enum {
    BR_UP = 0,
    BR_HOLD_MAX,
    BR_DOWN,
    BR_SLEEP_NEXT,
} br_state_t;

int main(void) {
    SystemInit();
    Delay_Init();
    RCC_SYSCLKConfig(RCC_SYSCLKSource_HSI);
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
    SystemCoreClockUpdate();
    UART_Init(230400);

    __enable_irq();
    PM_standby_init(1500);
    PM_standby_enter();
    PM_standby_enter();

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

    LEDMUX_init();
    printf("Looping LED mux animation.\r\n");
    uint16_t rot = 0x001;
    uint8_t is_cw = 0;

    const uint16_t anim_b_step_us = 12;
    const LEDMUX_anim_params_t ANIM_MIN = { .a = 0, .b = anim_b_step_us, .step = 1,  .flip = 1 };
    const LEDMUX_anim_params_t ANIM_MAX = { .a = 0, .b = LEDMUX_TICK_LED_FOCUS_US, .step = 40, .flip = 1 };
    
    LEDMUX_anim_params_t anim = ANIM_MIN;

    br_state_t br_state = BR_UP;
    uint8_t br_step = 1;
    uint8_t hold_max_updates = 64;

    anim.step = (int8_t) br_step;
    anim.b    = (uint16_t) br_step * anim_b_step_us;

    for (int i = 0;; ++i) {

        if (br_state == BR_SLEEP_NEXT) {
            while (LEDMUX_count_anim()) {
                LEDMUX_step();
            }
            PM_standby_enter();

            is_cw = !is_cw;
            br_state = BR_UP;
            br_step = 1;
            hold_max_updates = 64;
            anim.step = (int8_t) br_step;
            anim.b    = (uint16_t) br_step * anim_b_step_us;
        }

        if (!(i & 0x3)) {
            switch (br_state) {
                case BR_UP:
                    if (br_step < (uint8_t) ANIM_MAX.step) {
                        ++br_step;
                        anim.step = (int8_t) br_step;
                        anim.b    = (uint16_t) br_step * anim_b_step_us;
                    } else {
                        br_state = BR_HOLD_MAX;
                        hold_max_updates = 64;
                    }
                    break;

                case BR_HOLD_MAX:
                    if (hold_max_updates) {
                        --hold_max_updates;
                    } else {
                        br_state = BR_DOWN;
                    }
                    break;

                case BR_DOWN:
                    if (br_step > 1) {
                        --br_step;
                        anim.step = (int8_t) br_step;
                        anim.b    = (uint16_t) br_step * anim_b_step_us;
                    } else {
                        anim.step = (int8_t) br_step;
                        anim.b    = (uint16_t) br_step * anim_b_step_us;
                        br_state = BR_SLEEP_NEXT;
                    }
                    break;

                default:
                    break;
            }
        }

        if (!(i & 0x3)) {
            if (is_cw) rot = LEDMUX_ROTR(rot, 1);
            else       rot = LEDMUX_ROTL(rot, 1);

            LEDMUX_animate(rot, &anim);
        }

        if (!(i & 0xff)) {
            rtc_time_t now;
            if (DS1302_get_time(&now) == 0) {
                printf("RTC time: %04d-%02d-%02d %02d:%02d:%02d (DOW=%d).\r\n",
                    now.year + 2000u, now.month, now.day,
                    now.hour, now.min, now.sec, now.dow);
            } else {
                printf("Failed to read RTC time.\r\n");
            }
        }

        LEDMUX_step();
    }

    return 0;
}