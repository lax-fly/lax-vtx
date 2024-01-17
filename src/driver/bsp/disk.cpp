#include "disk.h"
#include "assert.h"
#include "flash.h"
#include "clock.h"
#include "string.h"

#define DISK_RUM_INTERVAL 100

int Disk::open(DEV_ID id)
{
    switch (id)
    {
    case DEV_ID_DISK1:
        break;
    default:
        assert(false);
        break;
    }

    if (0 != register_dev(id, this, "disk"))
        return -1;

    m_id = id;
    memset(m_tasklist, 0, sizeof(m_tasklist));
    return 0;
}

void Disk::close(void)
{
    for (int i=0; i<MAX_DISK_OFFSET; i++)
    {
        if (m_tasklist[i].dsz)
        {
            flash_update_block(i, m_tasklist[i].data, m_tasklist[i].dsz);
        }

    }
    unregister_dev(m_id);
    m_id = DEV_ID_NONE;
}

int Disk::read(uint32_t offset, uint8_t *buf, uint32_t bsz)
{
    flash_read_block(offset, buf, bsz);
    return bsz;
}

// async write
int Disk::write(uint32_t offset, uint8_t *data, uint32_t dsz)
{
    assert(offset < MAX_DISK_OFFSET);
    m_tasklist[offset].data = data;
    m_tasklist[offset].dsz = dsz;
    return dsz;
}

void Disk::event_poll(void)
{
    uint32_t now = get_ms();
    if (now < m_run_time)
        return;

    m_run_time = now + DISK_RUM_INTERVAL;

    static int s = 0;
    // poll write task
    int i = s;
    while (1)
    {
        if (i == MAX_DISK_OFFSET)
            i = 0;

        if (m_tasklist[i].dsz)
        {
            flash_update_block(i, m_tasklist[i].data, m_tasklist[i].dsz);
            m_tasklist[i].dsz = 0;
            break; // one write per loop
        }

        if (((i + 1) % MAX_DISK_OFFSET) == s)
            break;

        ++i;
    }

    s = (i + 1) % MAX_DISK_OFFSET; // avoid thirst
}
