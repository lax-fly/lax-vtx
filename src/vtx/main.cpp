#include "Config.h"
#include "main.h"

Led led_r;
Led led_g;
Led led_b;

Serial serial_1;
Button button_1;
Radio radio;
Sensor sensor;

Disk g_disk;
Serial debug_serial;

extern void resetModeIndication();

void system_init(void)
{
    Timeout::init();
    sensor.init();
    g_disk.open(DEV_ID_DISK1);
    led_r.open(DEV_ID_LED1);
    led_g.open(DEV_ID_LED2);
    led_b.open(DEV_ID_LED3);
    radio.open(DEV_ID_RF1);
    Protocol::init(g_config.vtxMode);
    resetModeIndication();
    g_config.load();
}
#include "stdio.h"
// char buf[1];
// Timeout t1(100);
void loop(void)
{
    uint32_t e;

    while (1)
    {
        Protocol::poll();
        Dev::poll();
        // printf("loop: %d", (int)sensor.get_mcu_temp());
        // e = time_us();
        // if (t1.check())
        // {
        //     int sz = debug_serial.recv((uint8_t*)buf, 1);
        //     if (sz)
        //         debug_serial.send((uint8_t*)buf, sz);
        // }
    }
}

int main(void)
{
    system_init();
    loop();
}
