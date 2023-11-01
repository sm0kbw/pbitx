// #include <Arduino.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "pico/stdlib.h"
#include "pbitx.h"



#define SAMPLE_PERIOD 	25

bool repeating_timer_callback(struct repeating_timer *t);

//Normal encoder state
static int8_t enc_count = 0;
bool accel_vfo;  
uint32_t time_tick;
static uint16_t speed_cnt = 0;
static int16_t speed = 0;
static int8_t prev_enc;
static int16_t enc_count_periodic = 0;

struct repeating_timer timer;


void enc_setup(void);
uint8_t enc_state (void);
void pci_setup(void);


uint8_t enc_state (void)
{
  return (gpio_get(ENC_A)?1:0 + gpio_get(ENC_B)?2:0);
}



void gpio_callback(uint gpio, uint32_t events) 
{
	int8_t cur_enc;
  
  
	cur_enc = enc_state ();

// interrupt events uint8_ts: 0  Low level, 1  High level, 2  Edge low, 3  Edge high

	if ((gpio == ENC_A || gpio == ENC_B)  &&  (events & 0x4) == 0x4)
	{
		//these transitions point to the enccoder being rotated anti-clockwise
		if ((prev_enc == 0 && cur_enc == 2) || 
			(prev_enc == 2 && cur_enc == 3) || 
			(prev_enc == 3 && cur_enc == 1) || 
			(prev_enc == 1 && cur_enc == 0))
		{
			enc_count -= 1;
			enc_count_periodic -= 1;
		}
		//these transitions point to the enccoder being rotated clockwise
		else 
		if ((prev_enc == 0 && cur_enc == 1) || 
			(prev_enc == 1 && cur_enc == 3) || 
			(prev_enc == 3 && cur_enc == 2) || 
			(prev_enc == 2 && cur_enc == 0))
		{
			enc_count += 1;
			enc_count_periodic += 1;
		}

		prev_enc = cur_enc; // Record state for next pulse interpretation
	}

}

/*
 * Setup the encoder interrupts and global variables.
 */
void pci_setup(void) 
{
	gpio_set_irq_enabled_with_callback(ENC_A, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);
	gpio_set_irq_enabled_with_callback(ENC_B, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);
//	gpio_set_irq_enabled_with_callback(FBUTTON, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &gpio_callback);
//	gpio_set_irq_enabled_with_callback(PTT, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &gpio_callback);   
//	gpio_set_irq_enabled_with_callback(ANALOG_KEYER, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &gpio_callback);
 }

void enc_setup(void)
{
  enc_count = 0;
 
  pci_setup();
  prev_enc = 0;
  add_repeating_timer_us (1000, repeating_timer_callback, NULL, &timer);
}

bool eq_sign (int a, int b)
{
	return ((a >= 0 && b >= 0) || (a <= 0 && b <= 0));
}

bool repeating_timer_callback(struct repeating_timer *t)
{
  time_tick++;
  
  if (speed_cnt++ == SAMPLE_PERIOD)
  {
	if (eq_sign (speed, enc_count_periodic))
		speed = (enc_count_periodic + speed) / 2;
	else
	{
		printf ("diff %d %d\n", speed, enc_count_periodic);
		speed = enc_count_periodic;
	}
	  speed_cnt = 0;
	  enc_count_periodic = 0;
  }
}




int enc_read(void)
{
    int16_t ret;
	int8_t mom_mag;

	if (enc_count != 0)
		ret = enc_count / abs(enc_count);
	else ret = 0;
	
	enc_count = 0;
			
	return ret;
}
