#include "pm.h"

#include <ch32v00X_pwr.h>
#include <ch32v00X_exti.h>
#include <stdint.h>

// NOTE: max 30s
void PM_standby_init(uint32_t wakeup_ms) {
    if (wakeup_ms == 0 || wakeup_ms >= 30240)
        return;

    RCC_PB1PeriphClockCmd(RCC_PB1Periph_PWR, ENABLE);
    RCC_LSICmd(ENABLE);
    while (RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == RESET);

    if (wakeup_ms < 252) {
        // NOTE: ~128000 Hz/512 = 250 Hz = 4 ms, so >> 2 for exact granularity, max 0x3F*4 = 252 ms
        PWR_AWU_SetPrescaler(PWR_AWU_Prescaler_512);
        PWR_AWU_SetWindowValue((uint32_t) (wakeup_ms >> 2)); 
    } else if (wakeup_ms < 2016) {
        // NOTE: ~128000 Hz/4096 = 31.25 Hz = 32 ms, so >> 5 for exact granularity, max 0x3F*32 = 2016 ms
        PWR_AWU_SetPrescaler(PWR_AWU_Prescaler_4096); 
        PWR_AWU_SetWindowValue((uint32_t) (wakeup_ms >> 5)); 
    } else if (wakeup_ms < 30240) {
        // NOTE: ~128000 Hz/61440 = 2.0833 Hz = 480 ms, so >> 9 for rough granularity, max 0x3F*480 = 30240 ms
        PWR_AWU_SetPrescaler(PWR_AWU_Prescaler_61440); 
        PWR_AWU_SetWindowValue((uint32_t) (wakeup_ms >> 9)); 
    }
    
    PWR_AutoWakeUpCmd(ENABLE);

    RCC_PB2PeriphClockCmd(RCC_PB2Periph_AFIO, ENABLE);
    EXTI_InitTypeDef ex = {0};
    ex.EXTI_Line    = EXTI_Line9;
    ex.EXTI_Mode    = EXTI_Mode_Event;
    ex.EXTI_Trigger = EXTI_Trigger_Falling;
    ex.EXTI_LineCmd = ENABLE;
    EXTI_Init(&ex);
}

void PM_standby_enter(PM_on_standby_exit_e e) {
    PWR_EnterSTANDBYMode(PWR_STANDBYEntry_WFE);
    // NOTE: after here code runs on standby exit.
    if (ON_STANDBY_EXIT_PLL48_SYSCLK)
        PM_sysclk_pll48();
    // __enable_irq();
}

// NOTE: exiting standby returns to HSI 24MHz without PLL, call this after every AWU.
void PM_sysclk_pll48(void) {
    RCC_DeInit();
    RCC_HSICmd(ENABLE);
    RCC_HCLKConfig(RCC_SYSCLK_Div1);

    RCC_PLLCmd(DISABLE);
    RCC_PLLConfig(RCC_PLLSource_HSI_MUL2);
    RCC_PLLCmd(ENABLE);
    while (RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET);

    RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);
    RCC_ClocksTypeDef c;
    RCC_GetClocksFreq(&c);
    SystemCoreClock = c.SYSCLK_Frequency;
}