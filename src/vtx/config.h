#pragma once

#include <stdint.h>
#include "protocol.h"
#include "helpers.h"

#define versionEEPROM 0x1111

class Config
{
public:
    uint16_t version;
    Protocol::vtxMode_e vtxMode;
    uint16_t currFreq;          // direct freq value, used when freqMode == 1, independent from channel
    uint8_t channel;            // channel, used when freqMode == 0, also freq table index, not used for protocol Tramp
    uint8_t freqMode;           // for SA and MSP, 1 means use currFreq to set radio frequence, 0 means channel
    uint8_t pitMode;            // for all protocol
    uint8_t pitmodeInRange;     // for SA
    uint8_t pitmodeOutRange;    // for SA
    uint16_t pitmodeFreq;       // for MSP
    uint8_t currPowerdB;
    uint8_t unlocked;
    
    int load(void);
    int loaddefault(void);
    int save(void);
};

extern Config g_config;
