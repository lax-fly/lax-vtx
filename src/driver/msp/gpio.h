#pragma once

#include "pins.h"
#include <stdint.h>

struct gpio_t
{
    uint32_t regs;
    uint32_t pin;
    gpio_t()
    {
    }
    gpio_t(uint32_t regs, uint32_t pin) : regs(regs), pin(pin)
    {
    }
};

gpio_t gpio_out_setup(uint32_t pin, uint32_t val);
void gpio_out_toggle(gpio_t g);
void gpio_out_write(gpio_t g, uint32_t val);
uint8_t gpio_out_read(gpio_t g);
gpio_t gpio_in_setup(uint32_t pin, int32_t pull_up);
uint8_t gpio_in_read(gpio_t g);

// each setup pin must work in pair with release when not used any more
void gpio_release(gpio_t g);

gpio_t gpio_in_out_setup(uint32_t pin, uint32_t val);
void gpio_af_setup(uint32_t pin, uint8_t af, int8_t pull_up);
gpio_t gpio_analog_setup(uint32_t pin);
