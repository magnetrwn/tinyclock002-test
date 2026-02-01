#include <ch32v00X.h>
#include <ch32v00X_rcc.h>
#include <ch32v00X_misc.h>
#include <ch32v00X_i2c.h>
#include <debug.h>

#include "ledmux.h"
#include "uart.h"
#include "ds1302.h"
#include "pm.h"

int main(void) {
    SystemInit();
    Delay_Init();
    RCC_SYSCLKConfig(RCC_SYSCLKSource_HSI);
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
    SystemCoreClockUpdate();
    UART_Init(230400);

    __enable_irq();
    PM_standby_init(500);
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
    LEDMUX_anim_params_t anim = { .a = 0, .b = LEDMUX_TICK_LED_FOCUS_US, .step = 32, .flip = 1 };

    int i = 0;
    uint16_t rot = 0b000000000001;
    int is_cw = 0;
    for (;; ++i) {
        if (!(i & 0x7)) {
            if (is_cw)
                rot = LEDMUX_ROTR(rot, 1);
            else
                rot = LEDMUX_ROTL(rot, 1);
            LEDMUX_animate(rot, &anim);
        }

        if (!(i & 0xff)) {
            is_cw = !is_cw;

            rtc_time_t now;
            if (DS1302_get_time(&now) == 0) {
                printf("RTC time: %04d-%02d-%02d %02d:%02d:%02d (DOW=%d).\r\n",
                        now.year + 2000u, now.month, now.day,
                        now.hour, now.min, now.sec,
                        now.dow);
            } else
                printf("Failed to read RTC time.\r\n");
        }

        LEDMUX_step();
    }

    return 0;
}