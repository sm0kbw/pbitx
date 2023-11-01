/**
 * This source file is under General Public License version 3.
 * 
 * This verision uses a built-in Si5351 library
 * Most source code are meant to be understood by the compilers and the computers. 
 * Code that has to be hackable needs to be well understood and properly documented. 
 * Donald Knuth coined the term Literate Programming to indicate code that is written be 
 * easily read and understood.
 * 
 * The Raduino is a small board that includes the Arduin Nano, a TFT display and
 * an Si5351a frequency synthesizer. This board is manufactured by HF Signals Electronics Pvt Ltd
 * 
 * To learn more about Arduino you may visit www.arduino.cc. 
 * 
 * The Arduino works by starts executing the code in a function called setup() and then it 
 * repeatedly keeps calling loop() forever. All the initialization code is kept in setup()
 * and code to continuously sense the tuning knob, the function button, transmit/receive,
 * etc is all in the loop() function. If you wish to study the code top down, then scroll
 * to the bottom of this file and read your way up.
 * 
 * Below are the libraries to be included for building the Raduino 
 * The EEPROM library is used to store settings like the frequency memory, caliberation data, etc.
 *
 *  The main chip which generates upto three oscillators of various frequencies in the
 *  Raduino is the Si5351a. To learn more about Si5351a you can download the datasheet 
 *  from www.silabs.com although, strictly speaking it is not a requirment to understand this code. 
 *  Instead, you can look up the Si5351 library written by xxx, yyy. You can download and 
 *  install it from www.url.com to complile this file.
 *  The Wire.h library is used to talk to the Si5351 and we also declare an instance of 
 *  Si5351 object to control the clocks.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/uart.h"
#include "pico/multicore.h"
//#include "ugui/ugui.h"
#include "pbitx.h"
#include "e_storage.h"
#include "gui_driver.h"
#include "dispatch.h"


/**
    The main chip which generates upto three oscillators of various frequencies in the
    Raduino is the Si5351a. To learn more about Si5351a you can download the datasheet
    from www.silabs.com although, strictly speaking it is not a requirment to understand this code.

    We no longer use the standard SI5351 library because of its huge overhead due to many unused
    features consuming a lot of program space. Instead of depending on an external library we now use
    Jerry Gaffke's, KE7ER, lightweight standalone mimimalist "si5351bx" routines (see further down the
    code). Here are some defines and declarations used by Jerry's routines:
*/



/**
 * The uuint8_tX is an upconnversion transceiver. The first IF is at 45 MHz.
 * The first IF frequency is not exactly at 45 Mhz but about 5 khz lower,
 * this shift is due to the loading on the 45 Mhz crystal filter by the matching
 * L-network used on it's either sides.
 * The first oscillator works between 48 Mhz and 75 MHz. The signal is subtracted
 * from the first oscillator to arriive at 45 Mhz IF. Thus, it is inverted : LSB becomes USB
 * and USB becomes LSB.
 * The second IF of 12 Mhz has a ladder crystal filter. If a second oscillator is used at 
 * 57 Mhz, the signal is subtracted FROM the oscillator, inverting a second time, and arrives 
 * at the 12 Mhz ladder filter thus doouble inversion, keeps the sidebands as they originally were.
 * If the second oscillator is at 33 Mhz, the oscilaltor is subtracated from the signal, 
 * thus keeping the signal's sidebands inverted. The USB will become LSB.
 * We use this technique to switch sidebands. This is to avoid placing the lsbCarrier close to
 * 12 MHz where its fifth harmonic beats with the arduino's 16 Mhz oscillator's fourth harmonic
 */


//we directly generate the CW by programmin the Si5351 to the cw tx frequency, hence, both are different modes
//these are the parameter passed to startTx

bool		ritOn = 0;
uint8_t 	active_vfo = VFO_A;
uint32_t	vfo_a_freq=7150000L, vfo_b_freq=14200000L;
uint32_t 	sideTone=800, usbCarrier;
uint8_t	 	mode_vfoa=LSB, mode_vfob=USB;
uint32_t 	frequency, ritRxFrequency, ritTxFrequency;  //frequency is the current frequency on the dial
uint32_t  	firstIF =   45005000L;
uint32_t 	cwTimeout = 0;       	// milliseconds to go before the cw transmit line is released and the radio goes back to rx mode

bool 		doingCAT = false;		// during CAT commands, we will freeeze the display until CAT is disengaged
bool 		txCAT = false;       	// turned on if the transmitting due to a CAT command
bool 		inTx = false;        	// it is set to 1 if in transmit mode (whatever the reason : cw, ptt or cat)
bool 		split_on = false;     	// working split, uses VFO B as the transmit frequency
bool		keyDown = false;     	// in cw mode, denotes the carrier is being transmitted
uint8_t		mode = USB;       	// upper sideband was selected, this is reset to the default for the frequency when it crosses the frequency border of 10 MHz
bool		menuOn = false;          	// set to 1 when the menu is being displayed, if a menu item sets it to zero, the menu is exited
bool 		modeCalibrate = false; 	//this mode of menus shows extended menus to calibrate the oscillators and choose the proper beat frequency<
bool 		sweep_on = false;									

// struct repeating_timer panorama_timer;

// bool repeating_timer_callback_pan(struct repeating_timer *t);

void setTXFilters(unsigned long freq);

// infinity loop controlling the radio
void loop(void);

/**
 * Below are the basic functions that control the uuint8_tx. Understanding the functions before 
 * you start hacking around
 */


// copy contents to VFO v from alternative VFO
void eq_vfo_ab (uint8_t v)
{
	if (v == VFO_A)
	{
		vfo_a_freq = vfo_b_freq;
		mode = VFO_B_MODE;
	}
	else
	{
		vfo_b_freq = vfo_a_freq;
		mode = VFO_A_MODE;
	}
	
	setfrequency (vfo_a_freq);
	
}

void sel_active_vfo (uint8_t v)
{
	if (active_vfo != v)
	{
		if (v == VFO_A)
		{
			frequency = vfo_a_freq;
			mode = VFO_A_MODE;
		}
		else
		{
			frequency = vfo_b_freq;
			mode = VFO_B_MODE;
				
		}
	}
}
void saveVFOs(void)
{
	if (active_vfo == VFO_A)
		e_put(VFO_A, frequency);
	else
		e_put(VFO_A, vfo_a_freq);
	
	if (mode_vfoa == USB)
		e_put(VFO_A_MODE, VFO_MODE_USB);
	else
		e_put(VFO_A_MODE, VFO_MODE_LSB);
	
	if (active_vfo == VFO_B)
		e_put(VFO_B, frequency);
	else
		e_put(VFO_B, vfo_b_freq);
	
	if (mode_vfob == USB)
		e_put(VFO_B_MODE, VFO_MODE_USB);
	else 
		e_put(VFO_B_MODE, VFO_MODE_LSB);
}

/**
 * Select the properly tx harmonic filters
 * The four harmonic filters use only three relays
 * the four LPFs cover 30-21 Mhz, 18 - 14 Mhz, 7-10 MHz and 3.5 to 5 Mhz
 * Briefly, it works like this, 
 * - When KT1 is OFF, the 'off' position routes the PA output through the 30 MHz LPF
 * - When KT1 is ON, it routes the PA output to KT2. Which is why you will see that
 *   the KT1 is on for the three other cases.
 * - When the KT1 is ON and KT2 is off, the off position of KT2 routes the PA output
 *   to 18 MHz LPF (That also works for 14 Mhz) 
 * - When KT1 is On, KT2 is On, it routes the PA output to KT3
 * - KT3, when switched on selects the 7-10 Mhz filter
 * - KT3 when switched off selects the 3.5-5 Mhz filter
 * See the circuit to understand this
 */

void setTXFilters(unsigned long freq)
{
	 // the default filter is with 35 MHz cut-off
	if (freq > 21000000L)
	{ 
		gpio_put(TX_LPF_A, 0);
		gpio_put(TX_LPF_B, 0);
		gpio_put(TX_LPF_C, 0);
	}
	 //thrown the KT1 relay on, the 30 MHz LPF is bypassed and the 14-18 MHz LPF is allowd to go through
	else 
	if (freq >= 14000000L)
	{
		gpio_put(TX_LPF_A, 1);
		gpio_put(TX_LPF_B, 0);
		gpio_put(TX_LPF_C, 0);
    }
	else
	if (freq > 7000000L)
	{
		gpio_put(TX_LPF_A, 0);
		gpio_put(TX_LPF_B, 1);
		gpio_put(TX_LPF_C, 0);    
	}
	else
	{
		gpio_put(TX_LPF_A, 0);
		gpio_put(TX_LPF_B, 0);
		gpio_put(TX_LPF_C, 1);    
	}
}

#define MID_FILTER 11057500l
//#define MID_FILTER usbCarrier
#define PAN_SPAN	200000l
//#define STEP	400

void get_pan_data (void)
{ 
	unsigned long ff;
	uint16_t i, val, s1, s2, s3;
	uint32_t step;
//	printf ("%ld | %ld\n", lsbCarrier, usbCarrier);
	step = PAN_SPAN / PAN_SZ;
	ff = frequency - (PAN_SZ/2) * step;

	adc_select_input(PAN_SPEC);

	si5351bx_setfreq(1, firstIF + MID_FILTER);
	si5351bx_setfreq(0, MID_FILTER);

	for (i = 0; i < PAN_SZ; i++)
	{
		si5351bx_setfreq(2, firstIF + ff);
		ff += step;

		s1 = adc_read ();
		s2 = adc_read ();
		s3 = adc_read ();

		val = (s1 + s2 + s3) / 3;
		pan_data[i] = (val + pan_data[i]) / 2;
			
	}
	
	si5351bx_setfreq(0, usbCarrier);
	si5351bx_setfreq(2, firstIF + frequency);
}


/**
 * This is the most frequently called function that configures the 
 * radio to a particular frequeny, sideband and sets up the transmit filters
 * 
 * The transmit filter relays are powered up only during the tx so they dont
 * draw any current during rx. 
 * 
 * The carrier oscillator of the detector/modulator is permanently fixed at
 * uppper sideband. The sideband selection is done by placing the second oscillator
 * either 12 Mhz below or above the 45 Mhz signal thereby inverting the sidebands 
 * through mixing of the second local oscillator.
 */

 
uint32_t getfrequency (void)
{
	return frequency;
}


void setfrequency(uint32_t f)
{
//	uint64_t osc_f, firstOscillator, secondOscillator;

	si5351bx_setfreq(2, firstIF + f);
	
	setTXFilters(f);

	if (mode == USB)
		si5351bx_setfreq(1, firstIF + usbCarrier);
	else
	if (mode == LSB)
		si5351bx_setfreq(1, firstIF - usbCarrier);
	else
	if (mode == CW) 
	{
		if (inTx)
		{
			// Turn off osc 0 and osc 1
			si5351bx_setfreq(0, 0);
			si5351bx_setfreq(1, 0);
			// set osc 2, first oscillator to tx frequency			
			si5351bx_setfreq(2, (f + sideTone));

		}
		else
		{			
			// reset to normal receive settings
			si5351bx_setfreq(2, firstIF + f);
			si5351bx_setfreq(1, firstIF + usbCarrier);
		}
			
	}
	frequency = f;
}



bool in_split (void)
{
	return split_on;
}



uint32_t split_freq (void)
{
	uint32_t f;
	
	if (in_split ())
	{
		if (active_vfo == VFO_A)
			f = vfo_b_freq;
		else
			f = vfo_a_freq;
	}
	else
		f = frequency;
	
	return f;
	
}
/**
 * startTx is called by the PTT, cw keyer and CAT protocol to
 * put the uuint8_tx in tx mode. It takes care of rit settings, sideband settings
 * Note: In cw mode, doesnt key the radio, only puts it in tx mode
 * CW offest is calculated as lower than the operating frequency when in LSB mode, and vice versa in USB mode
 */
 bool soft_ptt;
 
void startTx(bool use_soft)
{
	if (use_soft) 
		soft_ptt = true; 
	gpio_put(TX_RX, 1);
	inTx = true;
	
	if (ritOn)
	{
		//save the current as the rx frequency
		ritRxFrequency = frequency;
		setfrequency(ritTxFrequency);
	}
	else 
	{
		if (split_on)
		{
			if (active_vfo == VFO_B)
			{
				active_vfo = VFO_A;
				mode = mode_vfoa;
				frequency = vfo_a_freq;
			}
			else
			if (active_vfo == VFO_A)
			{
				active_vfo = VFO_B;
				frequency = vfo_b_freq;
				mode = mode_vfob;        
			}
		}
		
	}

	setfrequency(frequency);
	
	gpio_put(TX_RX, 1);     
	drawTx();
	updateDisplay(KEEP_VFO);
//	printf ("startTx : txMode %d : mode %d : inTx %d\n", txMode, mode, inTx);
}



void stopTx(bool use_soft)
{
	if ((!use_soft && !soft_ptt) ||  (soft_ptt && use_soft))
	{
		inTx = false;
		if (soft_ptt)
			soft_ptt = false;
		
		
		gpio_put(TX_RX, 0);           //turn off the tx

		si5351bx_setfreq(0, usbCarrier);  //set back the cardrier oscillator anyway, cw tx switches it off
	
		if (ritOn)
		setfrequency(ritRxFrequency);
		else
		{
			if (split_on) 
			{
				//vfo Change
				if (active_vfo == VFO_B)
				{
					active_vfo = VFO_A;
					frequency = vfo_a_freq;
					mode = mode_vfoa;        
				}
				else
				if (active_vfo == VFO_A)
				{
					active_vfo = VFO_B;
					frequency = vfo_b_freq;
					mode = mode_vfob;        
				}
			}
		
			setfrequency(frequency);
		}

		updateDisplay(KEEP_VFO);
		drawTx();
		printf ("StopTx : inTx %d : mode %d\n", inTx, mode);
	}
}

/**
 * ritEnable is called with a frequency parameter that determines
 * what the tx frequency will be
 */
void ritEnable(uint32_t f)
{
	ritOn = true;
	//save the non-rit frequency back into the VFO memory
	//as RIT is a temporary shift, this is not saved to EEPROM
	ritTxFrequency = f;
}

// this is called by the RIT menu routine
void ritDisable(void)
{
	if (ritOn)
	{
		ritOn = false;
		setfrequency(ritTxFrequency);
		updateDisplay(KEEP_VFO);
	}
}

/**
 * Basic User Interface Routines. These check the front panel for any activity
 */

/**
 * The PTT is checked only if we are not already in a cw transmit session
 * If the PTT is pressed, we shift to the ritbase if the rit was on
 * flip the T/R line to T and update the display to denote transmission
 */

void checkPTT(void)
{	
	//we don't check for ptt when transmitting cw
	if (cwTimeout == 0)
	{
		if (!gpio_get(PTT) && !inTx)
		{
			startTx(false);
			sleep_ms (10); //debounce the PTT
		}
		else
		if (gpio_get(PTT) && inTx)
			stopTx(false);
	}
}




//check if the encoder button was pressed
bool checkButton(void)
{
//	int i, t1, t2, knob, new_knob;
	
	//only if the button is pressed
	if (!btnDown())
		return false;
	
	sleep_ms (10);
	
	if (!btnDown()) //debounce
		return false;
	
	//disengage any CAT work
	doingCAT = false;
	
	int downTime = 0;
	while(btnDown())
	{
		sleep_ms (10);
		downTime++;
		if (downTime > 300)
		{
			doSetup2();
			return true;
		}
	}
	sleep_ms (50);
	
	
	//wait for the button to go up again
	while(btnDown())
		sleep_ms (20);
	
	sleep_ms(20);//debounce
	
	return true;
}


uint32_t get_vfo_freq (uint8_t v)
{
	return (v == VFO_A) ? vfo_a_freq : vfo_b_freq;
}



void swap_vfo (void)
{
	uint32_t f;
	
	f = vfo_a_freq;
	vfo_a_freq = vfo_b_freq;
	vfo_b_freq = f;

	if (active_vfo == VFO_A)
		setfrequency (vfo_a_freq);
	else
		setfrequency (vfo_b_freq);
	guiUpdate (true);
}

void switchVFO(int vfoselect)
{
	if (vfoselect == VFO_A)
	{
		if (active_vfo == VFO_B)
		{
			vfo_b_freq = frequency;
			mode_vfob = mode;
			e_put(VFO_B, frequency);
			if (mode_vfob)
				e_put(VFO_B_MODE, VFO_MODE_USB);
			else
				e_put(VFO_B_MODE, VFO_MODE_LSB);
		}
		active_vfo = VFO_A;
      frequency = vfo_a_freq;
      mode = mode_vfoa;
    }
    else
	{
		if (active_vfo == VFO_A)
		{
			vfo_a_freq = frequency;
			mode_vfoa = mode;
			e_put(VFO_A, frequency);
			if (mode_vfoa)
				e_put(VFO_A_MODE, VFO_MODE_USB);
			else
				e_put(VFO_A_MODE, VFO_MODE_LSB);
		}
		active_vfo = VFO_B;
//      printLine2("Selected VFO B  ");      
		frequency = vfo_b_freq;
		mode = mode_vfob;
    }

    setfrequency(frequency);
//    redraw_menus();
	displayVFO (CLEAR_VFO);
	saveVFOs();
}

/**
 * The tuning jumps by 50 Hz on each step when you tune slowly
 * As you spin the encoder faster, the jump size also increases 
 * This way, you can quickly move to another band by just spinning the 
 * tuning knob
 */

void doTuning(void)
{
	int s;
	static uint32_t prev_freq;
	static uint32_t nextFrequencyUpdate = 0;
	
	uint32_t now;
	
	now = millis();

	if (now >= nextFrequencyUpdate && prev_freq != frequency)
	{
		updateDisplay(KEEP_VFO);
		nextFrequencyUpdate = now + 500;
		prev_freq = frequency;
	}
	
	s = enc_read();
//	printf ("accel s = %d ", s);

	
	if (accel_vfo)
	{
		doingCAT = false; // go back to manual mode if you were doing CAT
		if (s > 0)
			frequency += 2000l;
		else
		if (s < 0)
			frequency -= 2000l;
	}	
	else
	{	
		if (s > 10)
			frequency += 200l * s;

		else 
		if (s > 5)
			frequency += 100l * s;

		else
		if (s > 0)
			frequency += 50l * s;
		else
		if (s < -10)
			frequency += 200l * s;
		else
		if (s < -5)
			frequency += 100l * s;
		else
		if (s  < 0)
			frequency += 50l * s;
	
		if (prev_freq < 10000000l && frequency > 10000000l)
			mode = USB;
	
		if (prev_freq > 10000000l && frequency < 10000000l)
			mode = LSB;

		setfrequency(frequency);    

	}
}


/**
 * RIT only steps back and forth by 100 hz at a time
 */
void doRIT(void)
{
//	unsigned long newFreq;
	
	int knob = enc_read();
	unsigned long old_freq = frequency;
	
	if (knob < 0)
		frequency -= 100l;
	else 
	if (knob > 0)
		frequency += 100;
	
	if (old_freq != frequency)
	{
		setfrequency(frequency);
		updateDisplay(KEEP_VFO);
	}
}

/**



 * The settings are read from EEPROM. The first time around, the values may not be 
 * present or out of range, in this case, some intelligent defaults are copied into the 
 * variables.
 */
void initSettings(void)
{
	uint8_t x;
	//read the settings from the eeprom and restore them
	//if the readings are off, then set defaults

	load_calibration ();
	usbCarrier = e_get(USB_CAL);
	vfo_a_freq = e_get(VFO_A);
	vfo_b_freq = e_get(VFO_B);
	sideTone = e_get(CW_SIDETONE);
	cwSpeed = e_get(CW_SPEED);
	cwDelayTime = e_get(CW_DELAYTIME);
	x = e_get (CW_KEY_TYPE);
	cw_mode = (x < 2) ? x : 0;
//	printf ("saved settings:\n calib %d usbCar %d\nvfoA %d vfo_b_freq %d\nsidetone %d CWspd %d cwDly %d\n" ,calibration, usbCarrier, vfo_a_freq, vfo_b_freq, sideTone, cwSpeed, cwDelayTime);
	
	// the screen calibration parameters : int slope_x=104, slope_y=137, offset_x=28, offset_y=29;
	
	if (usbCarrier > 11060000l || usbCarrier < 11048000l)
		usbCarrier = 11052000l;

	if (vfo_a_freq > 35000000l || 3500000l > vfo_a_freq)
		vfo_a_freq = 7150000l;

	if (vfo_b_freq > 35000000l || 3500000l > vfo_b_freq)
		vfo_b_freq = 14150000l;  

	if (sideTone < 100 || 2000 < sideTone) 
		sideTone = 800;

	if (cwSpeed < 10 || 1000 < cwSpeed) 
		cwSpeed = 100;

	if (cwDelayTime < 10 || cwDelayTime > 100)
		cwDelayTime = 50;
	
	/*
	* The VFO modes are read in as either 2 (USB) or 3(LSB), 0, the default
	* is taken as 'uninitialized
	*/
	
	x= e_get(VFO_A_MODE);
	
	switch(x)
	{
		case VFO_MODE_USB:
			mode_vfoa = true;
			break;

		case VFO_MODE_LSB:
			mode_vfoa = false;
			break;

		default:
			if (vfo_a_freq > 10000000l)
				mode_vfoa = true;
			else 
				mode_vfoa = false; 
		break;
	}
	
	x = e_get(VFO_B_MODE);
	
	switch(x)
	{
		case VFO_MODE_USB:
			mode_vfob = true;
			break;

		case VFO_MODE_LSB:
			mode_vfob = false;
			break;

		default:
			if (vfo_a_freq > 10000000l)
				mode_vfob = true;
			else 
				mode_vfob = false;      
			break;
	}
	
	//set the current mode
	mode = mode_vfoa;
	
	/*
	* The keyer type splits into two variables
	*/
	cw_mode = e_get(CW_KEY_TYPE);
	
	
//	if (x == 0)
//	Iambic_Key = false;
//	
//	else if (x == 1)
//	{
//	Iambic_Key = true;
//	keyerControl &= ~IAMBICB;
//	}
//	else if (x == 2)
//	{
//	Iambic_Key = true;
//	keyerControl |= IAMBICB;
//	}
//	printf ("actual settings:\n usbCar %d\nvfoA %d vfo_b_freq %d\nsidetone %d CWspd %d cwDly %d\n", usbCarrier, vfo_a_freq, vfo_b_freq, sideTone, cwSpeed, cwDelayTime);
}

void initPorts(void)
{
	gpio_init (ENC_A);
	gpio_set_dir (ENC_A, GPIO_IN);
	gpio_pull_up (ENC_A); 
	
	gpio_init (ENC_B);
	gpio_set_dir (ENC_B, GPIO_IN);
	gpio_pull_up (ENC_B); 
	
	gpio_init (FBUTTON);
	gpio_set_dir (FBUTTON, GPIO_IN);
	gpio_pull_up (FBUTTON);    
	
	gpio_init (PTT);
	gpio_set_dir (PTT, GPIO_IN);
	gpio_pull_up (PTT);    

    gpio_set_function(CW_TONE, GPIO_FUNC_PWM);

//	gpio_init (CW_TONE);
//	gpio_set_dir (CW_TONE, GPIO_IN);
//	gpio_put(CW_TONE, 0);
	
	gpio_init (TX_RX);
	gpio_set_dir (TX_RX, GPIO_OUT);    
	gpio_put(TX_RX, 0);
	
	gpio_init (TX_LPF_A);
	gpio_set_dir (TX_LPF_A, GPIO_OUT);    
	
	gpio_init (TX_LPF_B);
	gpio_set_dir (TX_LPF_B, GPIO_OUT);    
	
	gpio_init (TX_LPF_C);
	gpio_set_dir (TX_LPF_C, GPIO_OUT);    

	gpio_put(TX_LPF_A, 0);
	gpio_put(TX_LPF_B, 0);
	gpio_put(TX_LPF_C, 0);
	
	gpio_init (CW_KEY);
	gpio_set_dir (CW_KEY, GPIO_OUT);    
	gpio_put(CW_KEY, OPEN_KEY);
	
	gpio_init (UNUSED_A);
	gpio_set_dir (UNUSED_A, GPIO_OUT);    
	gpio_put(UNUSED_A, 0);
}


//void core1_main()
//{
//    while (1) {
//        printf("hello world from second core\n");
//        sleep_ms(1000);
//    }
//}
//




int main (void)
{
	uart_init(ACTIVE_UART, UART_SPEED);
	 // Set UART flow control CTS/RTS, we don't want these, so turn them off
    uart_set_hw_flow(ACTIVE_UART, false, false);

    // Set our data format
    uart_set_format(ACTIVE_UART, 8, 1, UART_PARITY_NONE);

    // Turn off FIFO's - we want to do this character by character
    uart_set_fifo_enabled(ACTIVE_UART, true);

	stdio_init_all();   

//  multicore_launch_core1(core1_main);
//    printf("hello wow\n");

//	gpio_init(IO_ENABLE);
//	gpio_set_dir(IO_ENABLE, GPIO_OUT);
//	gpio_put(IO_ENABLE, 1);


	// use UART0 at     

    adc_init();

    // Make sure GPIO is high-impedance, no pullups etc
    // Select ADC input 0 (GPIO26)
	adc_gpio_init (SMETER); 
    adc_gpio_init(ADC_KEY);
    adc_gpio_init(PAN_SPEC);
    adc_select_input(SMETER_IN);
	
	
	// use GPIO16 as TX and GPIO17 as RX
	gpio_set_function(UART_TX, GPIO_FUNC_UART);
	gpio_set_function(UART_RX, GPIO_FUNC_UART);
	
	gpio_pull_down(17);
	
	sleep_ms (100);
	printf ("---------------------\n Uuint8_tx starts\n---------------------\n\n");  
	
	
	printf ("%s\n", "Calling displayInit");  
	sleep_ms (100);
	displayInit();
	
	
	sleep_ms (100);
	printf ("\n%s\n", "Calling initSettings");  
	initSettings();
	
	printf ("\n%s\n", "Calling initPorts");  
	initPorts();
	
	printf ("\n%s\n", "Calling initOscillators");
	sleep_ms (100);
	initOscillators();
	printf ("\n%s\n", "Setting frequency");  
	frequency = vfo_a_freq;
	// setfrequency(vfo_a_freq);
	printf ("\n%s\n", "Calling encoder setup");  
	enc_setup();
	
	displayClear(DISPLAY_NAVY);
	displayVFO(CLEAR_VFO);  


	cw_keyer_init (800);  

	
	guiUpdate(CLEAR_VFO);
	
	inTx = false;
	
	draw_s_meter (true);
	
	sleep_ms (100);
//	add_repeating_timer_us (250000, repeating_timer_callback_pan, NULL, &panorama_timer);


	printf ("\nCalling main loop:\n\n");  
	
//	UG GUI gui ; // Global GUI s t r u c t u r e
//	
//	UG Init(&gui , , 320 , 240) ;

	
	loop ();
	
	return 0;
}


bool check_uart (void)
{
	uint32_t ints;
	int16_t ch;
	static uint8_t i = 0, j = 0;
	bool r = false;




	stdio_flush ();
	// check for key press
	ch = getchar_timeout_us (0);

	if (ch >= 0)
	{
		ch &= 0xFF;
		
		inque[i++] = ch;
		
		if (ch == 0xFD) 
		{
//			printf ("%.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X\n", inque[0], inque[1], inque[2], inque[3], inque[4], inque[5], inque[6], inque[7], inque[8], inque[9]);
			i = 0;
			r = true;
		}
	}
	
	return r;
}

/**
 * The main loop controlling the Uuint8_tx radio.
 */

// number of time_tick's (0.5ms)
#define DELTA_T 10
#define LDELTA_T 15*DELTA_T 

#define EVER ;;

void loop(void)
{ 
//	uint16_t i;
	static uint32_t t, t1, t2;

	 t = time_tick + DELTA_T;
	 t2 = time_tick + LDELTA_T;
	inque[0] = inque[1] = inque[2] = inque[3] = inque[4] = 0xAA;
	for (EVER)
	{
		if (time_tick >= t)
		{
			t = time_tick + DELTA_T;
			if (check_uart ())
				dispatch ();
		
//			get_paddle_state ();

			if (mode == CW)
				do_cw (); 
			else
			if (!txCAT)
			{	
				checkPTT();
			}
			if (checkButton())
				do_commands();


			//tune only when not tranmsitting 
			if (!inTx)
			{
				if (ritOn)
					doRIT();
				else 
					doTuning();
	
				// checkTouch();
			}

//			checkCAT();
			// Slow clocking
			if (time_tick >= t1)
			{  
				
				draw_s_meter (false);
				t1 = time_tick + LDELTA_T;
			}
			
			if (time_tick >= t2)
			{
				if (sweep_on)
				{	
					get_pan_data ();
					clearSweep ();
					displaySweep ();
				}
				else // there is to long delay for get_pan_data
				{
					clearSweep ();
					t2 = time_tick + DELTA_T;
				}
			}
		}
//		guiUpdate();	
	}
}


uint32_t millis (void)
{
	uint32_t time;
	time = to_ms_since_boot(get_absolute_time());
	return time;
}
void setmode (uint8_t m)
{
	mode = m;
}

uint8_t getmode (void)
{
	return mode;
}