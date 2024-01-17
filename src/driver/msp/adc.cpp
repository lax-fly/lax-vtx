#include "adc.h"
#include "gpio.h"
#include "helpers.h"
#include "at32f421_conf.h"

#define ADC_REF_VOLT_mW 3100
#define ADC_VREF (3.3f)
#define ADC_TEMP_BASE (1.28f)
#define ADC_TEMP_SLOPE (0.0043f)

static uint8_t init_done;
static uint16_t conv_results[32];

void adc_dma_config(uint32_t memory_base_addr, uint16_t buffer_cnt);

static void init_adc(void)
{
    if (init_done)
        return;
    init_done = 1;

    adc_base_config_type adc_base_struct;

    crm_adc_clock_div_set(CRM_ADC_DIV_6);

    adc_tempersensor_vintrv_enable(TRUE);
    /* adc_settings----------------------------------------------------------- */
    adc_base_default_para_init(&adc_base_struct);
    adc_base_struct.sequence_mode = TRUE;
    adc_base_struct.repeat_mode = TRUE;
    adc_base_struct.data_align = ADC_RIGHT_ALIGNMENT;
    adc_base_struct.ordinary_channel_length = 2;
    adc_base_config(ADC1, &adc_base_struct);

    /* adc_ordinary_conversionmode-------------------------------------------- */
    adc_ordinary_channel_set(ADC1, ADC_CHANNEL_16, 2, ADC_SAMPLETIME_239_5); // temperature

    adc_ordinary_conversion_trigger_set(ADC1, ADC12_ORDINARY_TRIG_SOFTWARE, TRUE);
    adc_ordinary_part_mode_enable(ADC1, FALSE);
    adc_dma_mode_enable(ADC1, TRUE);
    adc_enable(ADC1, TRUE);
    /* adc_ordinary_conversionmode-------------------------------------------- */

    /* adc calibration-------------------------------------------------------- */
    adc_calibration_init(ADC1);
    while (adc_calibration_init_status_get(ADC1))
        ;
    adc_calibration_start(ADC1);
    while (adc_calibration_status_get(ADC1))
        ;
}

adc_t adc_config(uint32_t pin)
{
    init_adc();
    // only for PA0~PA7, other pins are  supported in future
    if (GPIO2IDX(pin) > 7)
    {
        while (1)
        {
        }
    }
    adc_ordinary_channel_set(ADC1, (adc_channel_select_type)GPIO2IDX(pin), 1, ADC_SAMPLETIME_239_5);
    gpio_analog_setup(pin);
    adc_dma_config((uint32_t)conv_results, ARRAY_SIZE(conv_results));
    adc_ordinary_software_trigger_enable(ADC1, TRUE);
    return adc_t(GPIO2IDX(pin));
}

void adc_dma_config(uint32_t memory_base_addr, uint16_t buffer_cnt)
{
    dma_init_type dma_init_struct;

    dma_reset(DMA1_CHANNEL1);
    dma_default_para_init(&dma_init_struct);
    dma_init_struct.direction = DMA_DIR_PERIPHERAL_TO_MEMORY;
    dma_init_struct.memory_data_width = DMA_MEMORY_DATA_WIDTH_HALFWORD;
    dma_init_struct.memory_inc_enable = TRUE;
    dma_init_struct.peripheral_data_width = DMA_PERIPHERAL_DATA_WIDTH_HALFWORD;
    dma_init_struct.peripheral_inc_enable = FALSE;
    dma_init_struct.priority = DMA_PRIORITY_LOW;
    dma_init_struct.loop_mode_enable = TRUE;
    dma_init(DMA1_CHANNEL1, &dma_init_struct);

    dma_interrupt_enable(DMA1_CHANNEL1, DMA_HDT_INT | DMA_FDT_INT, TRUE);
    dma_channel_enable(DMA1_CHANNEL1, FALSE);

    DMA1_CHANNEL1->dtcnt = buffer_cnt;
    DMA1_CHANNEL1->paddr = (uint32_t)&ADC1->odt;
    DMA1_CHANNEL1->maddr = memory_base_addr;

    dma_channel_enable(DMA1_CHANNEL1, TRUE);

    nvic_irq_enable(DMA1_Channel1_IRQn, 0, 0);
}

static uint16_t adc_value;
static uint16_t adc_temp;

float adc_get_temp(void)
{
    return ((float)adc_temp * ADC_VREF / 4096 - ADC_TEMP_BASE) / ADC_TEMP_SLOPE + 25;
}

uint32_t adc_read(adc_t config)
{
    uint32_t tmp = adc_value;
    tmp *= ADC_REF_VOLT_mW;
    tmp /= 4096;
    return tmp;
}

extern "C"
{
    void DMA1_Channel1_IRQHandler(void);
}
void DMA1_Channel1_IRQHandler(void)
{
    uint32_t tmp = 0;
    uint32_t s = 0;
    uint32_t e = ARRAY_SIZE(conv_results) / 2;
    if (dma_flag_get(DMA1_HDT1_FLAG))
    {
        dma_flag_clear(DMA1_HDT1_FLAG);
    }
    else
    {
        dma_flag_clear(DMA1_FDT1_FLAG);
        s = ARRAY_SIZE(conv_results) / 2;
        e = ARRAY_SIZE(conv_results);
    }

    for (uint8_t iter = s; iter < e; iter += 2)
        tmp += conv_results[iter];

    tmp /= ARRAY_SIZE(conv_results) / 4;
    adc_value = tmp;

    tmp = 0;
    for (uint8_t iter = s + 1; iter < e; iter += 2)
        tmp += conv_results[iter];

    tmp /= ARRAY_SIZE(conv_results) / 4;
    adc_temp = tmp;
}
