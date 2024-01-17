#include "usart.h"
#include "gpio.h"
#include "helpers.h"
#include "assert.h"
#include "at32f421_conf.h"

typedef struct
{
    uint16_t pin;
    uint8_t af;
    dma_channel_type *dma_channel;
} usart_pin;

typedef struct
{
    usart_type *regs;
    usart_pin tx;
    usart_pin rx;

} usart_t;

usart_t usart_map[] = {
    {USART1,
     {PA9, GPIO_MUX_1, DMA1_CHANNEL2},
     {PA10, GPIO_MUX_1, DMA1_CHANNEL3}},
    {USART1,
     {PB6, GPIO_MUX_0, DMA1_CHANNEL2},
     {PB7, GPIO_MUX_0, DMA1_CHANNEL3}},

    {USART2,
     {PA2, GPIO_MUX_1, DMA1_CHANNEL4},
     {PA3, GPIO_MUX_1, DMA1_CHANNEL5}},
    {USART2,
     {PA8, GPIO_MUX_4, DMA1_CHANNEL4},
     {PB0, GPIO_MUX_3, DMA1_CHANNEL5}},
    {USART2,
     {PA14, GPIO_MUX_1, DMA1_CHANNEL4},
     {PA15, GPIO_MUX_1, DMA1_CHANNEL5}},
};

static volatile uint8_t rx_head, rx_tail;

static void dma_buffer_config(dma_channel_type *dmax_channely, uint32_t peripheral_base_addr, uint32_t memory_base_addr, uint16_t buffer_size)
{
    dmax_channely->dtcnt = buffer_size;
    dmax_channely->paddr = peripheral_base_addr;
    dmax_channely->maddr = memory_base_addr;
}

static int usart_dma_init(usart_t *usart)
{
    usart_type *usart_regs = usart->regs;
    dma_init_type dma_init_struct;
    dma_default_para_init(&dma_init_struct);

    usart_dma_transmitter_enable(usart_regs, TRUE);
    usart_dma_receiver_enable(usart_regs, TRUE);

    dma_reset(usart->tx.dma_channel);
    dma_init_struct.direction = DMA_DIR_MEMORY_TO_PERIPHERAL;
    dma_init_struct.memory_data_width = DMA_MEMORY_DATA_WIDTH_BYTE;
    dma_init_struct.memory_inc_enable = TRUE;
    dma_init_struct.peripheral_data_width = DMA_PERIPHERAL_DATA_WIDTH_BYTE;
    dma_init_struct.peripheral_inc_enable = FALSE;
    dma_init_struct.priority = DMA_PRIORITY_LOW;
    dma_init_struct.loop_mode_enable = FALSE;
    dma_init(usart->tx.dma_channel, &dma_init_struct);

    dma_reset(usart->rx.dma_channel);
    dma_init_struct.direction = DMA_DIR_PERIPHERAL_TO_MEMORY;
    dma_init_struct.loop_mode_enable = TRUE;
    dma_init(usart->rx.dma_channel, &dma_init_struct);

    return 0;
}

static void _usart_config(usart_t *usart_cfg, uint32_t baud, uint8_t halfduplex, float stopbit)
{
    usart_type *usart_regs = usart_cfg->regs;

    if (halfduplex)
    {
        gpio_af_setup(usart_cfg->tx.pin, usart_cfg->tx.af, 1);
        gpio_in_setup(usart_cfg->rx.pin, 1); // make rx a pullup resistance for tx, if you already have a pullup resistance at tx pin, this line is not needed
    }
    else
    {
        gpio_af_setup(usart_cfg->rx.pin, usart_cfg->rx.af, 0);
        gpio_af_setup(usart_cfg->tx.pin, usart_cfg->tx.af, 0);
    }

    usart_stop_bit_num_type stop;
    if (stopbit < 1)
        stop = USART_STOP_0_5_BIT;
    else if (stopbit == 1)
        stop = USART_STOP_1_BIT;
    else if (stopbit < 2)
        stop = USART_STOP_1_5_BIT;
    else
        stop = USART_STOP_2_BIT;

    /* configure param */
    usart_init(usart_regs, baud, USART_DATA_8BITS, stop);
    usart_transmitter_enable(usart_regs, TRUE);
    usart_receiver_enable(usart_regs, TRUE);
    usart_parity_selection_config(usart_regs, USART_PARITY_NONE);

    usart_interrupt_enable(usart_regs, USART_RDBF_INT, TRUE);

    if (halfduplex)
        usart_single_line_halfduplex_select(usart_regs, TRUE);

    usart_enable(usart_regs, TRUE);
}

void *usart_config(uint32_t tx_pin, uint32_t rx_pin, uint32_t baud, float stopbit)
{
    uint8_t iter, halfduplex = (tx_pin == rx_pin);

    for (iter = 0; iter < ARRAY_SIZE(usart_map); iter++)
    {
        if (usart_map[iter].tx.pin == tx_pin && (halfduplex || usart_map[iter].rx.pin == rx_pin))
        {
            _usart_config(&usart_map[iter], baud, halfduplex, stopbit);
            usart_dma_init(&usart_map[iter]);
            return &usart_map[iter];
        }
    }
    return NULL;
}

void usart_async_send(void *handle, const uint8_t *data, int dsz)
{
    assert(handle && data && dsz);
    usart_t *usart = (usart_t *)handle;
    dma_channel_enable(usart->tx.dma_channel, FALSE);
    dma_buffer_config(usart->tx.dma_channel, (uint32_t)&usart->regs->dt, (uint32_t)data, dsz);
    dma_channel_enable(usart->tx.dma_channel, TRUE);
}

void usart_sync_send(void *handle, const uint8_t *data, int dsz)
{
    usart_async_send(handle, data, dsz);
    usart_t *usart = (usart_t *)handle;
    while (dma_data_number_get(usart->tx.dma_channel) != 0)
    {
    } // wait for transmission
}

// return the next writing index
int usart_async_recv(void *handle, uint8_t *circle_buffer, int bsz)
{
    assert(handle && circle_buffer && bsz);
    usart_t *usart = (usart_t *)handle;

    if ((uint32_t)circle_buffer == usart->rx.dma_channel->maddr)
        return bsz - dma_data_number_get(usart->rx.dma_channel);

    dma_channel_enable(usart->rx.dma_channel, FALSE);
    dma_buffer_config(usart->rx.dma_channel, (uint32_t)&usart->regs->dt, (uint32_t)circle_buffer, bsz);
    dma_channel_enable(usart->rx.dma_channel, TRUE);
    return 0;
}
