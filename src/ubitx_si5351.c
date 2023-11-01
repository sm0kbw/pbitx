#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "pico/binary_info.h"
#include "hardware/i2c.h"
#include "pico/stdlib.h"
#include "pbitx.h"
#include "e_storage.h"




// *************  SI5315 routines - tks Jerry Gaffke, KE7ER   ***********************

// An minimalist standalone set of Si5351 routines.
// VCOA is fixed at 875mhz, VCOB not used.
// The output msynth dividers are used to generate 3 independent clocks
// with 1hz resolution to any frequency between 4khz and 109mhz.

// Usage:
// Call si5351bx_init() once at startup with no args;
// Call si5351bx_setfreq(clknum, freq) each time one of the
// three output CLK pins is to be updated to a new frequency.
// A freq of 0 serves to shut down that output clock.

// The global variable si5351bx_vcoa starts out equal to the nominal VCOA
// frequency of 25mhz*35 = 875000000 Hz.  To correct for 25mhz crystal errors,
// the user can adjust this value.  The vco frequency will not change but
// the number used for the (a+b/c) output msynth calculations is affected.
// Example:  We call for a 5mhz signal, but it measures to be 5.001mhz.
// So the actual vcoa frequency is 875mhz*5.001/5.000 = 875175000 Hz,
// To correct for this error:     si5351bx_vcoa=875175000;

// Most users will never need to generate clocks below 500khz.
// But it is possible to do so by loading a value between 0 and 7 into
// the global variable si5351bx_rdiv, be sure to return it to a value of 0
// before setting some other CLK output pin.  The affected clock will be
// divided down by a power of two defined by  2**si5351_rdiv
// A value of zero gives a divide factor of 1, a value of 7 divides by 128.
// This lightweight method is a reasonable compromise for a seldom used feature.


#define BB0(x) ((uint8_t)x)             // Bust int32 into Bytes
#define BB1(x) ((uint8_t)(x>>8))
#define BB2(x) ((uint8_t)(x>>16))

#define SI5351BX_ADDR 0x60              // I2C address of Si5351   (typical)
#define SI5351BX_XTALPF 2               // 1:6pf  2:8pf  3:10pf

// If using 27mhz crystal, set XTAL=27000000, MSA=33.  Then vco=891mhz
#define SI5351BX_XTAL 25000000          // Crystal freq in Hz
#define SI5351BX_MSA  35                // VCOA is at 25mhz*35 = 875mhz

// User program may have reason to poke new values into these 3 RAM variables
uint32_t si5351bx_vcoa = (SI5351BX_XTAL*SI5351BX_MSA);  // 25mhzXtal calibrate
uint8_t  si5351bx_rdiv = 0;             // 0-7, CLK pin sees fout/(2**rdiv)
uint8_t  si5351bx_drive[3] = {3, 3, 3}; // 0=2ma 1=4ma 2=6ma 3=8ma for CLK 0,1,2
uint8_t  si5351bx_clken = 0xFF;         // Private, all CLK output drivers off
int32_t calibration = 11850;

void i2cWriten(uint8_t reg, uint8_t *vals, uint8_t vcnt); 
void i2cWrite(uint8_t reg, uint8_t val);


void set_calibration (uint32_t cal)
{
	calibration = cal;
	e_put(MASTER_CAL, cal);
}


uint32_t get_calibration (void)
{
	return calibration;
}


void load_calibration (void)
{
	calibration = e_get (MASTER_CAL);
}


void i2cWrite(uint8_t reg, uint8_t val)
{  
	i2cWriten(reg, &val, 1); 
}

void i2cWriten(uint8_t reg, uint8_t *vals, uint8_t vcnt) 
{  

	uint8_t buff[20];
	uint8_t i, len;
	
	buff[0] = reg;

	len = vcnt + 1;
	
	for (i = 1; i < len; i++)
	{
		buff[i] = *vals++;
	}
	i2c_write_blocking (i2c1, SI5351BX_ADDR, buff, (size_t)len, false);			
 }



// Call once at power-up, start PLL
void si5351bx_init(void) 
{
//	uint8_t reg;
	uint32_t msxp1;
	

	i2c_init (i2c1, 100 * 1000);	
	
//	printf ("setting up i2c1 over pins GP14 and GP15, that is over I2C_SDA and I2C_SCL\nreturned %d\n", n);
	
	gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
	gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
	gpio_pull_up(I2C_SDA);
	gpio_pull_up(I2C_SCL);
	
	bi_decl(bi_2pins_with_func(I2C_SDA, I2C_SCL, GPIO_FUNC_I2C));
//	printf ("sending val 0 to port 0x95\n");
	sleep_ms (100);
	i2cWrite(0x95, 0);                     // SpreadSpectrum off

//	printf ("sending val %d to port 0x3\n", si5351bx_clken);
	
	i2cWrite(3, si5351bx_clken);          // Disable all CLK output drivers
	i2cWrite(0xB7, SI5351BX_XTALPF << 6);  // Set 25mhz crystal load capacitance
	msxp1 = 128 * SI5351BX_MSA - 512;     // and msxp2=0, msxp3=1, not fractional
	uint8_t  vals[8] = {0, 1, BB2(msxp1), BB1(msxp1), BB0(msxp1), 0, 0, 0};
	i2cWriten(0x1A, vals, 8);               // Write to 8 PLLA msynth regs
	i2cWrite(0xB1, 0x20);                  // Reset PLLA  (0x80 resets PLLB)
	i2cWriten(0x22, vals, 8);               // Write to 8 PLLA msynth regs
	i2cWrite(0xB1, 0xA0);                  // Reset PLLA  & PPLB (0x80 resets PLLB)
}


// Set a CLK to fout Hz
void si5351bx_setfreq(uint8_t clknum, uint32_t fout) 
{
	uint32_t  msa, msb, msc, msxp1, msxp2, msxp3p2top;
	
	if ((fout < 500000) || (fout > 109000000)) // If clock freq out of range
		si5351bx_clken |= 1 << clknum;      //  shut down the clock
	else
	{
		msa = si5351bx_vcoa / fout;     // Integer part of vco/fout
		msb = si5351bx_vcoa % fout;     // Fractional part of vco/fout
		msc = fout;                      // Divide by 2 till fits in reg
		
		while (msc & 0xfff00000)
		{
			msb = msb >> 1;
			msc = msc >> 1;
		}
		
		msxp1 = (128 * msa + 128 * msb / msc - 512) | (((uint32_t)si5351bx_rdiv) << 20);
		msxp2 = 128 * msb - 128 * msb / msc * msc; // msxp3 == msc;
		msxp3p2top = (((msc & 0x0F0000) << 4) | msxp2);     // 2 top nibbles
		uint8_t vals[8] = { BB1(msc), BB0(msc), BB2(msxp1), BB1(msxp1), BB0(msxp1), BB2(msxp3p2top), BB1(msxp2), BB0(msxp2)};
		i2cWriten(42 + (clknum * 8), vals, 8); // Write to 8 msynth regs
		i2cWrite(16 + clknum, 0x0C | si5351bx_drive[clknum]); // use local msynth
		
		si5351bx_clken &= ~(1 << clknum);   // Clear uint8_t to enable clock
	}
	
	i2cWrite(3, si5351bx_clken);        // Enable/disable clock
}


void si5351_set_calibration(int32_t cal)
{
    si5351bx_vcoa = (SI5351BX_XTAL * SI5351BX_MSA) + cal; // apply the calibration correction factor
    si5351bx_setfreq(0, usbCarrier);
}

void initOscillators(void)
{
  //initialize the SI5351
  si5351bx_init();
  si5351bx_vcoa = (SI5351BX_XTAL * SI5351BX_MSA) + calibration; // apply the calibration correction factor
  si5351bx_setfreq(0, usbCarrier);
}



