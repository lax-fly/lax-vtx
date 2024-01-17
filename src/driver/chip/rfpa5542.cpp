#ifdef USE_PA_RFPA5542

#include "rfpa5542.h"
#include "assert.h"

// guess there is more than once on board rfpa

PA5542::~PA5542()
{
    uninit();
}

int PA5542::init(ChipId id)
{
    pwm_val = 9000;
    switch (id)
    {
    case CHIP_ID_PA5542_1:
        vpd_pin = adc_config(PA5542_1_VPD);
        vref_pin = gpio_out_setup(PA5542_1_VREF, 0); // Power amp OFF
        outputPowerTimer = pwm_init(RTC_BIAS);
        if (outputPowerTimer.tim == 0)
            return -1;
        break;
    default:
        assert(false);
        break;
    }
    return 0;
}

void PA5542::uninit(void)
{
    // Repeated uninitialization check
}

int PA5542::set_amp(uint32_t vpd_set)
{
    // adjust power output to meet the set vpd
    if (vpd_set == 0)
    {
        gpio_out_write(vref_pin, 0);
        return 0;
    }

    gpio_out_write(vref_pin, 1);
    uint32_t real_vpd = adc_read(vpd_pin);

    if (real_vpd > vpd_set + 20)
    {
        if (pwm_val < 9000)
        {
            pwm_val += 100;
            pwm_out_write(outputPowerTimer, pwm_val);
        }
    }
    else if (real_vpd < vpd_set - 20)
    {
        if (pwm_val > 7000)
        {
            pwm_val -= 100;
            pwm_out_write(outputPowerTimer, pwm_val);
        }
    }
    pwm_out_write(outputPowerTimer, 9000);
    return 0;
}

void PA5542::print_all_regs(char *buf, int bsz)
{
}

#endif
