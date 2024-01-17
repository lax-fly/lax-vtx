#include "at32f421_conf.h"

void system_clock_config(void)
{
    /* reset crm */
    crm_reset();

    /* config flash psr register */
    flash_psr_set(FLASH_WAIT_CYCLE_3);

    /* enable lick */
    crm_clock_source_enable(CRM_CLOCK_SOURCE_LICK, TRUE);

    /* wait till lick is ready */
    while (crm_flag_get(CRM_LICK_STABLE_FLAG) != SET)
    {
    }

    /* enable hick */
    crm_clock_source_enable(CRM_CLOCK_SOURCE_HICK, TRUE);

    /* wait till hick is ready */
    while (crm_flag_get(CRM_HICK_STABLE_FLAG) != SET)
    {
    }

    /* config pll clock resource */
    crm_pll_config(CRM_PLL_SOURCE_HICK, CRM_PLL_MULT_30);

    /* enable pll */
    crm_clock_source_enable(CRM_CLOCK_SOURCE_PLL, TRUE);

    /* wait till pll is ready */
    while (crm_flag_get(CRM_PLL_STABLE_FLAG) != SET)
    {
    }

    /* config ahbclk */
    crm_ahb_div_set(CRM_AHB_DIV_1);

    /* config apb2clk */
    crm_apb2_div_set(CRM_APB2_DIV_1);

    /* config apb1clk */
    crm_apb1_div_set(CRM_APB1_DIV_1);

    /* enable auto step mode */
    crm_auto_step_mode_enable(TRUE);

    /* select pll as system clock source */
    crm_sysclk_switch(CRM_SCLK_PLL);

    /* wait till pll is used as system clock source */
    while (crm_sysclk_switch_status_get() != CRM_SCLK_PLL)
    {
    }

    /* disable auto step mode */
    crm_auto_step_mode_enable(FALSE);

    /* update system_core_clock global variable */
    system_core_clock_update();
}

void periph_clock_config(void)
{
    /* enable dma1 periph clock */
    crm_periph_clock_enable(CRM_DMA1_PERIPH_CLOCK, TRUE);

    /* enable crc periph clock */
    crm_periph_clock_enable(CRM_CRC_PERIPH_CLOCK, TRUE);

    /* enable gpioa periph clock */
    crm_periph_clock_enable(CRM_GPIOA_PERIPH_CLOCK, TRUE);

    /* enable gpiob periph clock */
    crm_periph_clock_enable(CRM_GPIOB_PERIPH_CLOCK, TRUE);

    /* enable scfg periph clock */
    crm_periph_clock_enable(CRM_SCFG_PERIPH_CLOCK, TRUE);

    /* enable adc1 periph clock */
    crm_periph_clock_enable(CRM_ADC1_PERIPH_CLOCK, TRUE);

    /* enable usart1 periph clock */
    crm_periph_clock_enable(CRM_USART1_PERIPH_CLOCK, TRUE);

    /* enable tmr3 periph clock */
    crm_periph_clock_enable(CRM_TMR3_PERIPH_CLOCK, TRUE);

    /* enable usart2 periph clock */
    crm_periph_clock_enable(CRM_USART2_PERIPH_CLOCK, TRUE);
}

void systick_config(void)
{
    systick_clock_source_config(SYSTICK_CLOCK_SOURCE_AHBCLK_NODIV);
    /* setup systick timer for 1ms interrupts  */
    SysTick_Config(system_core_clock / 1000);
    nvic_priority_group_config(NVIC_PRIORITY_GROUP_4);
    nvic_irq_enable(SysTick_IRQn, 0, 0);
}

void clock_init(void)
{
    system_clock_config();
    periph_clock_config();
    systick_config();
}

static volatile uint32_t sys_ms = 0;

void mcu_reboot(void)
{
    nvic_system_reset();
}

extern "C"
{
    void SysTick_Handler(void); // irq handlers are all C symbol
}
void SysTick_Handler(void)
{
    ++sys_ms;
}

uint32_t get_ms(void)
{
    return sys_ms;
}

uint32_t get_us(void)
{
    return (sys_ms * 1000) + ((SysTick->LOAD + 1 - SysTick->VAL) / 120); // 120 MHz
}

void delay_ms(uint32_t ms)
{
    uint32_t s_ms = sys_ms;
    uint32_t e_ms = s_ms + ms;
    if (e_ms < s_ms) // overflow check
    {
        while (sys_ms > s_ms)
        {
        } // make sure loop once duration in 1 ms
    }
    while (sys_ms < e_ms)
    {
    }
}

void delay_us(uint32_t us)
{
    uint32_t s_us = get_us();
    uint32_t e_us = s_us + us;
    if (e_us < s_us) // overflow check
    {
        while (get_us() > s_us)
        {
        } // make sure loop once duration in 1 ms
    }

    while (get_us() < e_us)
    {
    }
}
