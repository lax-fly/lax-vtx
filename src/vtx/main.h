#pragma once
#include <assert.h>
#include <stdio.h>

#include "led.h"
#include "sensor.h"
#include "radio.h"
#include "serial.h"
#include "button.h"
#include "timer.h"
#include "disk.h"

extern Led led_r;
extern Led led_g;
extern Led led_b;

extern Serial serial_1;

extern Button button_1;
extern Radio radio;
extern Sensor sensor;

extern Disk g_disk;

