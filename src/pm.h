#ifndef POWER_MGMT_H_
#define POWER_MGMT_H_

void PM_standby_init(int wakeup_ms);
void PM_standby_enter(void);
void PM_sysclk_pll_48mhz(void);

#endif /* POWER_MGMT_H_ */