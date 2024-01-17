#pragma once
#include "stdint.h"
#include "dev.h"

#define MAX_DISK_OFFSET 2

class Disk : Dev
{
private:
    struct TaskWrite
    {
        uint8_t *data;
        uint32_t dsz;
    };
    TaskWrite m_tasklist[MAX_DISK_OFFSET];

public:
    virtual int  open(DEV_ID id);
    virtual void close(void)    ;
    virtual void event_poll(void);
    int  read(uint32_t offset, uint8_t* buf, uint32_t bsz);
    int  write(uint32_t offset, uint8_t* data, uint32_t dsz);
};
