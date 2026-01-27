#include <ch32v00X.h>
#include <ch32v00X_rcc.h>
#include <ch32v00X_misc.h>
#include <ch32v00X_i2c.h>
#include <ch32v00X_pwr.h>
#include <ch32v00X_exti.h>
#include <debug.h>

#include "ledmux.h"
#include "uart.h"
#include "ds1302.h"

static void awu_exti9_event_init(void) {
    RCC_PB2PeriphClockCmd(RCC_PB2Periph_AFIO, ENABLE);

    EXTI_InitTypeDef ex = {0};
    ex.EXTI_Line    = EXTI_Line9; // AWU is wired internally to EXTI9
    ex.EXTI_Mode    = EXTI_Mode_Event; // NOTE: Event, not Interrupt
    ex.EXTI_Trigger = EXTI_Trigger_Falling;
    ex.EXTI_LineCmd = ENABLE;
    EXTI_Init(&ex);
}

static void standby_awu_init(void) {
    RCC_PB1PeriphClockCmd(RCC_PB1Periph_PWR, ENABLE);

    RCC_LSICmd(ENABLE);
    while (RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == RESET) {}

    PWR_AWU_SetPrescaler(PWR_AWU_Prescaler_4096);
    PWR_AWU_SetWindowValue(48); // ~128kHz/4096*48 = ~1.5s
    PWR_AutoWakeUpCmd(ENABLE);

    awu_exti9_event_init();
}

int main(void) {
    SystemInit();
    Delay_Init();
    RCC_SYSCLKConfig(RCC_SYSCLKSource_HSI);
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
    SystemCoreClockUpdate();

    __enable_irq();
    standby_awu_init();

    UART_Init(230400);
    Delay_Ms(1000);
    printf("SystemCoreClock: %d Hz.\r\n", (int) SystemCoreClock);
    printf("Device ID: 0x%08x.\r\n", (uint) DBGMCU_GetDEVID());

    DS1302_init_basic();
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

    printf("Looping LED mux animation.\r\n");
    for (;;) {
        PWR_EnterSTANDBYMode(PWR_STANDBYEntry_WFE);

        LEDMUX_GPIOWalk();

        rtc_time_t now;
        if (!DS1302_get_time(&now)) {
            printf("RTC time: %04d-%02d-%02d %02d:%02d:%02d (DOW=%d).\r\n",
                   now.year + 2000u, now.month, now.day,
                   now.hour, now.min, now.sec,
                   now.dow);
        } else
            printf("Failed to read RTC time.\r\n");
    }
}
