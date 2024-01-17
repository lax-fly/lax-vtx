#include "smartAudio.h"
#include "mspVtx.h"
#include "tramp.h"
#include "table.h"
#include "Config.h"
#include "main.h"
#include "protocol.h"
#include "errorCodes.h"
#include "modeindicator.h"

namespace Protocol
{
    uint8_t frame_err;
    vtxMode_e mode = BUTTON;
    void b1_short_press_cb(void *);
    void b1_long_press_cb(void *);
    void b1_long10s_press_cb(void *);
};

void Protocol::b1_short_press_cb(void *)
{
    if (g_config.vtxMode != BUTTON)
        return;
    g_config.freqMode = 0;
    g_config.channel = (g_config.channel + 1) % getFreqTableSize();
    radio.set_freq(getFreqByIdx(g_config.channel));
}

void Protocol::b1_long_press_cb(void *)
{
    if (g_config.vtxMode != BUTTON)
        return;
    uint8_t idx = 0;
    for (idx = 0; idx < SA_NUM_POWER_LEVELS; idx++)
    {
        if (g_config.currPowerdB == saPowerLevelsLut[idx])
            break;
    }
    g_config.currPowerdB = saPowerLevelsLut[(idx + 1) % SA_NUM_POWER_LEVELS];
    radio.set_power(g_config.currPowerdB);
}

void Protocol::b1_long10s_press_cb(void *)
{
    if (g_config.vtxMode != BUTTON)
        return;
    g_config.loaddefault();
    radio.set_power(getFreqByIdx(g_config.channel));
}

void Protocol::init(vtxMode_e mode)
{
    uint32_t baud, stopbits;

    serial_1.close();
    switch (mode)
    {
    case TRAMP:
        baud = TRAMP_BAUD;
        stopbits = 1;
        trampReset();
        break;
    case SMARTAUDIO:
        baud = SMARTAUDIO_BAUD;
        stopbits = 2;
        smartaudioReset();
        break;
    case MSP: // search and use 'openvtx' preset in betaflight configurator
        baud = MSP_BAUD;
        stopbits = 1;
        mspReset();
        break;
    default:
        baud = 9600;
        stopbits = 1;
        break;
    }
    serial_1.open(DEV_ID_USART1, baud, stopbits);
    button_1.open(DEV_ID_BUTTON1);
    button_1.register_event_callback(Button::SHORT_PRESS, b1_short_press_cb, NULL);
    button_1.register_event_callback(Button::LONG_PRESS_1S, b1_long_press_cb, NULL);
    button_1.register_event_callback(Button::LONG_PRESS_10S, b1_long10s_press_cb, NULL);
}

void Protocol::poll(void)
{
    int res = PROTOCOL_ERROR;

    switch (mode)
    {
    case TRAMP:
        res = trampProcessSerial();
        break;
    case SMARTAUDIO:
        res = smartaudioProcessSerial();
        break;
    case MSP:
        res = mspUpdate();
        break;
    default:
        break;
    }

    switch (res)
    {
    case PROTOCOL_LISTENING:
        break;
    case PROTOCOL_TIMEOUT:
    case PROTOCOL_ERROR:
        if (res == PROTOCOL_TIMEOUT || ++frame_err >= 16) // frame err or timeout err over 3 times
        {
            led_r.blink(1, 50);
            frame_err = 0; // avoid overflow
            mode = (vtxMode_e)((mode + 1) % VTX_MODE_MAX);
            Protocol::init((vtxMode_e)mode);
            g_config.vtxMode = BUTTON;
        }
        break;
    case PROTOCOL_OK:
        frame_err = 0;
        if (mode != g_config.vtxMode)
        {
            g_config.vtxMode = mode;
            g_config.save();
        }
        break;
    }

    // errorCheck();

    if (LED_INDICATION_OF_VTX_MODE &&
        g_config.vtxMode != TRAMP) // TRAMP doesnt use VTx Tables so LED indication of band/channel doesnt really work.
    {
        modeIndicationLoop();
    }
    else
    {
        led_g.set(g_config.vtxMode == mode);
    }
}
