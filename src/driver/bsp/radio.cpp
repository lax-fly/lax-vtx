#include "assert.h"
#include "stdio.h"
#include "stdlib.h"
#include "math.h"

#include "radio.h"
#include "helpers.h"
#include "timer.h"
#include "chipset.h"

#define OUTPUT_POWER_INTERVAL 5 // ms
#define CAL_FREQ_SIZE 9
#define CAL_DBM_SIZE 8
static uint16_t calFreqs[CAL_FREQ_SIZE] = {5600, 5650, 5700, 5750, 5800, 5850, 5900, 5950, 6000};
static uint8_t calDBm[CAL_DBM_SIZE] = {10, 14, 16, 20, 23, 26, 29, 32};
static uint16_t calVpd[CAL_DBM_SIZE][CAL_FREQ_SIZE] = {
    {415, 375, 375, 380, 380, 380, 395, 390, 400},
    {420, 420, 400, 400, 405, 425, 430, 440, 435},
    {470, 455, 445, 445, 455, 465, 485, 490, 490},
    {525, 510, 490, 490, 500, 515, 540, 550, 545},
    {575, 565, 555, 545, 560, 575, 595, 600, 600},
    {640, 630, 610, 610, 625, 645, 665, 675, 670},
    {730, 710, 685, 680, 690, 725, 750, 755, 755},
    {805, 785, 755, 755, 770, 805, 830, 840, 835},
};

int Radio::open(DEV_ID id)
{
    if (0 != register_dev(id, this, "rtc6705 rfpa5542"))
        return -1;

    switch (id)
    {
    case DEV_ID_RF1:
        m_pa_handle = (void *)RfPaChip::new_instance(CHIP_ID_PA5542_1);
        if (NULL == m_pa_handle)
            goto ERR;

        m_mod_handle = (void *)RfModChip::new_instance(CHIP_ID_RF6705_1);
        if (NULL == m_mod_handle)
            goto ERR;

        break;
    default:
        assert(false);
        break;
    }

    m_id = id;
    m_power_dbm = 0;
    m_freq = 0;
    m_vpd_set = 0;

    print_chip_regs();

    return 0;

ERR:
    close();
    return -1;
}

// cast to Dev* first if RfModChip is multiplely inherited(is not yet), refer to c++ multiple inheriting
#define TO_RFMODCHIP(x) ((RfModChip *)(Dev *)(x))
#define TO_RFPACHIP(x) ((RfPaChip *)(Dev *)(x))

void Radio::close(void)
{
    unregister_dev(m_id);
    if (m_mod_handle)
    {
        delete TO_RFMODCHIP(m_mod_handle);
    }
    if (m_pa_handle)
    {
        // cast to Dev* first if RfModChip is multiplely inherited(is not yet), refer to c++ multiple inheriting
        delete TO_RFPACHIP(m_pa_handle);
    }
    m_id = DEV_ID_NONE;
    m_power_dbm = 0;
    m_freq = 0;
    m_vpd_set = 0;
    m_mod_handle = NULL;
    m_pa_handle = NULL;
}

void Radio::event_poll(void)
{
    // adjust power output
    static uint32_t temp;
    uint32_t now = time_ms();
    if (OUTPUT_POWER_INTERVAL <= (now - temp))
    {
        temp = now;
        TO_RFPACHIP(m_pa_handle)->set_amp(m_vpd_set);
    }
}

uint16_t Radio::bilinearInterpolation(float dbm)
{
    uint16_t tempFreq = m_freq;
    uint8_t i;
    uint8_t calFreqsIndex = 0;
    uint8_t calDBmIndex = 0;

    if (tempFreq < 5600)
        tempFreq = 5600;
    if (tempFreq > 6000)
        tempFreq = 6000;

    for (i = 0; i < (ARRAY_SIZE(calFreqs) - 1); i++)
    {
        if (tempFreq < calFreqs[i + 1])
        {
            calFreqsIndex = i;
            break;
        }
    }

    for (i = 0; i < (ARRAY_SIZE(calDBm) - 1); i++)
    {
        if (dbm < calDBm[i + 1])
        {
            calDBmIndex = i;
            break;
        }
    }

    float x = dbm;
    float x1 = calDBm[calDBmIndex];
    float x2 = calDBm[calDBmIndex + 1];

    float y = tempFreq;
    float y1 = calFreqs[calFreqsIndex];
    float y2 = calFreqs[calFreqsIndex + 1];

    float Q11 = calVpd[calDBmIndex][calFreqsIndex];
    float Q12 = calVpd[calDBmIndex][calFreqsIndex + 1];
    float Q21 = calVpd[calDBmIndex + 1][calFreqsIndex];
    float Q22 = calVpd[calDBmIndex + 1][calFreqsIndex + 1];

    float fxy1 = Q11 * (x2 - x) / (x2 - x1) + Q21 * (x - x1) / (x2 - x1);
    float fxy2 = Q12 * (x2 - x) / (x2 - x1) + Q22 * (x - x1) / (x2 - x1);

    uint16_t fxy = fxy1 * (y2 - y) / (y2 - y1) + fxy2 * (y - y1) / (y2 - y1);

    return fxy;
}

int Radio::set_freq(uint32_t freq)
{
    int res = TO_RFMODCHIP(m_mod_handle)->set_freq(freq);
    // print_chip_regs();
    return res;
}

int Radio::set_power(uint32_t dbm)
{
    m_power_dbm = dbm;
    if (dbm == 0)
    {
        TO_RFMODCHIP(m_mod_handle)->set_power(0);
    }

    if (dbm < 10)
    {
        m_vpd_set = 0;
    }
    else if (dbm >= 26)
    {
        m_vpd_set = 1500;
    }
    else
    {
        m_vpd_set = bilinearInterpolation(dbm);
    }

    return 0;
}

int Radio::set_power_mw(uint32_t mW)
{
    float dbm = 10.0 * log10f((float)mW); // avoid double conversion!
    set_power(dbm);
    return 0;
}

int Radio::set_freq_power(uint32_t freq, uint32_t power)
{
    if (0 != set_freq(freq))
        return -1;
    if (0 != set_power(power))
        return -2;
    return 0;
}

Radio::State Radio::get_state(void) const
{
    if (TO_RFMODCHIP(m_mod_handle)->is_busy())
        return BUSY;
    else
        return READY;
}

void Radio::get_freq_range(uint32_t &min, uint32_t &max) const
{
    switch (m_id)
    {
    case DEV_ID_RF1:
        min = RTC6705_MIN_FREQ;
        max = RTC6705_MAX_FREQ;
        break;
    default:
        min = 0;
        max = 0;
        break;
    }
}

uint32_t Radio::get_max_power(void) const
{
    switch (m_id)
    {
    case DEV_ID_RF1:
        return powf(10, (RTC6705_MAX_POWER + PA5542_MAX_GAIN) / 10); // unit mW
    default:
        return 0;
    }
}

void Radio::print_chip_regs(void)
{
#ifndef NDEBUG
    char *buf = (char *)malloc(384);
    if (buf)
    {
        TO_RFMODCHIP(m_mod_handle)->print_all_regs(buf, 384);
        debug("%s", buf);
        free(buf);
    }
#endif
}
