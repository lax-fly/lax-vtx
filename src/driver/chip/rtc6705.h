#pragma once

#include <stdint.h>
#include "chipset.h"
#include "gpio.h"

class RTC6705 : public RfModChip
{
private:
    gpio_t ss_pin;
    gpio_t sck_pin;
    gpio_t mosi_pin;

private:
    uint32_t read_reg(uint8_t addr);
    void write_reg(uint8_t addr, uint32_t data);
    void power_on(void);
    void power_off(void);

public:
    virtual ~RTC6705();
    virtual int init(ChipId id);
    virtual void uninit(void);
    virtual int set_freq(uint32_t freq);
    virtual int set_power(uint32_t dbm);
    virtual bool is_busy(void);
    virtual void print_all_regs(char *buf, int bsz);
};
