#pragma once

#include <stdint.h>
#include "pins.h"

struct pwm_t
{
    uint32_t tim;
    uint16_t ch;
    pwm_t() {}
    pwm_t(uint32_t tim, uint16_t ch) : tim(tim), ch(ch)
    {
    }
};
pwm_t pwm_init(uint32_t pin);
// dutycycle 0~10000 to 0.0000 ~ 1.0
void pwm_out_write(pwm_t pwm, uint16_t dutycycle);
