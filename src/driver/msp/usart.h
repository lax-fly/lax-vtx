#pragma once

#include <stdint.h>

void *usart_config(uint32_t tx_pin, uint32_t rx_pin, uint32_t baud, float stopbit);
void usart_async_send(void *handle, const uint8_t *data, int dsz);
void usart_sync_send(void *handle, const uint8_t *data, int dsz);
int usart_async_recv(void *handle, uint8_t *circle_buffer, int bsz);
