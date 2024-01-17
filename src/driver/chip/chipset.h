#pragma once

#define MAX_POWER 1600 // mW

#define USART1_TX PA9
#define USART1_RX PA9

#define USART2_TX PA2
#define USART2_RX PA3

#define RTC6705_1_SPI_SS PA6
#define RTC6705_1_SPI_CLOCK PA5
#define RTC6705_1_SPI_MOSI PA7
#define RTC6705_MAX_POWER 13  // dbm
#define RTC6705_MIN_FREQ 4900 // dbm
#define RTC6705_MAX_FREQ 5925 // dbm

#define LED1 PB7 // Red (power)
#define LED2 PB6 // Green (connected)
#define LED3 PB5 // Blue (SA message)

#define PA5542_1_VREF PA1
#define PA5542_1_VPD PA0
#define PA5542_MAX_GAIN 33 // db

#define RTC_BIAS PB0

#define BUTTON1 PB4

enum ChipId
{
    CHIP_ID_RF6705_1,
    CHIP_ID_PA5542_1,
    CHIP_ID_MCUFLASH_1,
};

#include "stdint.h"

/*
    defined interfaces for all chips
*/

class RfModChip
{
public:
    virtual ~RfModChip(){};
    virtual int set_freq(uint32_t freq) = 0;
    virtual int set_power(uint32_t dbm) = 0;
    virtual bool is_busy(void) = 0;
    virtual void print_all_regs(char *buf, int bsz) = 0;
    static RfModChip *new_instance(ChipId id);
};

class RfPaChip
{
public:
    virtual ~RfPaChip(){};
    virtual int set_amp(uint32_t dbm) = 0;
    virtual void print_all_regs(char *buf, int bsz) = 0;
    static RfPaChip *new_instance(ChipId id);
};
