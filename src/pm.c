#include "pm.h"

#include <ch32v00X_pwr.h>
#include <ch32v00X_exti.h>

void PM_standby_init(int wakeup_ms) {
    RCC_PB1PeriphClockCmd(RCC_PB1Periph_PWR, ENABLE);

    RCC_LSICmd(ENABLE);
    while (RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == RESET) {}

    PWR_AWU_SetPrescaler(PWR_AWU_Prescaler_4096);
    PWR_AWU_SetWindowValue(wakeup_ms >> 5); // min: 31.25ms step, 32ms for granularity
    PWR_AutoWakeUpCmd(ENABLE);

    RCC_PB2PeriphClockCmd(RCC_PB2Periph_AFIO, ENABLE);

    EXTI_InitTypeDef ex = {0};
    ex.EXTI_Line    = EXTI_Line9; // AWU is wired internally to EXTI9
    ex.EXTI_Mode    = EXTI_Mode_Event; // NOTE: Event, not Interrupt
    ex.EXTI_Trigger = EXTI_Trigger_Falling;
    ex.EXTI_LineCmd = ENABLE;
    EXTI_Init(&ex);
}

void PM_standby_enter(void) {
    PWR_EnterSTANDBYMode(PWR_STANDBYEntry_WFE);
}