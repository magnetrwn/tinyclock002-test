#ifndef LEDMUX_H_
#define LEDMUX_H_

#define LEDMUX_SRCLK GPIO_Pin_3  // LCLK = SRCLK
#define LEDMUX_SER   GPIO_Pin_6  // LDATA = SER
#define LEDMUX_RCLK  GPIO_Pin_7  // LSET = RCLK

void LEDMUX_GPIOWalk(void);

#endif /* LEDMUX_H_ */