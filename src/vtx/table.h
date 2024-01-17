#pragma once

#include <stdint.h>

#define TRAMP_BAUD                      9600
#define SMARTAUDIO_BAUD                 4800
#define MSP_BAUD                        9600 // ~10% decrease in setup time increasing to 19200. But not worth it for the inceased chance in packets errors due to heat.

#define PROTOCOL_CHECK_TIMEOUT          200

#define RACE_MODE_POWER                 10  // dBm

#define BAND_NAME_LENGTH                8
#define IS_FACTORY_BAND                 1
#define CHANNEL_COUNT                   8
#define MAX_POWER_LEVEL                 16

#define POWER_LEVEL_LABEL_LENGTH    3
#define SA_NUM_POWER_LEVELS         8 // Max 8 for INAV.

extern uint8_t saPowerLevelsLut[SA_NUM_POWER_LEVELS];
extern uint8_t saPowerLevelsLabel[SA_NUM_POWER_LEVELS * POWER_LEVEL_LABEL_LENGTH];

uint8_t getPowerTableSize(void);
uint8_t getPowerByIdx(uint8_t idx);
uint8_t getIdxByPower(uint8_t power);
uint8_t getFreqTableSize(void);
uint8_t getFreqTableBands(void);
uint8_t getFreqTableChannels(void);
uint16_t getFreqByIdx(uint8_t idx);

uint8_t getBandLetterByIdx(uint8_t idx);
uint8_t channelFreqLabelByIdx(uint8_t idx);

void reboot_into_bootloader(uint32_t baud);

class PowerTable
{
    uint8_t size();
    uint8_t operator[](int i);
};

class FreqTable
{
    uint8_t size();
    uint8_t band_cnt();
    uint8_t channel_cnt();
    uint8_t operator[](int i);
};



