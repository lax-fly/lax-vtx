#include "pwm.h"
#include "gpio.h"
#include "helpers.h"
#include "at32f421_conf.h"

#define PWM_PERIOD 10000 // Results in ~12kHz PWM

struct pwms
{
    tmr_type *periph;
    uint16_t pin;
    tmr_channel_select_type ch;
    gpio_mux_sel_type af;
};
struct pwms pwm_config[] = {
    // each pin must be different from each other
    {TMR1, PA8, TMR_SELECT_CHANNEL_1, GPIO_MUX_0},
    {TMR1, PA9, TMR_SELECT_CHANNEL_2, GPIO_MUX_0},
    {TMR1, PA10, TMR_SELECT_CHANNEL_3, GPIO_MUX_0},

    {TMR3, PB4, TMR_SELECT_CHANNEL_1, GPIO_MUX_0},
    {TMR3, PB5, TMR_SELECT_CHANNEL_2, GPIO_MUX_0},
    {TMR3, PB0, TMR_SELECT_CHANNEL_3, GPIO_MUX_1},
    {TMR3, PB1, TMR_SELECT_CHANNEL_4, GPIO_MUX_1},

    {TMR14, PA4, TMR_SELECT_CHANNEL_1, GPIO_MUX_0},

    {TMR15, PA2, TMR_SELECT_CHANNEL_1, GPIO_MUX_0},
    {TMR15, PA3, TMR_SELECT_CHANNEL_2, GPIO_MUX_0},
};

pwm_t pwm_init(uint32_t pin)
{
    uint8_t index = 0;
    for (index = 0; index < ARRAY_SIZE(pwm_config); index++)
    {
        if (pin == pwm_config[index].pin)
            break;
    }

    if (ARRAY_SIZE(pwm_config) <= index)
    {
        return pwm_t(0, (tmr_channel_select_type)-1);
    }

    struct pwms pwm = pwm_config[index];

    gpio_af_setup(pin, pwm.af, 0);

    tmr_type *timer = pwm.periph;
    tmr_channel_select_type channel = pwm.ch;

    tmr_output_config_type tmr_output_struct;

    /* configure counter settings */
    tmr_base_init(timer, PWM_PERIOD - 1, 0);
    tmr_cnt_dir_set(timer, TMR_COUNT_UP);
    tmr_clock_source_div_set(timer, TMR_CLOCK_DIV1);
    tmr_period_buffer_enable(timer, FALSE);

    /* configure primary mode settings */
    tmr_sub_sync_mode_set(timer, FALSE);
    tmr_primary_mode_select(timer, TMR_PRIMARY_SEL_RESET);

    /* configure channel 3 output settings */
    tmr_output_struct.oc_mode = TMR_OUTPUT_CONTROL_PWM_MODE_A;
    tmr_output_struct.oc_output_state = TRUE;
    tmr_output_struct.occ_output_state = FALSE;
    tmr_output_struct.oc_polarity = TMR_OUTPUT_ACTIVE_HIGH;
    tmr_output_struct.occ_polarity = TMR_OUTPUT_ACTIVE_HIGH;
    tmr_output_struct.oc_idle_state = FALSE;
    tmr_output_struct.occ_idle_state = FALSE;
    tmr_output_channel_config(timer, channel, &tmr_output_struct);
    tmr_channel_value_set(timer, channel, PWM_PERIOD);
    tmr_output_channel_buffer_enable(timer, channel, FALSE);

    tmr_counter_enable(timer, TRUE);

    return pwm_t((uint32_t)timer, channel);
}

// dutycycle 0~10000 to 0.0000 ~ 1.0
void pwm_out_write(pwm_t pwm, uint16_t dutycycle)
{
    dutycycle *= 1;
    if (dutycycle > PWM_PERIOD)
        dutycycle = PWM_PERIOD;
    if (pwm.tim)
    {
        tmr_channel_value_set((tmr_type *)pwm.tim, (tmr_channel_select_type)pwm.ch, dutycycle);
    }
}
