#include "ds1302.h"

#include <ch32v00X.h>
#include <ch32v00X_gpio.h>
#include <ch32v00X_rcc.h>
#include <debug.h>

static inline uint8_t bin2bcd_u8(uint8_t v) {
    uint8_t tens = 0;
    while (v >= 10) { v -= 10; tens++; }
    return (uint8_t) ((tens << 4) | v);
}

static inline uint8_t bcd2bin_u8(uint8_t b) {
    return (uint8_t) (((b >> 4) & 0x0F) * 10 + (b & 0x0F));
}

static inline uint8_t year_to_yy(uint16_t year) {
    uint16_t y = year;
    for (; y > 100; y -= 100);
    return (uint8_t) y;
}

static inline void hi_io(void) { GPIO_SetBits(DS_PORT_IO, DS_IO); }
static inline void lo_io(void) { GPIO_ResetBits(DS_PORT_IO, DS_IO); }
static inline void hi_sclk(void) { GPIO_SetBits(DS_PORT_SCLK, DS_SCLK); }
static inline void lo_sclk(void) { GPIO_ResetBits(DS_PORT_SCLK, DS_SCLK); }
static inline void hi_ce(void) { GPIO_SetBits(DS_PORT_CE, DS_CE); }
static inline void lo_ce(void) { GPIO_ResetBits(DS_PORT_CE, DS_CE); }
static inline void ds_delay(void) { Delay_Us(2); }

static void ds_io_out(void) {
    GPIO_InitTypeDef g = {0};
    g.GPIO_Speed = GPIO_Speed_30MHz;
    g.GPIO_Mode  = GPIO_Mode_Out_PP;
    g.GPIO_Pin   = DS_IO;
    GPIO_Init(DS_PORT_IO, &g);
}

static void ds_io_in(void) {
    GPIO_InitTypeDef g = {0};
    g.GPIO_Speed = GPIO_Speed_30MHz;
    g.GPIO_Mode  = GPIO_Mode_IN_FLOATING;
    g.GPIO_Pin   = DS_IO;
    GPIO_Init(DS_PORT_IO, &g);
}

static void DS1302_begin(void) {
    RCC_PB2PeriphClockCmd(DS_PB, ENABLE);

    GPIO_InitTypeDef g = {0};
    g.GPIO_Speed = GPIO_Speed_30MHz;
    g.GPIO_Mode  = GPIO_Mode_Out_PP;
    g.GPIO_Pin   = DS_SCLK;
    GPIO_Init(DS_PORT_SCLK, &g);

    g.GPIO_Pin = DS_CE;
    GPIO_Init(DS_PORT_CE, &g);

    lo_ce();
    lo_sclk();
    ds_io_out();
    lo_io();
}

static void DS1302_end_to_i2c(void) {
    lo_ce();
    lo_sclk();

    GPIO_InitTypeDef g = {0};
    g.GPIO_Speed = GPIO_Speed_30MHz;
    g.GPIO_Mode  = GPIO_Mode_AF_OD;
    g.GPIO_Pin   = DS_IO;
    GPIO_Init(DS_PORT_IO, &g);

    g.GPIO_Pin   = DS_SCLK;
    GPIO_Init(DS_PORT_SCLK, &g);

    g.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    g.GPIO_Pin  = DS_CE;
    GPIO_Init(DS_PORT_CE, &g);
}

static void DS1302_write_byte(uint8_t v) {
    ds_io_out();

    // NOTE: LSB first
    for (int i = 0; i < 8; i++) {
        (v & 1u) ? hi_io() : lo_io();
        v >>= 1;

        ds_delay();
        hi_sclk();
        ds_delay();
        lo_sclk();
    }
}

static uint8_t DS1302_read_byte(void) {
    uint8_t v = 0;
    ds_io_in();

    for (int i = 0; i < 8; i++) {
        if (GPIO_ReadInputDataBit(DS_PORT_IO, DS_IO)) 
            v |= (1u << i);
        ds_delay();
        hi_sclk();
        ds_delay();
        lo_sclk();
    }

    return v;
}

static uint8_t DS1302_read_reg(uint8_t reg) {
    hi_ce();
    ds_delay();
    DS1302_write_byte(reg | 1u);
    uint8_t v = DS1302_read_byte();
    lo_ce();
    ds_delay();
    return v;
}

static void DS1302_write_reg(uint8_t reg, uint8_t val) {
    hi_ce();
    ds_delay();
    DS1302_write_byte(reg & 254u);
    DS1302_write_byte(val);
    lo_ce();
    ds_delay();
}

void DS1302_init_basic(void) {
    DS1302_begin();

    DS1302_write_reg(DS1302_REG_WP, DS1302_WP_DISABLE);

    // Ensure CH (clock halt) bit in seconds is 0
    uint8_t s = DS1302_read_reg(DS1302_REG_SECONDS);
    if (s & 0x80)
        DS1302_write_reg(DS1302_REG_SECONDS, (uint8_t)(s & ~0x80));

    DS1302_write_reg(DS1302_REG_WP, DS1302_WP_ENABLE);

    DS1302_end_to_i2c();
}

// NOTE: 24h only, don't use in 12h mode
int DS1302_get_time(rtc_time_t* t) {
    DS1302_begin();

    hi_ce();
    ds_delay();
    DS1302_write_byte(DS1302_BURST_CLOCK_RD);
    uint8_t sec = DS1302_read_byte();
    t->sec   = bcd2bin_u8(sec & 0x7F);
    t->min   = bcd2bin_u8(DS1302_read_byte() & 0x7F);
    t->hour  = bcd2bin_u8(DS1302_read_byte() & 0x3F);
    t->day   = bcd2bin_u8(DS1302_read_byte() & 0x3F);
    t->month = bcd2bin_u8(DS1302_read_byte() & 0x1F);
    t->dow   = bcd2bin_u8(DS1302_read_byte() & 0x07);
    t->year  = bcd2bin_u8(DS1302_read_byte());
    lo_ce();
    ds_delay();

    DS1302_end_to_i2c();

    // NOTE: If CH bit set, time is not running
    if (sec & 0x80) 
        return -1;

    return 0;
}

void DS1302_set_time(const rtc_time_t* t) {
    DS1302_begin();

    DS1302_write_reg(DS1302_REG_WP, DS1302_WP_DISABLE);

    hi_ce();
    ds_delay();
    DS1302_write_byte(DS1302_BURST_CLOCK_WR);
    DS1302_write_byte(bin2bcd_u8(t->sec) & 0x7F);       // seconds (CH cleared)
    DS1302_write_byte(bin2bcd_u8(t->min) & 0x7F);       // minutes
    DS1302_write_byte(bin2bcd_u8(t->hour) & 0x3F);      // hours (24h mode)
    DS1302_write_byte(bin2bcd_u8(t->day) & 0x3F);       // date 1..31
    DS1302_write_byte(bin2bcd_u8(t->month) & 0x1F);     // month 1..12
    DS1302_write_byte(bin2bcd_u8(t->dow) & 0x07);       // day-of-week 1..7 (user-defined but sequential)
    DS1302_write_byte(bin2bcd_u8(year_to_yy(t->year))); // year 00..99
    //DS1302_write_byte(DS1302_WP_ENABLE);                // control reg (can you set WP=0 in burst too?)
    lo_ce();
    ds_delay();

    DS1302_write_reg(DS1302_REG_WP, DS1302_WP_ENABLE);

    DS1302_end_to_i2c();
}