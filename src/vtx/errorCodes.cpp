#include "errorCodes.h"
#include "helpers.h"
#include "main.h"

// LED blink period is in ms / 10
// Final index 250 is a 2.5s LED off period before repeating the cycle
static const uint8_t rtc6705NotDetectedime[8] = {100, 50, 10, 50, 10, 50, 10, 250};  // 1s on to indicate start of error code, followed by  3 x 0.1s short flashes.
static const uint8_t power5vNotDetectedime[6] = {100, 50, 10, 50, 10, 250};          // 1s on to indicate start of error code, followed by  2 x 0.1s short flashes.
static const uint8_t power3v3NotDetectedime[4] = {100, 50, 10, 250};                 // 1s on to indicate start of error code, followed by  1 x 0.1s short flashe.

static uint8_t currentErrorMode = NO_ERROR;
static uint8_t errorIndex;
static uint32_t errorTime;

void errorCheck(void)
{
    if (currentErrorMode == NO_ERROR)
    {
        led_r.blink(1,1000); // Default LED on and no error
        return;
    }

    uint32_t now = time_ms();
    if (now > errorTime)
    {
        errorIndex++;

        switch (currentErrorMode) {
            case RTC6705_NOT_DETECTED:
                errorTime = rtc6705NotDetectedime[errorIndex % ARRAY_SIZE(rtc6705NotDetectedime)];
            break;
            case POWER_5V_NOT_DETECTED:
                errorTime = power5vNotDetectedime[errorIndex % ARRAY_SIZE(power5vNotDetectedime)];
            break;
            case POWER_3V3_NOT_DETECTED:
                errorTime = power3v3NotDetectedime[errorIndex % ARRAY_SIZE(power3v3NotDetectedime)];
            break;
            default:
                return;
        }

        errorTime *= 10;
        errorTime += now;

        led_r.set(!(errorIndex % 2));
    }
}
