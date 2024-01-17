#pragma once

#include <stdint.h>
#include "pins.h"

struct adc_t
{
    uint32_t ch;
    adc_t() {}
    adc_t(uint32_t ch) : ch(ch)
    {
    }
};

adc_t adc_config(uint32_t pin);
uint32_t adc_read(adc_t config);
float adc_get_temp(void);
