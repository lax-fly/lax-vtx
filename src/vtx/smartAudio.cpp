#include "smartAudio.h"
#include "table.h"
#include "Config.h"
#include "radio.h"
#include "helpers.h"
#include "main.h"
#include "protocol.h"

/**** SmartAudio definitions ****/
#define CRC_LEN 1
#define LEGHT_CALC(len) (sizeof(sa_header_t) + CRC_LEN + (len))

#define PIT_MODE_FREQ_GET (0x1 << 14)
#define PIT_MODE_FREQ_SET (0x1 << 15)

#define SA_SYNC_BYTE 0xAA
#define SA_HEADER_BYTE 0x55
#define SA_NUM_POWER_LEVELS 8 // Max 8 for INAV.
#define RESERVE_BYTE 0x01

// SmartAudio command and response codes
enum
{
    SA_CMD_NONE = 0x00,
    SA_CMD_GET_SETTINGS = 0x01,
    SA_CMD_SET_POWER,
    SA_CMD_SET_CHAN,
    SA_CMD_SET_FREQ,
    SA_CMD_SET_MODE,
    SA_CMD_GET_SETTINGS_V2 = 0x09, // Response only
    SA_CMD_GET_SETTINGS_V21 = 0x11,// should be actually 0x10 in the SA protocol, but betaflight uses 0x11, and I don't know why

    /* Internal custom commands */
    SA_CMD_BOOTLOADER = 0x78,
};

typedef struct
{
    uint8_t sync;
    uint8_t header;
    uint8_t command;
    uint8_t length;
} sa_header_t;

typedef struct
{
    uint8_t channel;
    uint8_t power;
    uint8_t operationMode;
    uint8_t frequency[2];
    uint8_t rawPowerValue;
    uint8_t num_pwr_levels;
    uint8_t levels[SA_NUM_POWER_LEVELS];
} sa_settings_resp_t;

typedef struct
{
    uint8_t data_u8;
    uint8_t reserved;
} sa_u8_resp_t;

typedef struct
{
    uint8_t data_u16[2];
    uint8_t reserved;
} sa_u16_resp_t;

enum
{
    SA_SYNC = 0,
    SA_HEADER,
    SA_COMMAND,
    SA_LENGTH,
    SA_DATA,
    SA_CRC,
};

extern Serial serial_1;
extern Radio radio;
extern Led led_b;

static uint8_t state = SA_SYNC, in_idx, in_len;
static uint8_t rxPacket[64];
static uint8_t txPacket[64];

uint8_t *fill_resp_header(uint8_t cmd, uint8_t len)
{
    sa_header_t *hdr = (sa_header_t *)txPacket;
    hdr->sync = SA_SYNC_BYTE;
    hdr->header = SA_HEADER_BYTE;
    hdr->command = cmd;
    hdr->length = len;
    return &txPacket[sizeof(sa_header_t)];
}

// https://github.com/betaflight/betaflight/blob/287741b816fb5bdac1f72a825846303454765fac/src/main/io/vtx_smartaudio.c#L152
uint8_t smartadioCalcCrc(const uint8_t *data, uint8_t len)
{
#define POLYGEN 0xd5
    uint8_t crc = 0;
    uint_fast8_t ii;

    while (len--)
    {
        crc ^= *data++;
        ii = 8;
        while (ii--)
        {
            if ((crc & 0x80) != 0)
                crc = (crc << 1) ^ POLYGEN;
            else
                crc <<= 1;
        }
    }
    return crc;
}

static Timeout timeout(500);

void smartaudioReset(void)
{
    state = SA_SYNC;
    in_idx = 0;
    timeout.set(500);
}

void smartaudioSendPacket(void)
{
    sa_header_t *hdr = (sa_header_t *)txPacket;
    uint8_t len = hdr->length + sizeof(sa_header_t);
    txPacket[len] = smartadioCalcCrc(
        (uint8_t *)&hdr->command,
        (len - offsetof(sa_header_t, command)));
    len += CRC_LEN;
    // Flight Controller needs a bit time to swap TX to RX state
    sleep_ms(5);
    serial_1.send(txPacket, len);
}

void smartaudioBuildSettingsPacket(void)
{
    sa_settings_resp_t *payload =
        (sa_settings_resp_t *)fill_resp_header(
            SA_CMD_GET_SETTINGS_V21, sizeof(sa_settings_resp_t));
    uint8_t operationMode = 0;
    bitWrite(operationMode, 0, g_config.freqMode);
    bitWrite(operationMode, 1, g_config.pitMode);
    bitWrite(operationMode, 2, g_config.pitmodeInRange);
    bitWrite(operationMode, 3, g_config.pitmodeOutRange);
    bitWrite(operationMode, 4, g_config.unlocked);

    payload->channel = g_config.channel;
    payload->power = 0; // Fake index to allow setting any power level
    payload->operationMode = operationMode;
    payload->frequency[0] = (uint8_t)(g_config.currFreq >> 8);
    payload->frequency[1] = (uint8_t)(g_config.currFreq);
    payload->rawPowerValue = g_config.currPowerdB;
    payload->num_pwr_levels = ARRAY_SIZE(saPowerLevelsLut);
    for (uint8_t i = 0; i < payload->num_pwr_levels; i++)
        payload->levels[i] = saPowerLevelsLut[i];

    smartaudioSendPacket();
}

void smartaudioProcessFrequencyPacket(void)
{
    sa_u16_resp_t *payload =
        (sa_u16_resp_t *)fill_resp_header(
            SA_CMD_SET_FREQ, sizeof(sa_u16_resp_t));
    uint16_t freq = rxPacket[4];
    freq <<= 8;
    freq |= rxPacket[5];

    if (freq & PIT_MODE_FREQ_GET)
    {
        // POR is not supported in SA2.1 so return currFreq
        freq = g_config.currFreq;
    }
    else if (freq & PIT_MODE_FREQ_SET)
    {
        // POR is not supported in SA2.1 so do not set a POR freq
        freq = g_config.currFreq;
    }
    else
    {
        radio.set_freq(freq);
        g_config.currFreq = freq;
        g_config.freqMode = 1;
        g_config.save();
    }

    payload->data_u16[0] = (uint8_t)(freq >> 8);
    payload->data_u16[1] = (uint8_t)(freq);
    payload->reserved = RESERVE_BYTE;

    smartaudioSendPacket();
}

void smartaudioProcessChannelPacket(void)
{
    sa_u8_resp_t *payload =
        (sa_u8_resp_t *)fill_resp_header(
            SA_CMD_SET_CHAN, sizeof(sa_u8_resp_t));
    uint8_t channel = rxPacket[4];

    if (channel < getFreqTableSize())
    {
        g_config.channel = channel;
        radio.set_freq(getFreqByIdx(channel));
        g_config.freqMode = 0;
        g_config.save();
    }
    else
    {
        channel = g_config.channel;
    }
    payload->data_u8 = channel;
    payload->reserved = RESERVE_BYTE;

    smartaudioSendPacket();
}

void smartaudioProcessPowerPacket(void)
{
    sa_u8_resp_t *payload =
        (sa_u8_resp_t *)fill_resp_header(
            SA_CMD_SET_POWER, sizeof(sa_u8_resp_t));
    uint8_t data = rxPacket[4];
    /* SA2.1 sets the MSB to indicate power is in dB.
     * Set MSB to zero and currPower will now be in dB. */
    uint8_t const value_in_dbm = data & 0x80;
    data &= 0x7F;
    /* Make sure the received power index (SA2.0) is valid if not in dBm
     *    Some firmwares are using indices even for SA2.1 so make sure it is
     *    handled here as well (when MSB is not set)
     */
    if (!value_in_dbm)
    {
        /* Force into pitmode if a invalid index is received */
        data = (data < ARRAY_SIZE(saPowerLevelsLut)) ? saPowerLevelsLut[data] : 0;
    }

    if (!data)
    {
        /* 0dB is pit mode enable */
        g_config.pitMode = value_in_dbm ? 1 : 0; // Do not set pitmode for a 0mW.  INav sends this command on boot for some reason. https://github.com/iNavFlight/inav/issues/6976
    }

    if (g_config.pitMode)
        radio.set_power(RACE_MODE_POWER);
    else
        radio.set_power(data);

    g_config.currPowerdB = data;
    payload->data_u8 = value_in_dbm ? g_config.currPowerdB : getIdxByPower(g_config.currPowerdB);
    payload->reserved = RESERVE_BYTE;

    smartaudioSendPacket();
}

void smartaudioProcessModePacket(void)
{
    sa_u8_resp_t *payload =
        (sa_u8_resp_t *)fill_resp_header(
            SA_CMD_SET_MODE, sizeof(sa_u8_resp_t));
    uint8_t data = rxPacket[4];

    uint8_t previousPitmode = g_config.pitMode;

    // Set PIR and POR. POR is no longer used in SA2.1 and is treated like PIR
    g_config.pitmodeInRange = bitRead(data, 0);
    g_config.pitmodeOutRange = bitRead(data, 1);
    g_config.pitMode = bitRead(data, 2) ? 0 : g_config.pitmodeInRange || g_config.pitmodeOutRange;
    g_config.unlocked = bitRead(data, 3);

    // When turning on pitmode
    if (previousPitmode < g_config.pitMode)
    {
        /* Enable pitmode */
        if (g_config.pitmodeOutRange)   // make compatibility for SA2.0
        {
            radio.set_freq(5584);   // solidly use 5584MHz in pitmodeOutRange mode
        }
        radio.set_power(RACE_MODE_POWER);
    }

    g_config.save();

    payload->data_u8 = data;
    payload->reserved = RESERVE_BYTE;

    smartaudioSendPacket();
}

int smartaudioProcessSerial(void)
{
    int res = PROTOCOL_LISTENING;

    if (timeout.check())
        return PROTOCOL_TIMEOUT;

    uint8_t data;
    int rd_sz = serial_1.recv(&data, 1);
    if (rd_sz == 0)
        return res;

    debug_r("%02X ", data);

    rxPacket[in_idx] = data;

    switch (state)
    {
    case SA_SYNC:
        if (data == SA_SYNC_BYTE)
            state = SA_HEADER;
        break;
    case SA_HEADER:
        if (data == SA_HEADER_BYTE)
            state = SA_COMMAND;
        else
            state = SA_SYNC;
        break;
    case SA_COMMAND:
        state = SA_LENGTH;
        break;
    case SA_LENGTH:
        state = data ? SA_DATA : SA_CRC;
        in_len = in_idx + 1 + data;
        break;
    case SA_DATA:
        state = (in_idx + 1 < in_len) ? SA_DATA : SA_CRC;
        break;
    case SA_CRC:
        // CRC check and packet processing
        if (smartadioCalcCrc(rxPacket, in_len) == data)
        {
            if (!LED_INDICATION_OF_VTX_MODE)
                led_b.set(1);

            switch (rxPacket[2] >> 1) // Commands
            {
            case SA_CMD_GET_SETTINGS: // 0x03
                smartaudioBuildSettingsPacket();
                break;
            case SA_CMD_SET_POWER: // 0x05
                smartaudioProcessPowerPacket();
                break;
            case SA_CMD_SET_CHAN: // 0x07
                smartaudioProcessChannelPacket();
                break;
            case SA_CMD_SET_FREQ: // 0x09
                smartaudioProcessFrequencyPacket();
                break;
            case SA_CMD_SET_MODE: // 0x0B   Set mode is supported by SmartAudio V2 only!
                smartaudioProcessModePacket();
                break;
            case SA_CMD_BOOTLOADER:
                if (rxPacket[4] == 'R' && rxPacket[5] == 'S' && rxPacket[6] == 'T')
                    reboot_into_bootloader(4801); // LSB represents stopbits. 0 = 1 stopbit, 1 = 2 stopbit.  SA passthrough requires 2 stopbits.
                break;
            }
            if (!LED_INDICATION_OF_VTX_MODE)
                led_b.set(0);

            timeout.set(TIMEOUT_NEVER);
            res = PROTOCOL_OK;
        }
        state = SA_SYNC;
        break;
    }

    if (state == SA_SYNC)
    {
        in_idx = 0; // Restart
        if (res != PROTOCOL_OK)
            res = PROTOCOL_ERROR;
    }
    else
        in_idx++;

    return res;
}
