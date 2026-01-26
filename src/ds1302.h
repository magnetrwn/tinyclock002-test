#ifndef DS1302_H_
#define DS1302_H_

#include <stdint.h>

#define DS_PB (RCC_PB2Periph_GPIOC | RCC_PB2Periph_GPIOD)
#define DS_PORT_IO   GPIOC
#define DS_PORT_SCLK GPIOC
#define DS_PORT_CE   GPIOD
#define DS_IO    GPIO_Pin_1   // I/O = PC1
#define DS_SCLK  GPIO_Pin_2   // SCLK = PC2
#define DS_CE    GPIO_Pin_4   // /RST = PD4

#define DS1302_CMD_READ   0x01
#define DS1302_CMD_WRITE  0x00

#define DS1302_REG_SECONDS 0x80
#define DS1302_REG_MINUTES 0x82
#define DS1302_REG_HOURS   0x84
#define DS1302_REG_DATE    0x86
#define DS1302_REG_MONTH   0x88
#define DS1302_REG_DAY     0x8A
#define DS1302_REG_YEAR    0x8C
#define DS1302_REG_WP      0x8E

#define DS1302_BURST_CLOCK_WR 0xBE
#define DS1302_BURST_CLOCK_RD 0xBF

#define DS1302_WP_ENABLE  0x80
#define DS1302_WP_DISABLE 0x00

typedef struct {
    uint8_t sec, min, hour;
    uint8_t day, month;
    uint16_t year;
    uint8_t dow;
} rtc_time_t;

void DS1302_init_basic(void);
int DS1302_get_time(rtc_time_t* t);
void DS1302_set_time(const rtc_time_t* t);

#endif /* DS1302_H_ */