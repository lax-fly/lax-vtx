#include "mspVtx.h"
#include "table.h"
#include "Config.h"
#include "serial.h"
#include <string.h>
#include "helpers.h"
#include "main.h"

#define MSP_HEADER_DOLLAR 0x24
#define MSP_HEADER_X 0x58
#define MSP_HEADER_REQUEST 0x3C
#define MSP_HEADER_RESPONSE 0x3E
#define MSP_HEADER_ERROR 0x21
#define MSP_HEADER_SIZE 8

#define FC_QUERY_PERIOD_MS 200

enum MspFunction
{
    MSP_VTX_CONFIG = 88,               // out message         Get vtx settings - betaflight
    MSP_SET_VTX_CONFIG = 89,           // in message          Set vtx settings - betaflight
    MSP_VTXTABLE_BAND = 137,           // out message         vtxTable band/channel data
    MSP_SET_VTXTABLE_BAND = 227,       // in message          set vtxTable band/channel data (one band at a time)
    MSP_VTXTABLE_POWERLEVEL = 138,     // out message         vtxTable powerLevel data
    MSP_SET_VTXTABLE_POWERLEVEL = 228, // in message          set vtxTable powerLevel data (one powerLevel at a time)
    MSP_EEPROM_WRITE = 250,            // in message          no param
    MSP_REBOOT = 68,                   // in message reboot settings
};

typedef enum
{
    CHECK_FREQ_TABLE = 0,
    CHECK_POWER_LEVELS,
    CHECK_BANDS,
    SEND_EEPROM_WRITE,
    MONITORING,
    MSP_STATE_MAX
} mspState_e;

enum
{
    MSP_SYNC_DOLLAR = 0,
    MSP_SYNC_X,
    MSP_TYPE,
    MSP_FLAG,
    MSP_FUNCTION,
    MSP_PAYLOAD_SIZE,
    MSP_PAYLOAD,
    MSP_CRC,
};

// https://github.com/betaflight/betaflight/blob/master/src/main/msp/msp.c#L1949
typedef struct
{
    uint8_t vtxType;
    uint8_t band;
    uint8_t channel;
    uint8_t power;
    uint8_t pitmode;
    // uint16_t freq; // This doesnt work and bytes are missing after memcpy.
    uint8_t freqLSB;
    uint8_t freqMSB;
    uint8_t deviceIsReady;
    uint8_t lowPowerDisarm;
    // uint16_t pitModeFreq; // This doesnt work and bytes are missing after memcpy.
    uint8_t pitModeFreqLSB;
    uint8_t pitModeFreqMSB;
    uint8_t vtxTableAvailable;
    uint8_t bands;
    uint8_t channels;
    uint8_t powerLevels;
} mspVtxConfigStruct;

extern Serial serial_1;
extern Radio radio;
extern Led led_b;

static mspVtxConfigStruct in_mspVtxConfigStruct;
static uint32_t nextFlightControllerQueryTime = 0;
static mspState_e mspState = CHECK_FREQ_TABLE;
static uint8_t checkingIndex = 0;

static uint8_t state, in_idx, in_CRC;
static MspFunction in_Function;
static uint16_t in_PayloadSize, in_Type;

static uint8_t rxPacket[64];
static uint8_t txPacket[64];

static Timeout timeout(PROTOCOL_TIMEOUT);

uint8_t mspCalcCrc(uint8_t crc, unsigned char a)
{
    crc ^= a;
    for (int ii = 0; ii < 8; ++ii)
    {
        if (crc & 0x80)
        {
            crc = (crc << 1) ^ 0xD5;
        }
        else
        {
            crc = crc << 1;
        }
    }
    return crc;
}

void mspReset(void)
{
    state = MSP_SYNC_DOLLAR;
    mspState = CHECK_FREQ_TABLE;
    in_idx = 0;
    timeout.set(700);
}

static void mspSendPacket(uint8_t len)
{
    sleep_ms(2); // Flight Controller needs a bit time to swap TX to RX state
    serial_1.send(txPacket, len);
}

static void mspCreateHeader(void)
{
    memset(txPacket, 0, sizeof(txPacket));
    txPacket[0] = '$';
    txPacket[1] = 'X';
    txPacket[2] = '<';
    txPacket[3] = 0;
}

static void mspSendSimpleRequest(uint16_t opCode)
{
    mspCreateHeader();

    txPacket[4] = opCode & 0xFF;
    txPacket[5] = (opCode >> 8) & 0xFF;

    txPacket[6] = 0; // PayloadSize LSB
    txPacket[7] = 0; // PayloadSize MSB

    uint8_t crc = 0;
    for (int i = 3; i < 8; i++)
    {
        crc = mspCalcCrc(crc, txPacket[i]);
    }

    txPacket[MSP_HEADER_SIZE] = crc;

    mspSendPacket(MSP_HEADER_SIZE + 1);
}

static void sendEepromWrite()
{
    mspSendSimpleRequest(MSP_EEPROM_WRITE);
    mspState = MONITORING;
}

static void setVtxTableBand(uint8_t band)
{
    uint16_t payloadSize = 29;

    mspCreateHeader();

    txPacket[4] = MSP_SET_VTXTABLE_BAND & 0xFF;
    txPacket[5] = (MSP_SET_VTXTABLE_BAND >> 8) & 0xFF;

    txPacket[6] = payloadSize & 0xFF;
    txPacket[7] = (payloadSize >> 8) & 0xFF;

    txPacket[8] = band;
    txPacket[9] = BAND_NAME_LENGTH;

    for (uint8_t i = 0; i < CHANNEL_COUNT; i++)
    {
        txPacket[10 + i] = channelFreqLabelByIdx((band - 1) * CHANNEL_COUNT + i);
    }

    txPacket[10 + CHANNEL_COUNT] = getBandLetterByIdx(band - 1);
    txPacket[11 + CHANNEL_COUNT] = IS_FACTORY_BAND;
    txPacket[12 + CHANNEL_COUNT] = CHANNEL_COUNT;

    int i;
    for (i = 0; i < CHANNEL_COUNT; i++)
    {
        txPacket[(13 + CHANNEL_COUNT) + (i * 2)] = getFreqByIdx(((band - 1) * CHANNEL_COUNT) + i) & 0xFF;
        txPacket[(14 + CHANNEL_COUNT) + (i * 2)] = (getFreqByIdx(((band - 1) * CHANNEL_COUNT) + i) >> 8) & 0xFF;
    }

    uint8_t crc = 0;
    for (i = 3; i < MSP_HEADER_SIZE + payloadSize; i++)
    {
        crc = mspCalcCrc(crc, txPacket[i]);
    }

    txPacket[MSP_HEADER_SIZE + payloadSize] = crc;

    mspSendPacket(MSP_HEADER_SIZE + payloadSize + 1);
}

static void queryVtxTableBand(uint8_t idx)
{
    uint16_t payloadSize = 1;

    mspCreateHeader();

    txPacket[4] = MSP_VTXTABLE_BAND & 0xFF;
    txPacket[5] = (MSP_VTXTABLE_BAND >> 8) & 0xFF;

    txPacket[6] = payloadSize & 0xFF;
    txPacket[7] = (payloadSize >> 8) & 0xFF;

    txPacket[8] = idx; // get power array entry

    uint8_t crc = 0;
    for (int i = 3; i < MSP_HEADER_SIZE + payloadSize; i++)
    {
        crc = mspCalcCrc(crc, txPacket[i]);
    }

    txPacket[MSP_HEADER_SIZE + payloadSize] = crc;

    mspSendPacket(MSP_HEADER_SIZE + payloadSize + 1);
}

static void setVtxTablePowerLevel(uint8_t idx)
{
    uint16_t payloadSize = 7;

    mspCreateHeader();

    txPacket[4] = MSP_SET_VTXTABLE_POWERLEVEL & 0xFF;
    txPacket[5] = (MSP_SET_VTXTABLE_POWERLEVEL >> 8) & 0xFF;

    txPacket[6] = payloadSize & 0xFF;
    txPacket[7] = (payloadSize >> 8) & 0xFF;

    txPacket[8] = idx;
    txPacket[9] = saPowerLevelsLut[idx - 1] & 0xFF;         // powerValue LSB
    txPacket[10] = (saPowerLevelsLut[idx - 1] >> 8) & 0xFF; // powerValue MSB
    txPacket[11] = POWER_LEVEL_LABEL_LENGTH;
    txPacket[12] = saPowerLevelsLabel[((idx - 1) * POWER_LEVEL_LABEL_LENGTH) + 0];
    txPacket[13] = saPowerLevelsLabel[((idx - 1) * POWER_LEVEL_LABEL_LENGTH) + 1];
    txPacket[14] = saPowerLevelsLabel[((idx - 1) * POWER_LEVEL_LABEL_LENGTH) + 2];

    uint8_t crc = 0;
    for (int i = 3; i < MSP_HEADER_SIZE + payloadSize; i++)
    {
        crc = mspCalcCrc(crc, txPacket[i]);
    }

    txPacket[MSP_HEADER_SIZE + payloadSize] = crc;

    mspSendPacket(MSP_HEADER_SIZE + payloadSize + 1);
}

static void queryVtxTablePowerLevel(uint8_t idx)
{
    uint16_t payloadSize = 1;

    mspCreateHeader();

    txPacket[4] = MSP_VTXTABLE_POWERLEVEL & 0xFF;
    txPacket[5] = (MSP_VTXTABLE_POWERLEVEL >> 8) & 0xFF;

    txPacket[6] = payloadSize & 0xFF;
    txPacket[7] = (payloadSize >> 8) & 0xFF;

    txPacket[8] = idx + 1;

    uint8_t crc = 0;
    for (int i = 3; i < MSP_HEADER_SIZE + payloadSize; i++)
    {
        crc = mspCalcCrc(crc, txPacket[i]);
    }

    txPacket[MSP_HEADER_SIZE + payloadSize] = crc;

    mspSendPacket(MSP_HEADER_SIZE + payloadSize + 1);
}

static void sendLocalVtxTable(void)
{
    uint16_t payloadSize = 15;
    uint8_t band = g_config.channel / getFreqTableChannels();
    uint8_t channel = g_config.channel % getFreqTableChannels();
    uint8_t power_idx = getIdxByPower(g_config.currPowerdB) + 1;

    mspCreateHeader();

    txPacket[4] = MSP_SET_VTX_CONFIG & 0xFF;
    txPacket[5] = (MSP_SET_VTX_CONFIG >> 8) & 0xFF;

    txPacket[6] = payloadSize & 0xFF;
    txPacket[7] = (payloadSize >> 8) & 0xFF;

    txPacket[8] = 0;                            // idx LSB
    txPacket[9] = 0;                            // idx MSB
    txPacket[10] = power_idx;                   // Power idx, 1 origin
    txPacket[11] = g_config.pitMode;            // pitmode
    txPacket[12] = 0;                           // lowPowerDisarm
    txPacket[13] = g_config.pitmodeFreq & 0xff; // pitModeFreq LSB
    txPacket[14] = g_config.pitmodeFreq >> 8;   // pitModeFreq MSB
    txPacket[15] = band + 1;                    // newBand - 1 origin
    txPacket[16] = channel + 1;                 // newChannel - 1 origin
    txPacket[17] = g_config.currFreq & 0xff;    // newFreq  LSB
    txPacket[18] = g_config.currFreq >> 8;      // newFreq  MSB
    txPacket[19] = getFreqTableBands();         // newBandCount
    txPacket[20] = getFreqTableChannels();      // newChannelCount
    txPacket[21] = SA_NUM_POWER_LEVELS;         // newPowerCount
    txPacket[22] = 1;                           // vtxtable should be cleared

    uint8_t crc = 0;
    for (int i = 3; i < MSP_HEADER_SIZE + payloadSize; i++)
    {
        crc = mspCalcCrc(crc, txPacket[i]);
    }

    txPacket[MSP_HEADER_SIZE + payloadSize] = crc;

    mspSendPacket(MSP_HEADER_SIZE + payloadSize + 1);
}

static void set_config(const mspVtxConfigStruct &mspVtxConfig)
{
    //  Store initially received values.  If the VTx Table is correct, only then set these values.  //
    g_config.pitMode = mspVtxConfig.pitmode;

    uint8_t power_idx = mspVtxConfig.power;
    power_idx -= 1;                  // Correct for BF starting at 1.
    if (mspVtxConfig.lowPowerDisarm) // Force on boot because BF doesnt send a low power index.
        power_idx = 0;

    g_config.pitmodeFreq = (mspVtxConfig.pitModeFreqMSB << 8) + mspVtxConfig.pitModeFreqLSB;

    if (power_idx < SA_NUM_POWER_LEVELS)
        g_config.currPowerdB = saPowerLevelsLut[power_idx];
    else
        g_config.currPowerdB = 0; // 1mW

    uint16_t freq_to_set = 0;
    uint16_t power_to_set = 0;
    uint8_t band = mspVtxConfig.band;
    uint8_t channel = mspVtxConfig.channel;
    if (band == 0 || channel == 0)
    {
        g_config.freqMode = 1;
        g_config.currFreq = (mspVtxConfig.freqMSB << 8) + mspVtxConfig.freqLSB;
        freq_to_set = g_config.currFreq;
        radio.set_freq(g_config.currFreq);
    }
    else
    {
        channel = ((mspVtxConfig.band - 1) * 8) + (mspVtxConfig.channel - 1);
        if (channel >= getFreqTableSize())
            channel = 27; // F4 5800MHz
        g_config.channel = channel;
        g_config.freqMode = 0;
        freq_to_set = getFreqByIdx(channel);
    }
    g_config.pitMode = mspVtxConfig.pitmode;
    if (g_config.pitMode)
        power_to_set = RACE_MODE_POWER;
    else
    {
        power_to_set = g_config.currPowerdB;
        freq_to_set = (mspVtxConfig.pitModeFreqMSB << 8) + mspVtxConfig.pitModeFreqMSB;
    }
    g_config.save();

    // Set power before freq changes to prevent PLL settling issues and spamming other frequencies.
    radio.set_power(power_to_set);
    radio.set_freq(freq_to_set);
}

static void mspProcessPacket(void)
{
    uint16_t value;
    uint8_t bandNameLength, bandLetter, bandFactory, bandLength;
    uint8_t frequenciesCorrect = 1;

    switch (in_Function)
    {
    case MSP_VTX_CONFIG:
        memcpy(&in_mspVtxConfigStruct, rxPacket + 8, in_PayloadSize);

        switch (mspState)
        {
        case CHECK_FREQ_TABLE:
            if (in_mspVtxConfigStruct.bands == getFreqTableBands() &&
                in_mspVtxConfigStruct.channels == getFreqTableChannels() &&
                in_mspVtxConfigStruct.powerLevels == SA_NUM_POWER_LEVELS)
            {
                mspState = CHECK_POWER_LEVELS;
                nextFlightControllerQueryTime = time_ms();
                break;
            }
            sendLocalVtxTable();
            break;
        case MONITORING:
            set_config(in_mspVtxConfigStruct);
            break;
        default:
            break;
        }
        break;

    case MSP_VTXTABLE_POWERLEVEL:
        value = ((uint16_t)rxPacket[10] << 8) + rxPacket[9];
        if (value == saPowerLevelsLut[checkingIndex] && rxPacket[11] == POWER_LEVEL_LABEL_LENGTH) // Check lengths before trying to check content
        {
            if (rxPacket[12] == saPowerLevelsLabel[checkingIndex * POWER_LEVEL_LABEL_LENGTH + 0] &&
                rxPacket[13] == saPowerLevelsLabel[checkingIndex * POWER_LEVEL_LABEL_LENGTH + 1] &&
                rxPacket[14] == saPowerLevelsLabel[checkingIndex * POWER_LEVEL_LABEL_LENGTH + 2])
            {
                checkingIndex++;
                if (checkingIndex > SA_NUM_POWER_LEVELS - 1)
                {
                    checkingIndex = 0;
                    mspState = CHECK_BANDS;
                }
                nextFlightControllerQueryTime = time_ms();
                break;
            }
        }
        setVtxTablePowerLevel(checkingIndex + 1);
        break;

    case MSP_VTXTABLE_BAND:
        bandNameLength = rxPacket[9];
        bandLetter = rxPacket[10 + bandNameLength];
        bandFactory = rxPacket[11 + bandNameLength];
        bandLength = rxPacket[12 + bandNameLength];
        if (bandNameLength == BAND_NAME_LENGTH && bandLetter == getBandLetterByIdx(checkingIndex) && bandFactory == IS_FACTORY_BAND && bandLength == CHANNEL_COUNT) // Check lengths before trying to check content
        {
            if (rxPacket[10] == channelFreqLabelByIdx(checkingIndex * CHANNEL_COUNT + 0) &&
                rxPacket[11] == channelFreqLabelByIdx(checkingIndex * CHANNEL_COUNT + 1) &&
                rxPacket[12] == channelFreqLabelByIdx(checkingIndex * CHANNEL_COUNT + 2) &&
                rxPacket[13] == channelFreqLabelByIdx(checkingIndex * CHANNEL_COUNT + 3) &&
                rxPacket[14] == channelFreqLabelByIdx(checkingIndex * CHANNEL_COUNT + 4) &&
                rxPacket[15] == channelFreqLabelByIdx(checkingIndex * CHANNEL_COUNT + 5) &&
                rxPacket[16] == channelFreqLabelByIdx(checkingIndex * CHANNEL_COUNT + 6) &&
                rxPacket[17] == channelFreqLabelByIdx(checkingIndex * CHANNEL_COUNT + 7))
            {
                for (uint8_t i = 0; i < 8; i++)
                {
                    value = ((uint16_t)rxPacket[22 + (2 * i)] << 8) + rxPacket[21 + (2 * i)];
                    if (value != getFreqByIdx(checkingIndex * CHANNEL_COUNT + i))
                        frequenciesCorrect = 0;
                }

                if (frequenciesCorrect)
                {
                    checkingIndex++;
                    if (checkingIndex > getFreqTableBands() - 1)
                    {
                        mspState = MONITORING;
                        set_config(in_mspVtxConfigStruct);
                    }
                    nextFlightControllerQueryTime = time_ms();
                    break;
                }
            }
        }
        setVtxTableBand(checkingIndex + 1);
        break;

    case MSP_SET_VTX_CONFIG:
        mspState = SEND_EEPROM_WRITE;
        nextFlightControllerQueryTime = time_ms();
        break;

    case MSP_SET_VTXTABLE_BAND:
    case MSP_SET_VTXTABLE_POWERLEVEL:
        nextFlightControllerQueryTime = time_ms();
        break;

    case MSP_EEPROM_WRITE:
        set_config(in_mspVtxConfigStruct);
        mspState = MONITORING;
        break;
    case MSP_REBOOT:
        reboot_into_bootloader(9600);
        break;
    }
}

int mspProcessSerial(void)
{
    int res = PROTOCOL_LISTENING;

    uint8_t data;
    int rd_sz = serial_1.recv(&data, 1);
    if (rd_sz == 0)
        return PROTOCOL_LISTENING;

    rxPacket[in_idx] = data;
    debug_r("%02X ", data);

    switch (state)
    {
    case MSP_SYNC_DOLLAR:
        if (data == MSP_HEADER_DOLLAR)
            state = MSP_SYNC_X;
        else
            state = MSP_SYNC_DOLLAR;
        break;
    case MSP_SYNC_X:
        if (data == MSP_HEADER_X)
        {
            state = MSP_TYPE;
#if LED_INDICATION_OF_VTX_MODE == 0
            led_b.set(1); // Got header so turn on incoming packet LED.
#endif
        }
        else
            state = MSP_SYNC_DOLLAR;
        break;
    case MSP_TYPE:
        if (data == MSP_HEADER_REQUEST || data == MSP_HEADER_RESPONSE || data == MSP_HEADER_ERROR)
        {
            in_Type = data;
            state = MSP_FLAG;
            in_CRC = 0;
        }
        else
            state = MSP_SYNC_DOLLAR;
        break;
    case MSP_FLAG:
        in_CRC = mspCalcCrc(in_CRC, data);
        state = MSP_FUNCTION;
        break;
    case MSP_FUNCTION:
        in_CRC = mspCalcCrc(in_CRC, data);
        if (in_idx == 4)
        {
            in_Function = (MspFunction)(((uint16_t)rxPacket[5] << 8) | rxPacket[4]);
            state = MSP_PAYLOAD_SIZE;
        }
        break;
    case MSP_PAYLOAD_SIZE:
        in_CRC = mspCalcCrc(in_CRC, data);
        if (in_idx == 6)
        {
            in_PayloadSize = ((uint16_t)rxPacket[7] << 8) | rxPacket[6];
            state = MSP_PAYLOAD;
        }
        break;
    case MSP_PAYLOAD:
        in_CRC = mspCalcCrc(in_CRC, data);
        if (in_idx == (7 + in_PayloadSize))
            state = MSP_CRC;
        break;
    case MSP_CRC:
        if (in_CRC == data)
        {
            res = PROTOCOL_OK;
            if (in_Type != MSP_HEADER_ERROR)
                mspProcessPacket();
        }

        state = MSP_SYNC_DOLLAR;
        break;
    default:
        state = MSP_SYNC_DOLLAR;
        break;
    }

#if LED_INDICATION_OF_VTX_MODE == 0
    if (state == MSP_SYNC_DOLLAR)
        led_b.set(0);
#endif

    if (state == MSP_SYNC_DOLLAR)
    {
        in_idx = 0;
        if (res != PROTOCOL_OK)
            res = PROTOCOL_ERROR;
    }
    else
    {
        in_idx++;
    }

    return res;
}

int mspUpdate(void)
{
    uint32_t now = time_ms();
    int res = mspProcessSerial();

    if (timeout.check())
        res = PROTOCOL_TIMEOUT;

    if (res == PROTOCOL_OK)
        timeout.set(TIMEOUT_NEVER);

    if (now < nextFlightControllerQueryTime)
    {
        return res;
    }

    nextFlightControllerQueryTime = now + FC_QUERY_PERIOD_MS; // Wait for any reply.

    switch (mspState)
    {
    case CHECK_FREQ_TABLE:
        mspSendSimpleRequest(MSP_VTX_CONFIG);
        break;
    case CHECK_POWER_LEVELS:
        queryVtxTablePowerLevel(checkingIndex);
        break;
    case CHECK_BANDS:
        queryVtxTableBand(checkingIndex + 1);
        break;
    case SEND_EEPROM_WRITE:
        sendEepromWrite();
        break;
    case MONITORING:
        break;
    default:
        assert(false);
        break;
    }

    return res;
}
