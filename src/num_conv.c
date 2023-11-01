#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "num_conv.h"

	


// BCD related functions:
//
// The BCD coded strings is in the format { 50, 12 , 34 , 34. 4}
// and should interpret to 432 341 250 Hz.




// This function will return the frequency in Hz if the BCD string
// is  terminated with the proper 0xFD

uint32_t bcd2int (uint8_t offset, uint8_t *inque)
{
	uint32_t sum;

	uint8_t i;
	uint32_t mul;
	uint8_t d;

	sum = 0;
	mul = 1;
	i = 0;


	while (inque[i + offset] != 0xFD)
	{
		d = inque[i + offset];
//		printf ("%d | ", d);
		sum += (d & 0xF) * mul;
		mul *= 10;
		sum += (d >> 4) * mul;
		mul *= 10;
		i++;
	}
//	printf ("\n");
	return sum;
}


// This function will slice the frequency given in arg. freq to a BCD string
// placed in str.



void int2bcd (uint32_t f, uint8_t *str)
{
	uint8_t i, d, pos;
	uint32_t n;
	
	
	n = f;
	pos = 0;
	for (i = 0; i < 10;)
	{
		d = n % 10;
		if (pos == 0)
		{	str[i] = d;
			pos = 1;
		}
		else
		{
			str[i++] += d << 4;
			pos = 0;
		}

		n /= 10;
	}
}

