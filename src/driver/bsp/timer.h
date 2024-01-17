#pragma once
#include "stdint.h"
#include "dev.h"

#define TIMEOUT_NEVER ((uint32_t)-1)

uint32_t time_ms();
uint32_t time_us();

void sleep_ms(uint32_t ms);
void sleep_us(uint32_t us);
void sleep_s(uint32_t s);

class Timeout
{
private:
    uint32_t m_outtime;
    uint32_t m_timeout;

public:
    bool check(void);
    void set(uint32_t timeout);
    void fresh(void);
    Timeout(uint32_t timeout);

    static void init(void);
};
