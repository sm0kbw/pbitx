/* KBW_keyer.c is my standard CW-key code. It's just a rather simple state machine for the bugg combined with an even simpler straight keyer control
 *
 * SM0KBW / Bengt
 */

 
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"
#include "pbitx.h"

#define BUG_TR_MUL 		8
#define BRK_IN_DEFAULT	100

#define KEY_TEST_NUM 	4
#define NUM_SAMPLES     8


#define OPEN_VAL 0xE0
#define RPAD_VAL 0xD8
#define LPAD_VAL 0x70
#define BOTH_VAL 0x60


int cwSpeed = 80;
int cwDelayTime = 60;
bool Iambic_Key = true;
uint8_t cw_mode;


struct repeating_timer cw_tone_timer;

static uint8_t paddle_state;
static bool do_dah;
static bool do_dih;
static bool do_space;
bool cw_monitor;

static bool paddle_chg;

static uint16_t d_cnt;
static uint16_t dot_len;
static uint16_t tr_cnt;
static uint16_t tr_delay;
static 	uint slice_num;


void cw_out (void);
bool repeating_timer_callback2(struct repeating_timer *t);
void do_bugg (void);
void do_straight_key (void);
void cwKeyer(void);



void cw_keyer_init (uint32_t frq)
{
	set_cw_speed (cwSpeed);
	tr_delay = BUG_TR_MUL * dot_len;
	set_cw_mon_freq (frq);
	d_cnt = 5 * dot_len;	
	no_tone ();
	set_cw_mon_freq (600);
	
	get_paddle_state ();
	
	if (paddle_state == 0x00)
	{
		cw_mode = STRAIGHT;
		tr_delay = 40;
	}
	else
	if (paddle_state == 0x10)
	{
		cw_mode = DUMB_BUGG;
		Iambic_Key = false;
		set_cw_speed (cwSpeed);
	}
	else
	if (paddle_state == 0x01)
	{
		cw_mode = IAMBIC_BUGG;
		Iambic_Key = true;
		set_cw_speed (cwSpeed);
	}
}


void set_cw_speed (uint16_t spd)
{
	dot_len = 500 / spd; 
	
	if (cw_mode != STRAIGHT)
		tr_delay = 5 * dot_len;
}




uint16_t pwm_set_freq_duty(uint slice_num, uint chan,uint32_t f)
{
	uint wrap;
	
	wrap = (32000000 / f) - 1;
	pwm_set_clkdiv_int_frac (slice_num,  4, 0);
	pwm_set_wrap(slice_num, wrap);
	pwm_set_chan_level(slice_num, chan, wrap / 2);
	
	printf ("wrap %d\n", wrap);
	return wrap;
}


void set_cw_mon_freq (uint32_t frq)
{	
//	uint16_t wrap;
	uint chan; 

	printf ("CW sidetone %ld\n", frq);

    slice_num = pwm_gpio_to_slice_num(CW_TONE);
    chan = pwm_gpio_to_channel(CW_TONE);
    pwm_set_freq_duty(slice_num,chan, frq);
}



void do_cw (void)
{
//	printf ("do_cw, mode %s\n", ((cw_mode == BUGG) ? "bugg"  : "key"));
	if (cw_mode != STRAIGHT)
	{
		do_bugg ();
	}
	else
	{
		do_straight_key ();
	}
}



void do_bugg (void)
{
	if (d_cnt != 0)
		d_cnt--;

	if (tr_cnt != 0 && !inTx)
	{
		startTx(false);
	}
	else
	if (tr_cnt == 0 && inTx)
		stopTx (false);
	else
	if (tr_cnt != 0)
		tr_cnt--;
	
	if (d_cnt == 0)
	{
		if (do_space)
		{
			d_cnt = dot_len;
			gpio_put(CW_KEY, OPEN_KEY);
			no_tone ();
			tr_cnt = tr_delay;
			do_space = false;
		}
	else
		{
			cwKeyer ();
		}
		cw_out ();
	}
}
	
	




// The actual bugg procedure
void cwKeyer (void)
{	
	// Decide what to do dependig on CW states and paddle status
	// A bugg is indeed a state machine!
	get_paddle_state ();
	{
		switch (paddle_state)
		{

			case 0x00:
				do_dih = do_dah = false;
				break;
			
			case 0x01: 
				do_dih = false;
				do_dah = true;
				break;
			
			case 0x10:
				do_dih = true;
				do_dah = false;
				break;
				
			case 0x11:
				if (Iambic_Key != 0)
				{
					if (do_dah)
					{
						do_dih = true;
						do_dah = false;
					}
					else
					if (do_dih)
					{
						do_dah = true;
						do_dih = false;
					}
					else
						do_dah = true;
				}
				break;	
		}
	}	
		// paddle states recognized - clear for next change		
	paddle_state = 0;		
}	




// Hardware layer control of the  state 
void cw_out (void)
{

	// CW output
	if (d_cnt == 0 && do_space == 0)
	{
		if (do_dih)
		{
			d_cnt = dot_len;
			gpio_put(CW_KEY, CLOSED_KEY);
			do_space = true;
			tone ();
	 		tr_cnt = tr_delay;
		}
		else
		if (do_dah)
		{
			d_cnt = 3 * dot_len;
			gpio_put(CW_KEY, CLOSED_KEY);
			do_space = true;
			tone ();
	 		tr_cnt = tr_delay;
		}
	}
}


void get_paddle_state (void)
{
	uint16_t adc_val;
	static uint8_t lr_cnt, r_pad_cnt, l_pad_cnt;
	static bool prev_lpad, prev_rpad, prev_lr;
	static uint8_t i;
	
	lr_cnt = l_pad_cnt = r_pad_cnt = 0;
	
	for (i = 0; i  < NUM_SAMPLES; i++)
	{
		adc_val = analogRead (ANALOG_KEYER);
		adc_val >>= 4;
		
		if (adc_val < OPEN_VAL)
		{
			if (adc_val < BOTH_VAL)
				lr_cnt++;
			else
			if (adc_val < LPAD_VAL)
				l_pad_cnt++;
			else
			if (adc_val < RPAD_VAL)
				r_pad_cnt++;
		}		
	}
	paddle_state = 0x00;

	if (lr_cnt > KEY_TEST_NUM)
	{
		paddle_state = 0x11;
		paddle_chg = (!prev_lr) ? true : false;
		prev_lr = true;
	}
	else
	if (l_pad_cnt > KEY_TEST_NUM)
	{
		paddle_chg = (!prev_lpad) ? true : false;
		paddle_state = 0x10;
		prev_lpad = true;
	}
	else
	if (r_pad_cnt > KEY_TEST_NUM)
	{
		paddle_chg = (!prev_rpad) ? true : false;
		paddle_state = 0x01;
		prev_rpad = true;
	}
	else
	if (lr_cnt < KEY_TEST_NUM)
	{
		paddle_chg = (prev_lr) ? true : false;
		prev_lr = false;
	}
	else
	if (l_pad_cnt == 0)
	{
		paddle_chg = (prev_lpad) ? true : false;
		prev_lpad = false;
	}
	else
	if (r_pad_cnt == 0)
	{
		paddle_chg = (prev_rpad) ? true : false;
		prev_rpad = false;
	}
	r_pad_cnt = l_pad_cnt = lr_cnt = 0;
}



void tone (void)
{
    pwm_set_enabled(slice_num, true);
	cw_monitor = true;
}


void no_tone (void)
{
    pwm_set_enabled(slice_num, false);
	cw_monitor = false;
}


void do_straight_key (void)
{
	uint16_t adc_val;
	uint8_t i, state_cnt;
	
	
	if (tr_cnt != 0 && !inTx)
	{
		startTx(false);
	}
	else
	if (tr_cnt == 0 && inTx)
		stopTx (false);
	else
	if (tr_cnt != 0)
		tr_cnt--;
	
	state_cnt = 0;
	
	for (i = 0; i  < NUM_SAMPLES; i++)
	{
		adc_val = analogRead (ANALOG_KEYER);
		adc_val >>= 4;
		if (adc_val < RPAD_VAL)
		state_cnt++;
	}
	
	if (state_cnt > 5) 
	{
		tr_cnt = tr_delay;
		tone ();
		gpio_put(CW_KEY, CLOSED_KEY);
	}
	else
	if (state_cnt == 0)
	{
		no_tone ();
		gpio_put(CW_KEY, OPEN_KEY);
	}
}

