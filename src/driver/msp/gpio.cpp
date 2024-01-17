#include "at32f421_conf.h"
#include "gpio.h"

gpio_t gpio_out_setup(uint32_t pin, uint32_t val)
{
    uint32_t gpio_periph = GPIOA_BASE + 0x400 * GPIO2PORT(pin);
    uint32_t gpio_pin = GPIO2BIT(pin);

    gpio_init_type gpio_init_struct;
    gpio_default_para_init(&gpio_init_struct);

    gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_MODERATE;
    gpio_init_struct.gpio_out_type = GPIO_OUTPUT_PUSH_PULL;
    gpio_init_struct.gpio_mode = GPIO_MODE_OUTPUT;
    gpio_init_struct.gpio_pins = gpio_pin;
    gpio_init_struct.gpio_pull = GPIO_PULL_NONE;
    gpio_init((gpio_type *)gpio_periph, &gpio_init_struct);

    gpio_t g(gpio_periph, gpio_pin);
    gpio_out_write(g, val);
    return g;
}

gpio_t gpio_in_out_setup(uint32_t pin, uint32_t val)
{
    uint32_t gpio_periph = GPIOA_BASE + 0x400 * GPIO2PORT(pin);
    uint32_t gpio_pin = GPIO2BIT(pin);

    gpio_init_type gpio_init_struct;
    gpio_default_para_init(&gpio_init_struct);

    gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_MODERATE;
    gpio_init_struct.gpio_out_type = GPIO_OUTPUT_OPEN_DRAIN;
    gpio_init_struct.gpio_mode = GPIO_MODE_OUTPUT;
    gpio_init_struct.gpio_pins = gpio_pin;
    gpio_init_struct.gpio_pull = GPIO_PULL_UP;
    gpio_init((gpio_type *)gpio_periph, &gpio_init_struct);

    gpio_t g(gpio_periph, gpio_pin);
    gpio_out_write(g, val);
    return g;
}

void gpio_out_toggle(gpio_t g)
{
    if (gpio_input_data_bit_read((gpio_type *)g.regs, g.pin))
        gpio_bits_reset((gpio_type *)g.regs, g.pin);
    else
        gpio_bits_set((gpio_type *)g.regs, g.pin);
}

void gpio_out_write(gpio_t g, uint32_t val)
{
    if (val)
        gpio_bits_set((gpio_type *)g.regs, g.pin);
    else
        gpio_bits_reset((gpio_type *)g.regs, g.pin);
}

uint8_t gpio_out_read(gpio_t g)
{
    return gpio_output_data_bit_read((gpio_type *)g.regs, g.pin);
}

static inline gpio_pull_type PULL_TYPE(int32_t x)
{
    gpio_pull_type _type = GPIO_PULL_NONE;
    if (x < 0)
    {
        _type = GPIO_PULL_DOWN;
    }
    else if (0 < x)
    {
        _type = GPIO_PULL_UP;
    }
    return _type;
}

gpio_t gpio_in_setup(uint32_t pin, int32_t pull_up)
{
    uint32_t gpio_periph = GPIOA_BASE + 0x400 * GPIO2PORT(pin);
    uint32_t gpio_pin = GPIO2BIT(pin);

    gpio_init_type gpio_init_struct;
    gpio_default_para_init(&gpio_init_struct);

    gpio_pull_type pull_type = PULL_TYPE(pull_up);

    /* gpio input config */
    gpio_init_struct.gpio_mode = GPIO_MODE_INPUT;
    gpio_init_struct.gpio_pins = gpio_pin;
    gpio_init_struct.gpio_pull = pull_type;
    gpio_init((gpio_type *)gpio_periph, &gpio_init_struct);

    return gpio_t(gpio_periph, gpio_pin);
}

gpio_t gpio_analog_setup(uint32_t pin)
{
    uint32_t gpio_periph = GPIOA_BASE + 0x400 * GPIO2PORT(pin);
    uint32_t gpio_pin = GPIO2BIT(pin);

    gpio_init_type gpio_init_struct;
    gpio_default_para_init(&gpio_init_struct);

    /* gpio input config */
    gpio_init_struct.gpio_mode = GPIO_MODE_ANALOG;
    gpio_init_struct.gpio_pins = gpio_pin;
    gpio_init((gpio_type *)gpio_periph, &gpio_init_struct);

    return gpio_t(gpio_periph, gpio_pin);
}

uint8_t gpio_in_read(gpio_t g)
{
    return gpio_input_data_bit_read((gpio_type *)g.regs, g.pin);
}

void gpio_af_setup(uint32_t pin, uint8_t af, int8_t pull_up)
{
    gpio_init_type gpio_init_struct;

    gpio_default_para_init(&gpio_init_struct);

    uint32_t gpio_periph = GPIOA_BASE + 0x400 * GPIO2PORT(pin);
    uint32_t gpio_pin = GPIO2BIT(pin);
    uint32_t gpio_pin_src = GPIO2IDX(pin);

    gpio_pull_type pull_type = PULL_TYPE(pull_up);

    /* configure the tmr3 CH3 pin */
    gpio_init_struct.gpio_pins = gpio_pin;
    gpio_init_struct.gpio_mode = GPIO_MODE_MUX;
    gpio_init_struct.gpio_out_type = pull_type != GPIO_PULL_NONE ? GPIO_OUTPUT_OPEN_DRAIN : GPIO_OUTPUT_PUSH_PULL;
    gpio_init_struct.gpio_pull = pull_type;
    gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_MODERATE;
    gpio_init((gpio_type *)gpio_periph, &gpio_init_struct);

    gpio_pin_mux_config((gpio_type *)gpio_periph, (gpio_pins_source_type)gpio_pin_src, (gpio_mux_sel_type)af);
}
