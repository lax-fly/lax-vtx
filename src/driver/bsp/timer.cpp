#include "clock.h"
#include "timer.h"

// for future os support

void Timeout::init(void)
{
    clock_init();
}

void sleep_ms(uint32_t ms)
{
    delay_ms(ms);
}

void sleep_us(uint32_t us)
{
    delay_us(us);
}

void sleep_s(uint32_t s)
{
    uint32_t ms = s * 1000;
    sleep_ms(ms);
}

uint32_t time_ms()
{
    return get_ms();
}

uint32_t time_us()
{
    return get_us();
}

Timeout::Timeout(uint32_t timeout)
{
    set(timeout);
}

bool Timeout::check(void)
{
    uint32_t now = get_ms();
    if (m_timeout != (uint32_t)-1 && now > m_outtime)
    {
        fresh();
        return true;
    }
    return false;
}

void Timeout::set(uint32_t timeout)
{
    m_timeout = timeout;
    m_outtime = get_ms() + timeout;
}

void Timeout::fresh(void)
{
    m_outtime = get_ms() + m_timeout;
}
