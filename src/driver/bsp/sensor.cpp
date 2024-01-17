#include "sensor.h"
#include "adc.h"

int Sensor::init(void)
{
    return 0;
}

float Sensor::get_mcu_temp(void)
{
    return adc_get_temp();
}
