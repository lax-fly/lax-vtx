#pragma once

#include <stdint.h>
#include "dev.h"

extern int RegsterTable[DEV_ID_MAX];

class Serial : public Dev
{
private:
    int m_rxi;
    int m_rxj;
    uint8_t m_tx_buf[128];
    uint8_t m_rx_buf[128];
    void *m_handle;

public:
    // sync: 0 async, 1 sync, when use sync, you'd better check transmission is finished before next call
    void send(const uint8_t *data, int dsz, int sync = 0);
    int get_send_state(void);
    int recv(uint8_t *buffer, int bsz);

    int open(DEV_ID id, int baudrate, float stopbit = 1);
    virtual int open(DEV_ID id);
    virtual void close(void);
    virtual void event_poll(void);
};
