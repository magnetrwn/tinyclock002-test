#include <ch32v00X.h>
#include <ch32v00X_rcc.h>
#include <ch32v00X_misc.h>
#include <ch32v00X_i2c.h>
#include <debug.h>

#include "ledmux.h"
#include "uart.h"
#include "ds1302.h"
#include "pm.h"
#include "anim.h"
#include "pwm.h"

int main(void) {
    SystemInit();
    RCC_SYSCLKConfig(RCC_SYSCLKSource_HSI);
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
    PM_sysclk_pll48();
    // __enable_irq();
    Delay_Init();
    UART_Init(230400);
    PM_standby_init(1000);
    PM_standby_enter(ON_STANDBY_EXIT_PLL48_SYSCLK);

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

    PWM_init();
    ANIM_setup();
    for (;;)
        ANIM_job();

    return 0;
}