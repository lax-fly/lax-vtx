
#include "at32f421_conf.h"

// not used yet
void crc_init(void)
{
    crc_init_data_set(0x00000000);
    crc_poly_size_set(CRC_POLY_SIZE_8B);
    crc_poly_value_set(0xB7);
    crc_reverse_input_data_set(CRC_REVERSE_INPUT_NO_AFFECTE);
    crc_reverse_output_data_set(CRC_REVERSE_OUTPUT_NO_AFFECTE);
    crc_data_reset();
}
