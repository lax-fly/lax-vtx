#pragma once

#include <stdint.h>

void clock_init(void);
void mcu_reboot(void);
uint32_t get_ms(void);
uint32_t get_us(void);
void delay_ms(uint32_t ms);
void delay_us(uint32_t us);
