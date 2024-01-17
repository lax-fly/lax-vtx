#pragma once
#include "event.h"

void trampBuildrPacket(void);
int trampProcessSerial(void);
void trampReset(void);

class Protobol : public Event
{
    virtual void poll(void);
    virtual void set_protocol(void);
    //virtual void poll(void);
};

class Tramp : public Protobol
{
    
};
