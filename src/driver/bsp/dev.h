#pragma once
#include "stdint.h"

enum DEV_ID
{
    DEV_ID_NONE,
    DEV_ID_USART1,
    DEV_ID_USART2,
    DEV_ID_LED1,
    DEV_ID_LED2,
    DEV_ID_LED3,
    DEV_ID_BUTTON1,
    DEV_ID_RF1,
    DEV_ID_DISK1,
    DEV_ID_MAX
};

class Dev
{
protected:
    DEV_ID m_id;
    uint32_t m_run_time;
public:
    virtual int  open(DEV_ID id)  = 0;
    virtual void close(void)      = 0;
    virtual void event_poll(void) = 0;
    static void poll(void);
};


int register_dev(DEV_ID id, Dev* handle, const char* name);
void unregister_dev(DEV_ID& id);
void reboot(void);

