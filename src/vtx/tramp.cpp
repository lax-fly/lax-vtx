#include "tramp.h"
#include "table.h"
#include "config.h"
#include "timer.h"
#include "main.h"
#include "string.h"
#include "protocol.h"

#define TRAMP_HEADER 0x0F // 15
#define TRAMP_MSG_SIZE 15
#define TRAMP_PROTOCOL_TIMEOUT 300 // ms

extern Serial serial_1;
extern Radio radio;
extern Led led_r, led_g, led_b;
extern Sensor sensor;

static uint8_t rxPacket[64];
static uint8_t txPacket[64];
bool tramp_alive = false;

struct tramp_msg
{
    uint8_t header;
    uint8_t cmd;
    uint8_t payload[12];
    uint8_t crc;
};

enum
{
    TRAMP_SYNC = 0,
    TRAMP_DATA,
    TRAMP_CRC,
};

static uint8_t state, in_idx;
static Timeout timeout(PROTOCOL_TIMEOUT);

uint8_t trampCalcCrc(uint8_t *packet)
{
    uint8_t crc = 0;

    for (int i = 1; i < (TRAMP_MSG_SIZE - 1); i++)
    {
        crc += packet[i];
    }

    return crc;
}

void trampReset(void)
{
    state = TRAMP_SYNC;
    in_idx = 0;
    timeout.set(1500);
}

void trampSendPacket(void)
{
    // Flight Controller needs a bit time to swap TX to RX state
    sleep_ms(2);

    txPacket[TRAMP_MSG_SIZE - 1] = trampCalcCrc(txPacket);
    txPacket[TRAMP_MSG_SIZE] = 0;
    serial_1.send(txPacket, (TRAMP_MSG_SIZE + 1), 1);
}

void trampBuildrPacket(void)
{
    uint32_t min_freq, max_freq, max_power;
    radio.get_freq_range(min_freq, max_freq);
    max_power = radio.get_max_power();
    memset(txPacket, 0, sizeof(txPacket));
    txPacket[0] = TRAMP_HEADER;
    txPacket[1] = 'r';
    txPacket[2] = min_freq & 0xff;
    txPacket[3] = (min_freq >> 8) & 0xff;
    txPacket[4] = max_freq & 0xff;
    txPacket[5] = (max_freq >> 8) & 0xff;
    txPacket[6] = max_power & 0xff;
    txPacket[7] = (max_power >> 8) & 0xff;
    trampSendPacket();
}

void trampBuildvPacket(void)
{
    uint16_t dbm = g_config.currPowerdB;

    memset(txPacket, 0, sizeof(txPacket));
    txPacket[0] = TRAMP_HEADER;
    txPacket[1] = 'v';
    txPacket[2] = g_config.currFreq & 0xff;         // freq
    txPacket[3] = (g_config.currFreq >> 8) & 0xff;
    txPacket[4] = dbm & 0xff;           // Configured transmitting power
    txPacket[5] = (dbm >> 8) & 0xff;    // Configured transmitting power
    txPacket[6] = 0;                    // trampControlMode, Currently only used for race lock
    txPacket[7] = g_config.pitMode;     // trampPitMode
    txPacket[8] = dbm & 0xff;           // Actual transmitting power
    txPacket[9] = (dbm >> 8) & 0xff;    // Actual transmitting power
    trampSendPacket();
}

void trampBuildsPacket(void)
{
    uint16_t temperature = sensor.get_mcu_temp(); // Dummy value.

    memset(txPacket, 0, sizeof(txPacket));
    txPacket[0] = TRAMP_HEADER;
    txPacket[1] = 's';
    txPacket[6] = temperature & 0xff;
    txPacket[7] = (temperature >> 8) & 0xff;
    trampSendPacket();
}

void trampProcessFPacket(void)
{
    uint32_t freq = rxPacket[3];
    freq <<= 8;
    freq |= rxPacket[2];
    radio.set_freq(freq);
    g_config.currFreq = freq;
    g_config.freqMode = 1;  // Tramp use direct freqence mode only
    g_config.save();
}

void trampProcessPPacket(void)
{
    uint16_t dbm = rxPacket[3];
    dbm <<= 8;
    dbm += rxPacket[2];

    radio.set_power(dbm);
    g_config.currPowerdB = dbm;
    g_config.save();
}

void trampProcessIPacket(void)
{
    g_config.pitMode = !rxPacket[2];

    if (g_config.pitMode)
        radio.set_power(RACE_MODE_POWER);
    else
        radio.set_power(g_config.currPowerdB);

    g_config.save();
}


int trampProcessSerial(void)
{
    if (timeout.check())
    {
        state = TRAMP_SYNC;
        return PROTOCOL_TIMEOUT;
    }

    int res = PROTOCOL_LISTENING;
    uint8_t data;
    int rd_sz = serial_1.recv(&data, 1);
    if (rd_sz == 0)
        return res;

    debug_r("%02X ", data);

    rxPacket[in_idx] = data;

    switch (state)
    {
    case TRAMP_SYNC:
        if (data == TRAMP_HEADER)
            state = TRAMP_DATA;
        break;
    case TRAMP_DATA:
        if ((TRAMP_MSG_SIZE - 2) <= in_idx)
            state = TRAMP_CRC;
        break;
    case TRAMP_CRC:
        if (data == trampCalcCrc(rxPacket))
        {
            led_b.set(1);

            switch (rxPacket[1]) // command
            {
            case 'F': // 0x50 - Freq - do not respond to this packet
                trampProcessFPacket();
                break;
            case 'P': // 0x50 - Power - do not respond to this packet
                trampProcessPPacket();
                break;
            case 'I': // 0x49 - Pitmode - do not respond to this packet. Pitmode is not remember on reboot for Tramp, but I have so that matches SA and is useful.
                trampProcessIPacket();
                break;
            case 'r': // 0x72 - Max min freq and power packet
                trampBuildrPacket();
                break;
            case 'v': // 0x76 - Verify
                trampBuildvPacket();
                break;
            case 's': // 0x73 - Temperature
                trampBuildsPacket();
                break;
            case 'R': // Reboot to bootloader
                if (rxPacket[2] == 'S' && rxPacket[3] == 'T')
                    reboot_into_bootloader(9600);
                break;
            default:
                break;
            }

            timeout.fresh();
            res = PROTOCOL_OK;
            led_b.set(0);
        }
        state = TRAMP_SYNC;
        break;
    }

    if (state == TRAMP_SYNC)
    {
        in_idx = 0;
        if (res != PROTOCOL_OK)
            res = PROTOCOL_ERROR;
    }
    else
        in_idx++;

    return res;
}
