#include "serial.h"
#include "usart.h"
#include "pins.h"
#include "timer.h"
#include "string.h"
#include "stdlib.h"
#include "helpers.h"
#include "assert.h"
#include "chipset.h"

#define SERIAL_POLL_INTERVAL 5

int Serial::open(DEV_ID id)
{
    return open(id, 115200);
}

int Serial::open(DEV_ID id, int baudrate, float stopbit)
{
    if (0 != register_dev(id, this, "serial"))
        return -2;

    switch (id)
    {
    case DEV_ID_USART1:
        m_handle = usart_config(USART1_TX, USART1_RX, baudrate, stopbit);
        break;
    case DEV_ID_USART2:
        m_handle = usart_config(USART2_TX, USART2_RX, baudrate, stopbit);
        break;
    default:
        assert(false);
        break;
    }
    m_id = id;
    m_rxi = m_rxj = 0;
    memset(m_tx_buf, 0, sizeof(m_tx_buf));
    memset(m_rx_buf, 0, sizeof(m_rx_buf));
    m_run_time = SERIAL_POLL_INTERVAL;
    return 0;
}

void Serial::close(void)
{
    unregister_dev(m_id);
    m_id = DEV_ID_NONE;
    m_rxi = m_rxj = 0;
    m_handle = NULL;
}

void Serial::send(const uint8_t *data, int dsz, int sync)
{
    assert(dsz <= (int)sizeof(m_tx_buf) && m_handle);

    if (dsz > 0)
    {
        memcpy(m_tx_buf, data, dsz);
        if (sync)
            usart_sync_send(m_handle, m_tx_buf, dsz);
        else
            usart_async_send(m_handle, m_tx_buf, dsz);
    }
}

void Serial::event_poll(void)
{
    assert(m_handle);
    uint32_t now = time_ms();
    if (now > m_run_time)
    {
        m_run_time = now + SERIAL_POLL_INTERVAL;
        m_rxj = usart_async_recv(m_handle, m_rx_buf, (int)sizeof(m_rx_buf));
    }
}

// int Serial::recv(uint8_t *buffer, int bsz)
// {
//     assert(buffer);
//     int rd_sz = 0;

//     if (m_rxi < m_rxj)
//     {
//         int dsz = m_rxj - m_rxi;
//         rd_sz = dsz > bsz ? bsz : dsz;
//     }
//     else if (m_rxi > m_rxj)
//     {
//         int dsz = sizeof(m_rx_buf) - m_rxi;
//         rd_sz = dsz > bsz ? bsz : dsz;
//     }

//     if (rd_sz > 0)
//     {
//         memcpy(buffer, m_rx_buf + m_rxi, rd_sz);
//         m_rxi = (m_rxi + rd_sz) % sizeof(m_rx_buf);
//     }
//     return rd_sz;
// }

int Serial::recv(uint8_t *buffer, int bsz)
{
    assert(buffer);
    int rd_sz = 0;

    if (m_rxi < m_rxj)
    {
        int dsz = m_rxj - m_rxi;
        rd_sz = dsz > bsz ? bsz : dsz;
        memcpy(buffer, m_rx_buf + m_rxi, rd_sz);
        m_rxi += rd_sz;
    }
    else if (m_rxi > m_rxj)
    {
        int dsz = sizeof(m_rx_buf) - m_rxi;
        if (dsz > bsz)
        {
            rd_sz = bsz;
            memcpy(buffer, m_rx_buf + m_rxi, rd_sz);
            m_rxi += bsz;
        }
        else
        {
            rd_sz = dsz;
            memcpy(buffer, m_rx_buf + m_rxi, rd_sz);
            m_rxi = 0;
            bsz -= rd_sz;
            dsz = m_rxj;
            if (bsz > 0 && dsz > 0)
            {
                int rd_sz2 = dsz > bsz ? bsz : dsz;
                memcpy(buffer + rd_sz, m_rx_buf, rd_sz2);
                rd_sz += rd_sz2;
                m_rxi = rd_sz2;
            }
        }
    }

    return rd_sz;
}
