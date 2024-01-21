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

#define INIT_ERROR()  \
    {                 \
        led_r.set(1); \
        while (true)  \
            ;         \
    }

extern void resetModeIndication();
#include "table.h"
void system_init(void)
{
    Timeout::init();
    sensor.init();
    if (0 != debug_serial.open(DEV_ID_USART2, 256000))
        INIT_ERROR();
    if (0 != g_disk.open(DEV_ID_DISK1))
        INIT_ERROR();
    if (0 != led_r.open(DEV_ID_LED1))
        INIT_ERROR();
    if (0 != led_g.open(DEV_ID_LED2))
        INIT_ERROR();
    if (0 != led_b.open(DEV_ID_LED3))
        INIT_ERROR();
    if (0 != radio.open(DEV_ID_RF1))
        INIT_ERROR();
    g_config.load();
    Protocol::init(g_config.vtxMode);
    resetModeIndication();
}
#include "stdio.h"
#include "string.h"
#include "stdlib.h"
char buf[32];
Timeout t1(50);
void loop(void)
{
    while (1)
    {
        Protocol::poll();
        Dev::poll();
        if (t1.check())
        {
            int sz = debug_serial.recv((uint8_t *)buf, 32);
            for (int i = 1; i < sz; ++i)
            {
                if (buf[i - 1] == '\r' && buf[i] == '\n')
                {
                    buf[i - 1] = 0;
                    if (strcmp(buf, "print") == 0)
                    {
                        radio.print_chip_regs();
                    }
                    if (strncmp(buf, "setfreq ", 8) == 0)
                    {
                        int freq = strtod(buf + 8, NULL);
                        radio.set_freq(freq);
                        printf("setfreq %d\n", freq);
                    }
                    if (strncmp(buf, "setpower ", 9) == 0)
                    {
                        int dbm = strtod(buf + 9, NULL);
                        radio.set_power(dbm);
                        printf("setpower %d\n", dbm);
                    }
                    break;
                }
            }
        }
    }
}

int main(void)
{
    system_init();
    loop();
}
