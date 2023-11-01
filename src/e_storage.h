#ifndef _E_STORAGE_
#define _E_STORAGE_


void erase (void);

void read(void);
void write(void);

uint32_t e_get(uint16_t addr);
void e_put(uint16_t addr, uint32_t val);
//    EEPROM[]



#endif // _E_STORAGE_