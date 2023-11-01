// Just a simple replacement for the 256 byte eeprom storage
// 
// BA 2021-Mar-10 

#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/flash.h"
#include "hardware/sync.h"
#include "pico.h"
#include "e_storage.h"

#define USE_PAGE 2

uint8_t buffer[FLASH_PAGE_SIZE];


void erase (void)
{
  uint32_t ints;

  // Erase the last sector of the flash
  ints = save_and_disable_interrupts ();
  flash_range_erase ((PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE), FLASH_SECTOR_SIZE);
  restore_interrupts (ints);
}


void write (void)
{
  uint32_t ints;

	// Program buf[] into the first page of this sector
	ints = save_and_disable_interrupts ();
	flash_range_program ((PICO_FLASH_SIZE_BYTES - FLASH_PAGE_SIZE), (const uint8_t *)buffer, FLASH_PAGE_SIZE);
	restore_interrupts (ints);
}

 
void print_buf(const uint8_t *buf, size_t len)
{
	uint16_t i;


    for (i = 0; i < len; i++) {
        printf("%02x", buf[i]);
        if (i % 16 == 15)
            printf("\n");
        else
            printf(" ");
    }
}

void read (void)
{
  uint8_t *p, *b;
  int i;

	p = (uint8_t *) (XIP_BASE + PICO_FLASH_SIZE_BYTES - FLASH_PAGE_SIZE);
	b = buffer;

	for (i = 0; i < 256; i++)
	{
		*b++ = *p++;
	}
}


void e_put(uint16_t addr, uint32_t value)
{
//	uint32_t itrps;
   
	read ();
	uint8_t p = addr;
		
	if (addr < (USE_PAGE * FLASH_PAGE_SIZE))
	{
//		printf ("saving 0x%X at 0x%X\n", value, addr);

		buffer [p++] = value & 0xFF;
		buffer [p++] = (value >> 8)  & 0xFF;
		buffer [p++] = (value >> 16) & 0xFF;
		buffer [p] = (value >> 24) & 0xFF;

//		printf ("buffer : 0x%.2X 0x%.2X 0x%.2X 0x%.2X\n", buffer[addr], buffer[addr + 1], buffer[addr + 2], buffer[addr + 3]);

		erase ();
		sleep_ms (100);
		write ();
		read();
//		print_buf(buffer, FLASH_PAGE_SIZE);
		
		e_get (addr);
		
	}
}


uint32_t e_get(uint16_t addr)
{
	uint32_t val;
	
	read ();
//	printf ("e_get 0x%.2X : 0x%.2X 0x%.2X 0x%.2X 0x%.2X\n", addr, buffer[addr], buffer[addr + 1], buffer[addr + 2], buffer[addr + 3]);

	val  = buffer[addr++];
	val |= buffer[addr++] << 8;
	val |= buffer[addr++] << 16;
	val |= buffer[addr] << 24;
	
	return val;
}



//              printf("FLASH_PAGE_SIZE = %d\n", FLASH_PAGE_SIZE);             //       FLASH_PAGE_SIZE = 256
//              printf("FLASH_SECTOR_SIZE = %d\n", FLASH_SECTOR_SIZE);           //     FLASH_SECTOR_SIZE = 4096
//              printf("FLASH_BLOCK_SIZE = %d\n", FLASH_BLOCK_SIZE);             //     FLASH_BLOCK_SIZE = 65536
//              printf("PICO_FLASH_SIZE_BYTES = %d\n", PICO_FLASH_SIZE_BYTES);   //     PICO_FLASH_SIZE_BYTES = 2097152
//              printf("XIP_BASE = %d\n", XIP_BASE);                             //    XIP_BASE = 268435456
