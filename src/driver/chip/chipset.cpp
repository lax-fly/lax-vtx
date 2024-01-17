#include "assert.h"
#include "stddef.h"

#include "chipset.h"
#include "rtc6705.h"
#include "rfpa5542.h"

RfModChip *RfModChip::new_instance(ChipId id)
{
    switch (id)
    {
    case CHIP_ID_RF6705_1:
    {
        RTC6705 *mod_chip = new RTC6705();
        if (0 == mod_chip->init(CHIP_ID_RF6705_1))
            return mod_chip;
        delete mod_chip;
    }
    default:
        assert(false);
        break;
    }
    return NULL;
}

RfPaChip *RfPaChip::new_instance(ChipId id)
{
    switch (id)
    {
    case CHIP_ID_PA5542_1:
    {
        PA5542 *pa_chip = new PA5542();
        if (0 == pa_chip->init(CHIP_ID_PA5542_1))
            return pa_chip;
        delete pa_chip;
        break;
    }
    default:
        assert(false);
        break;
    }
    return NULL;
}
