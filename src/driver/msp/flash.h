#pragma once
#include <stdint.h>

void flash_update_block(uint16_t idx, uint8_t *ptr, uint32_t len);
void flash_read_block(const uint16_t idx, uint8_t *ptr, uint32_t len);
