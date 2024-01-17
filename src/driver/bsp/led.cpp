#include "led.h"
#include "gpio.h"
#include "clock.h"
#include "assert.h"
#include "chipset.h"

#define LED_POLL_INTERVAL 5

int Led::open(DEV_ID id)
{
    if (0 != register_dev(id, this, "led"))
    {
        return -2;
    }
    uint32_t pin = 0;
    switch (id)
    {
    case DEV_ID_LED1:
        pin = LED1;
        break;
    case DEV_ID_LED2:
        pin = LED2;
        break;
    case DEV_ID_LED3:
        pin = LED3;
        break;
    default:
        assert(false);
        break;
    }
    m_id = id;
    gpio_t gpio = gpio_out_setup(pin, OFF);
    m_gpio_handle = new gpio_t(gpio.regs, gpio.pin);
    m_id = DEV_ID_NONE;
    m_run_time = LED_POLL_INTERVAL;
    return 0;
}

void Led::close(void)
{
    gpio_out_write(*(gpio_t *)m_gpio_handle, OFF);
    unregister_dev(m_id);
    m_id = DEV_ID_NONE;
    delete (gpio_t *)m_gpio_handle; // cannot be null while created by new without std::nothrow
}

void Led::set(int state)
{
    gpio_out_write(*(gpio_t *)m_gpio_handle, state);
}

void Led::blink(int times, int interval)
{
    // setup the task, the real work will be done in event_poll
    if (m_blink_task.times)
        return;
    m_blink_task.interval = interval;
    m_blink_task.times = times;
}

void Led::event_poll(void)
{
    uint32_t now = get_ms();
    if (now > m_run_time && m_blink_task.times)
    {
        if (m_blink_task.len_state == ON)
        {
            set(OFF);
            m_blink_task.len_state = OFF;
            m_run_time = now + m_blink_task.interval;
            m_blink_task.times--;
        }
        else
        {
            set(ON);
            m_blink_task.len_state = ON;
            m_run_time = now + (50 > m_blink_task.interval ? m_blink_task.interval : 50);
        }
    }
}
