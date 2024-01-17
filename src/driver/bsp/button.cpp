#include "button.h"
#include "helpers.h"
#include "gpio.h"
#include "timer.h"
#include "chipset.h"
#include "assert.h"

#define DEBOUNCE_TIME   50
#define LONG_PRESS_TIME 1000
#define DEFAULT_TIME    10000

enum
{
    BUTTON_PRESSED,
    BUTTON_RELEASED
};
    
int  Button::open(DEV_ID id)
{
    if (0 != register_dev(id, this, "button"))
        return -1;
    
    gpio_t gpio;
    switch (id)
    {
        case DEV_ID_BUTTON1:
            gpio = gpio_in_setup(BUTTON1, 1);
            break;
        default:
            assert(false);
            break;
    }
    m_id = id;
    m_gpio_handle = new gpio_t(gpio.regs, gpio.pin);
    m_long_press_1s_cb = NULL;
    m_long_press_10s_cb = NULL;
    m_short_press_cb = NULL;
    m_sample_time = 50;
	m_tmp_state = BUTTON_RELEASED;
	m_state = BUTTON_RELEASED;
	m_press_time = 0;
	m_data_s = NULL;
	m_data_l1s = NULL;
	m_data_l10s = NULL;
    return 0;
}

void Button::close(void)
{
    unregister_dev(m_id);
    m_id = DEV_ID_NONE;
    m_long_press_1s_cb = NULL;
    m_long_press_10s_cb = NULL;
    m_short_press_cb = NULL;
    delete (gpio_t*)m_gpio_handle;
    m_gpio_handle = NULL;
    m_sample_time = 50;
	m_tmp_state = BUTTON_RELEASED;
	m_state = BUTTON_RELEASED;
	m_press_time = 0;
	m_data_s = NULL;
	m_data_l1s = NULL;
	m_data_l10s = NULL;
}
    
void Button::register_event_callback (EventType type, Button::CallBack callback, void* data)
{
    switch (type)
    {
        case LONG_PRESS_1S:
            m_long_press_1s_cb = callback;
            m_data_l1s = data;
            break;
        case LONG_PRESS_10S:
            m_long_press_10s_cb = callback;
            m_data_l10s = data;
            break;
        case SHORT_PRESS:
            m_short_press_cb = callback;
            m_data_s = data;
            break;
        default:
            assert(false);
            break;
    }
}


void Button::event_poll(void)
{
    uint32_t now = time_ms();

    if (now > m_sample_time)
    {
        int tmp_state = gpio_in_read(*(gpio_t*)m_gpio_handle);
        m_sample_time += DEBOUNCE_TIME;
        if (tmp_state == m_tmp_state)
        {
            if (m_state == BUTTON_RELEASED && tmp_state == BUTTON_PRESSED)	// motion pressing
            {
                m_press_time = now;
                m_state = tmp_state;
            }
            if (m_state == BUTTON_PRESSED && tmp_state == BUTTON_RELEASED)// motion releasing
            {
                int interval = now - m_press_time;
                if (interval < LONG_PRESS_TIME)
                {
                    if (m_short_press_cb)
                        m_short_press_cb(m_data_s);
                } else
                // long press
                if (interval < DEFAULT_TIME)
                {
                    if (m_long_press_1s_cb)
                        m_long_press_1s_cb(m_data_l1s);
                } else
                // default EEPROM
                {
                    if (m_long_press_10s_cb)
                        m_long_press_10s_cb(m_data_l10s);
                }
                m_state = tmp_state;
            }
        }
        m_tmp_state = tmp_state;
    }
}
