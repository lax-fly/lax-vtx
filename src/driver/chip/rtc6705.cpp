#if USE_RTC6705

#include "rtc6705.h"
#include "gpio.h"
#include "clock.h"
#include "stdio.h"
#include "pins.h"

#define SynthesizerRegisterA 0x00
#define SynthesizerRegisterB 0x01
#define SynthesizerRegisterC 0x02
#define RFVCODFCControlRegister 0x03
#define VCOControlRegister1 0x04
#define VCOControlRegister2 0x05
#define AudioModulatorControlRegister 0x06
#define PredriverandPAControlRegister 0x07
#define StateRegister 0x0F

#define STATE_RESET 0
#define STATE_PWRON_CAL 1
#define STATE_STBY 2
#define STATE_VCO_CAL 3

#define READ_BIT 0x00
#define WRITE_BIT 0x01

#define SYNTH_REG_A_DEFAULT 0x0190
#define POWER_AMP_ON 0X4FBF // 0b100111110111111

#define PLL_SETTLE_TIME 5

RTC6705::~RTC6705()
{
    uninit();
}

int RTC6705::init(ChipId id)
{
    ss_pin = gpio_out_setup(RTC6705_1_SPI_SS, 1);
    sck_pin = gpio_out_setup(RTC6705_1_SPI_CLOCK, 0);
    mosi_pin = gpio_out_setup(RTC6705_1_SPI_MOSI, 0);

    write_reg(StateRegister, 0);
    write_reg(SynthesizerRegisterA, SYNTH_REG_A_DEFAULT);
    // check if chip exists
    if (read_reg(SynthesizerRegisterA) == SYNTH_REG_A_DEFAULT)
    {
        power_off();
        return 0;
    }
    else
        return -1;
}

void RTC6705::uninit(void)
{
    // Repeated uninitialization check
}

uint32_t RTC6705::read_reg(uint8_t addr)
{
    uint32_t writeData = addr | (READ_BIT << 4);
    uint32_t readData = 0;
    uint8_t i;

    gpio_out_write(ss_pin, 0);
    delay_us(1);

    // Write register address and read bit
    for (i = 0; i < 5; i++)
    {
        gpio_out_write(sck_pin, 0);
        delay_us(1);
        gpio_out_write(mosi_pin, writeData & 0x1);
        delay_us(1);
        gpio_out_write(sck_pin, 1);
        delay_us(1);

        writeData >>= 1;
    }

    // Change pin from output to input
    gpio_t miso_pin = gpio_in_setup(RTC6705_1_SPI_MOSI, 0);

    // Read data 20 bits
    for (i = 0; i < 20; i++)
    {
        gpio_out_write(sck_pin, 0);
        delay_us(1);

        if (gpio_in_read(miso_pin))
        {
            readData = readData | (1 << i);
        }

        gpio_out_write(sck_pin, 1);
        delay_us(1);
    }
    // Change pin back to output
    mosi_pin = gpio_out_setup(RTC6705_1_SPI_MOSI, 0);

    gpio_out_write(sck_pin, 0);
    gpio_out_write(ss_pin, 1);

    return readData;
}

void RTC6705::write_reg(uint8_t addr, uint32_t data)
{
    uint32_t frame = addr | (WRITE_BIT << 4) | (data << 5);
    gpio_out_write(ss_pin, 0);
    delay_us(1);

    for (uint8_t i = 0; i < 25; i++)
    {
        gpio_out_write(sck_pin, 0);
        delay_us(1);
        gpio_out_write(mosi_pin, frame & 0x1);
        delay_us(1);
        gpio_out_write(sck_pin, 1);
        delay_us(1);

        frame >>= 1;
    }

    gpio_out_write(sck_pin, 0);
    gpio_out_write(mosi_pin, 0);
    gpio_out_write(ss_pin, 1);
    delay_us(1);
}

void RTC6705::power_on(void)
{
    uint32_t pa_reg_v = read_reg(PredriverandPAControlRegister);

    if (pa_reg_v == POWER_AMP_ON)
        return;

    write_reg(PredriverandPAControlRegister, POWER_AMP_ON);
}

void RTC6705::power_off(void)
{
    uint32_t pa_reg_v = read_reg(PredriverandPAControlRegister);

    if (pa_reg_v == 0)
        return;

    write_reg(PredriverandPAControlRegister, 0);
}
// newFreq: unit MHz
int RTC6705::set_freq(uint32_t newFreq)
{
    uint32_t freq = newFreq * 1000U;
    freq /= 40;
    uint32_t SYN_RF_N_REG = freq / 64;
    uint32_t SYN_RF_A_REG = freq % 64;

    uint32_t newRegData = SYN_RF_A_REG | (SYN_RF_N_REG << 7);
    uint32_t syn_reg_b = read_reg(SynthesizerRegisterB);

    if (newRegData == syn_reg_b)
        return 0;

    /* Switch off */
    power_off();

    write_reg(SynthesizerRegisterA, SYNTH_REG_A_DEFAULT);

    /* Set frequency */
    write_reg(SynthesizerRegisterB, newRegData);

    // wait for pll to settle
    delay_ms(PLL_SETTLE_TIME);
    power_on();

    return 0;
}

int RTC6705::set_power(uint32_t dbm)
{
    if (dbm == 0)
        power_off();
    else
        power_on();
    return 0;
}

bool RTC6705::is_busy(void)
{
    return (read_reg(StateRegister) != STATE_STBY);
}

void RTC6705::print_all_regs(char *buf, int bsz)
{
    uint32_t regs[8];
    for (int i = 0; i < 8; i++)
    {
        regs[i] = read_reg(i);
    }
    uint32_t statereg = read_reg(StateRegister);
    snprintf(buf, bsz,
             "SynthesizerA:          0X%lX\r\n"
             "SynthesizerB:          0X%lX\r\n"
             "SynthesizerC:          0X%lX\r\n"
             "RFVCODFCControl:       0X%lX\r\n"
             "VCOControl1:           0X%lX\r\n"
             "VCOControl2:           0X%lX\r\n"
             "AudioModulatorControl: 0X%lX\r\n"
             "PredriverPAControl:    0X%lX\r\n"
             "State:                 0X%lX\r\n",
             regs[0], regs[1], regs[2], regs[3], regs[4], regs[5], regs[6], regs[7], statereg);
}

#endif
