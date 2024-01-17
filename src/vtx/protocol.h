#pragma once

#define PROTOCOL_LISTENING 0
#define PROTOCOL_ERROR -1
#define PROTOCOL_OK 1
#define PROTOCOL_TIMEOUT 500 // ms

namespace Protocol
{
    typedef enum
    {
        BUTTON,
        TRAMP,
        SMARTAUDIO,
        MSP,
        // BOOTLOADER,
        VTX_MODE_MAX
    } vtxMode_e;

    void init(vtxMode_e proto = BUTTON);
    void poll();

};
