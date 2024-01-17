#pragma once
#include "stdint.h"

class Event
{
protected:
    uint32_t run_time;
public:
    virtual void poll(void) = 0;
};
