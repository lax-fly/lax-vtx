#pragma once

#include <stdint.h>
#include "table.h"

#define SET_MODE_LED(x) led_g.set(x)
#define SET_VALUE_LED(x) led_b.set(x)

#define LED_ON_DURATION 80
#define LED_OFF_DURATION 380
#define INTER_MODE_DELAY 500

typedef enum {
    IND_BAND,
    IND_CHANNEL,
    IND_POWER,
    IND_MAX
} indication_mode_type_t;

typedef enum {
    MODE,
    VALUE
} indication_mode_t;

typedef struct {
    indication_mode_t mode;
    indication_mode_type_t modeType;
    uint8_t blinksDone;
    uint32_t lastLedActionTime;
    uint8_t ledIsOn;
    uint32_t delay; 

    uint8_t channel;
    float currPowerdB;
} mode_indicator_state_t;

void modeIndicationLoop();
void resetModeIndication();
