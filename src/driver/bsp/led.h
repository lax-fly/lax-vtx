#pragma once
#include "stddef.h"
#include "dev.h"

class Led : public Dev
{
private:
    void *m_gpio_handle;
    struct BlinkTask
    {
        int len_state;
        int interval;
        int times;
    } m_blink_task;

public:
    enum State
    {
        OFF = 0,
        ON = 1,
    };

public:
    void set(int state);
    void blink(int times, int interval = 100);
    virtual int open(DEV_ID id);
    virtual void close(void);
    virtual void event_poll(void);
};
