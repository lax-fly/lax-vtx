#pragma once
#include "dev.h"

class Button : public Dev
{
public:
    typedef void (*CallBack)(void *);
    enum EventType
    {
        LONG_PRESS_1S,
        LONG_PRESS_10S,
        SHORT_PRESS,
    };

private:
    uint32_t m_sample_time;
    int m_tmp_state;
    int m_state;
    int m_press_time;
    void *m_gpio_handle;
    CallBack m_short_press_cb;
    CallBack m_long_press_1s_cb;
    CallBack m_long_press_10s_cb;
    void *m_data_s;
    void *m_data_l1s;
    void *m_data_l10s;

public:
    void register_event_callback(EventType type, CallBack callback, void *data);
    virtual int open(DEV_ID id);
    virtual void close(void);
    virtual void event_poll(void);
};
