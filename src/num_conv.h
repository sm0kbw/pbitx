#ifndef CONVERSION
#define CONVERSION
#include <stdint.h>
#include <stdbool.h>


uint32_t  bcd2int (uint8_t offset, uint8_t *inque);
void int2bcd (uint32_t num, uint8_t *str);

uint16_t ch2div (uint8_t ch_num, bool tx);
uint16_t div2frq (uint16_t div, bool tx);
uint16_t frq2div (uint16_t freq, bool tx);


#endif // CONVERSION
