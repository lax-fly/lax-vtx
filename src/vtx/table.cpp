#include "table.h"
#include <string.h>
#include <math.h>
#include "helpers.h"
#include "main.h"

#define FREQ_TABLE_SIZE 48

static const uint8_t channelFreqLabel[48] = {
    'B', 'A', 'N', 'D', '_', 'A', ' ', ' ', // A
    'B', 'A', 'N', 'D', '_', 'B', ' ', ' ', // B
    'B', 'A', 'N', 'D', '_', 'E', ' ', ' ', // E
    'F', 'A', 'T', 'S', 'H', 'A', 'R', 'K', // F
    'R', 'A', 'C', 'E', ' ', ' ', ' ', ' ', // R
    'R', 'A', 'C', 'E', '_', 'L', 'O', 'W', // L
};

static const uint8_t bandLetter[6] = {'A', 'B', 'E', 'F', 'R', 'L'};

static uint16_t channelFreqTable[FREQ_TABLE_SIZE] = {
    5865, 5845, 5825, 5805, 5785, 5765, 5745, 5725, // A
    5733, 5752, 5771, 5790, 5809, 5828, 5847, 5866, // B
    5705, 5685, 5665, 5645, 5885, 5905, 5925, 5945, // E
    5740, 5760, 5780, 5800, 5820, 5840, 5860, 5880, // F
    5658, 5695, 5732, 5769, 5806, 5843, 5880, 5917, // R
    5362, 5399, 5436, 5473, 5510, 5547, 5584, 5621  // L[owRace]
};

/* SA2.1 powerlevels in dBm.
 *
 * INav:
 *    Max of 5 [https://github.com/iNavFlight/inav/blob/a8016edd0d6f05bb12a75b0ea75a3483772baaeb/src/main/io/vtx_smartaudio.h#L36]
 *    Index 0 is ignored [https://github.com/iNavFlight/inav/blob/a8016edd0d6f05bb12a75b0ea75a3483772baaeb/src/main/io/vtx_smartaudio.c#L334]
 *
 */
uint8_t saPowerLevelsLut[SA_NUM_POWER_LEVELS] = {0, RACE_MODE_POWER, 14, 17, 20, 23, 26, 30};   // in dbm

uint8_t saPowerLevelsLabel[SA_NUM_POWER_LEVELS * POWER_LEVEL_LABEL_LENGTH] = 
{   // upper case only because betafligh will turn every char to upper case
    '1', 'M', 'W',
    'R', 'C', 'E',
    '2', '5', ' ',
    '5', '0', ' ',
    '1', '0', '0',
    '2', '0', '0',
    '4', '0', '0',
    '1', 'W', ' '
};

uint8_t getPowerTableSize(void)
{
    return ARRAY_SIZE(saPowerLevelsLut);
}

uint8_t getPowerByIdx(uint8_t idx)
{
    if (idx < ARRAY_SIZE(saPowerLevelsLut))
        return saPowerLevelsLut[idx];
    else
        return saPowerLevelsLut[0];
}

uint8_t getIdxByPower(uint8_t power)
{
    for (uint8_t i=0; i<SA_NUM_POWER_LEVELS; ++i)
    {
        if (power == saPowerLevelsLut[i])
            return i;
    }
    return 0;
}

uint8_t getFreqTableSize(void)
{
    return ARRAY_SIZE(channelFreqTable);
}

uint8_t getFreqTableBands(void)
{
    return getFreqTableSize() / getFreqTableChannels();
}

uint8_t getFreqTableChannels(void)
{
    return 8;
}

uint16_t getFreqByIdx(uint8_t idx)
{
    return channelFreqTable[idx];
}

uint8_t channelFreqLabelByIdx(uint8_t idx)
{
    return channelFreqLabel[idx];
}

uint8_t getBandLetterByIdx(uint8_t idx)
{
    return bandLetter[idx];
}


#define BOOTLOADER_KEY  0x4f565458 // OVTX
#define BOOTLOADER_TYPE 0xACDC

struct bootloader {
    uint32_t key;
    uint32_t reset_type;
    uint32_t baud;
};

uint32_t _bootloader_data = 0x08000000;	// TODO
void reboot_into_bootloader(uint32_t baud)
{
    struct bootloader * blinfo = (struct bootloader*)&_bootloader_data;
    blinfo->key = BOOTLOADER_KEY;
    blinfo->reset_type = BOOTLOADER_TYPE;
    blinfo->baud = baud;

    sleep_ms(200);
    reboot();
}
