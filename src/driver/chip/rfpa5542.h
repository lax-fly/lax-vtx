#pragma once
#include <stdint.h>
#include "chipset.h"
#include "adc.h"
#include "gpio.h"
#include "pwm.h"

class PA5542 : public RfPaChip
{
private:
    adc_t vpd_pin;
    gpio_t vref_pin;
    pwm_t outputPowerTimer;
    uint32_t pwm_val;

public:
    virtual ~PA5542();
    virtual int init(ChipId id);
    virtual void uninit(void);
    virtual int set_amp(uint32_t val);
    virtual void print_all_regs(char *buf, int bsz);
};
