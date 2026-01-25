#include <ch32v00X.h>
#include <ch32v00X_rcc.h>
#include <ch32v00X_misc.h>
#include <debug.h>

#include "ledmux.h"
#include "uart.h"

int main(void) {
    SystemInit();
    Delay_Init();
    RCC_SYSCLKConfig(RCC_SYSCLKSource_HSI);
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
    SystemCoreClockUpdate();

    UART_Init(230400);

    Delay_Ms(1000);
    printf("SystemCoreClock: %d Hz\r\n", (int) SystemCoreClock);
    printf("Device ID: 0x%08x\r\n", (uint) DBGMCU_GetDEVID());

    LEDMUX_GPIOWalk();
}
