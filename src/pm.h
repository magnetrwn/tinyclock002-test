#ifndef POWER_MGMT_H_
#define POWER_MGMT_H_

#include <stdint.h>

typedef enum {
    ON_STANDBY_EXIT_HSI24_SYSCLK, // TODO: check if this is not always HSI24, but could be different at init.
    ON_STANDBY_EXIT_PLL48_SYSCLK
} PM_on_standby_exit_e;

void PM_standby_init(uint32_t wakeup_ms);
void PM_standby_enter(PM_on_standby_exit_e e);
void PM_sysclk_pll48(void);

#endif /* POWER_MGMT_H_ */