#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <pico/stdlib.h>
#include "pbitx.h"
#include "e_storage.h"
//#include "morse.h"
#include "gui_driver.h"

/** Menus
 *  The Radio menus are accessed by tapping on the function button. 
 *  - The main loop() constantly looks for a button press and calls doMenu() when it detects
 *  a function button press. 
 *  - As the encoder is rotated, at every 10th pulse, the next or the previous menu
 *  item is displayed. Each menu item is controlled by it's own function.
 *  - Eache menu function may be called to display itself
 *  - Each of these menu routines is called with a button parameter. 
 *  - The btn flag denotes if the menu itme was clicked on or not.
 *  - If the menu item is clicked on, then it is selected,
 *  - If the menu item is NOT clicked on, then the menu's prompt is to be displayed
 */

 //this is used by the si5351 routines in the uuint8_tx_5351 file
extern uint32_t si5351bx_vcoa;


void setupExit(void)
{
	menuOn = false;
	displayClear(DISPLAY_NAVY);
	displayVFO(CLEAR_VFO);  
	guiUpdate(KEEP_VFO);
}




void setupFreq(void)
{
	int8_t knob;
	int32_t calibration; //, prev_calibration;
	
	uint8_t buff[30];
	
	displayDialog("Set Frequency", "Push TUNE to Save"); 

	//round off the the nearest khz
	frequency = (frequency/1000l)* 1000lu;
	setfrequency(frequency);
	
	displayRawText("You should have a", 20, 50, DISPLAY_CYAN, DISPLAY_BLUE, A_NORMAL);
	displayRawText("signal exactly at ", 20, 75, DISPLAY_CYAN, DISPLAY_BLUE, A_NORMAL);
	sprintf ((char *)buff,"%ld kHz", (frequency/1000l));
	displayRawText((char *)buff, 20, 100, DISPLAY_CYAN, DISPLAY_BLUE, A_NORMAL);
	printf("You should have a signal exactly at %ld kHz\n", (frequency/1000l)); 
	displayRawText("Rotate to zerobeat", 20, 140, DISPLAY_CYAN, DISPLAY_BLUE, A_NORMAL);

	while (btnDown())
		sleep_ms(100);
	sleep_ms (100);
	
//	prev_calibration = get_calibration ();
	calibration = 0;
	

	while (!btnDown())
	{
		knob = enc_read();
		if (knob != 0)
			calibration += knob * 875;
//	    else
//		if (knob < 0)
//			calibration -=  knob * 875;
	
		si5351bx_setfreq(0, usbCarrier);  //set back the cardrier oscillator anyway, cw tx switches it off  
		si5351_set_calibration(calibration);
		setfrequency(frequency);
		
		sprintf ((char *)buff, "%ld", calibration); 
		displayText(buff, 100, 140, DISPLAY_CYAN, DISPLAY_NAVY, A_NORMAL);
	}

	set_calibration (calibration);
	printf ("calibration set to %ld\n", calibration);
	initOscillators();
	si5351_set_calibration(calibration);
	setfrequency(frequency);    

	//debounce and delay
	wait4btn_up ();
}

void setupBFO(void)
{
	int knob;
	
	displayDialog("Set BFO", "Press TUNE to Save"); 
	
	usbCarrier = 11053000l;
	si5351bx_setfreq(0, usbCarrier);
	printCarrierFreq(usbCarrier);
	
	while (!btnDown())
	{
		knob = enc_read();
	
		if (knob != 0)
			usbCarrier -= 50 * knob;
		else
			continue; //don't update the frequency or the display
		
		si5351bx_setfreq(0, usbCarrier);
		setfrequency(frequency);
		printCarrierFreq(usbCarrier);
	
		sleep_ms (100);
	}
	
	e_put(USB_CAL, usbCarrier);  
	si5351bx_setfreq(0, usbCarrier);          
	setfrequency(frequency);
	updateDisplay(CLEAR_VFO);
	menuOn = true; 
}

void setupCwDelay(void)
{
	uint8_t buff[30];
	int8_t knob;


	displayDialog("Set CW T/R Delay", "Press tune to Save"); 
	
	sleep_ms (25);
	
	sprintf ((char *)buff, " Delay  is %.3dms\n", (int)cwDelayTime);
	displayText(buff, 20, 100, DISPLAY_CYAN, DISPLAY_BLACK, A_NORMAL);
	
	while (!btnDown())
	{
		knob = enc_read();
	
		if (knob < 0 && cwDelayTime > 10)
			cwDelayTime -= 10;
		else
		if (knob > 0 && cwDelayTime < 100)
			cwDelayTime += 10;
		else
			continue; //don't update the frequency or the display
	
		sprintf ((char *)buff, " Delay  is %.3dms\n", (int)cwDelayTime);
		displayText(buff, 20, 100, DISPLAY_CYAN, DISPLAY_BLACK, A_NORMAL);
	}
	
	e_put(CW_DELAYTIME, cwDelayTime);
	sleep_ms (25);
}

void setupKeyer(void)
{
	int tmp_key;
	int knob = 0;
	int prev_knob;
	
	displayDialog("Set CW Keyer", "Press button to Save"); 
	
	if (cw_mode == STRAIGHT)
		displayText((uint8_t *)("< Straight Key >"), 20, 100, DISPLAY_CYAN, DISPLAY_BLACK, A_NORMAL);
	else
	if (cw_mode == IAMBIC_BUGG)
	displayText((uint8_t *)("<  Iambic Bugg >"), 20, 100, DISPLAY_CYAN, DISPLAY_BLACK, A_NORMAL);
	else
	if (cw_mode == DUMB_BUGG)
	displayText((uint8_t *)("<  Dumb Bugg   >"), 20, 100, DISPLAY_CYAN, DISPLAY_BLACK, A_NORMAL);
	
	tmp_key = 1; // bugg
	
	while (!btnDown())
	{	prev_knob = knob;
		knob = enc_read();
		
		if (knob == 0)
		{
			sleep_ms (25);
			continue;
		}
		
		if (knob < 0  &&  prev_knob < 0  &&  tmp_key > 0)
			tmp_key--;
		
		if (knob > 0  &&  prev_knob > 0)
			tmp_key++;
		
		if (tmp_key > 2)
			tmp_key = 2;
		
		prev_knob = knob;

		if (tmp_key == 0)
			displayText((uint8_t *)("<  Iambic Bugg >"), 20, 100, DISPLAY_CYAN, DISPLAY_BLACK, A_NORMAL);
		else
		if (tmp_key == 1)
			displayText((uint8_t *)("<  Dumb Bugg   >"), 20, 100, DISPLAY_CYAN, DISPLAY_BLACK, A_NORMAL);
		else
		if (tmp_key == 2)
			displayText((uint8_t *)("< Straight Key >"), 20, 100, DISPLAY_CYAN, DISPLAY_BLACK, A_NORMAL);
	}

	sleep_ms (25);
//	if (tmp_key == )
//		cw_mode = BUGG;
//	else
////	if (tmp_key == 1)
//	{
//		cw_mode = STRAIGHT;
//	}
	
	cw_mode = tmp_key;
  
	e_put(CW_KEY_TYPE, tmp_key);
}

void drawSetupMenu(void)
{
	displayClear(DISPLAY_BLUE);
	displayText((uint8_t *)("Setup"), 16, 16, DISPLAY_WHITE, DISPLAY_NAVY, A_NORMAL); 
	displayRect(10,10,300,220, DISPLAY_WHITE);
	displayRawText("Set Freq...", 30, 38, DISPLAY_WHITE, DISPLAY_NAVY, A_NORMAL);       
	displayRawText("Set BFO....", 30, 68, DISPLAY_WHITE, DISPLAY_NAVY, A_NORMAL);       
	displayRawText("CW Delay...", 30, 98, DISPLAY_WHITE, DISPLAY_NAVY, A_NORMAL);       
	displayRawText("CW Keyer...", 30, 128, DISPLAY_WHITE, DISPLAY_NAVY, A_NORMAL);       
	displayRawText("  Exit  ", 30, 188, DISPLAY_WHITE, DISPLAY_NAVY, A_NORMAL);       
}


void movePuck(int i)
{
	static int prevPuck = -1;
	
	if (prevPuck >= 0)
		displayRect(15, 49 + (prevPuck * 30), 290, 25, DISPLAY_NAVY);
	displayRect(15, 49 + (i * 30), 290, 25, DISPLAY_WHITE);

	prevPuck = i;
}

void doSetup2(void)
{
	int select=0, i;
	
	drawSetupMenu();
	movePuck(select);
	
	//wait for the button to be raised up
	wait4btn_up ();
	
	menuOn = true;
	
	while (menuOn)
	{
		i = enc_read();
	
		if (i > 0)
		{
			if (select + i < 60)
			select += i;
			movePuck(select/10);
		}
		
		if (i < 0 && select - i >= 0)
		{
			select += i;      //caught ya, i is already -ve here, so you add it
			movePuck(select/10);
		}
	
		if (!btnDown())
		{
			sleep_ms (25);
			continue;
		}

		//wait for the touch to lift off and debounce
		wait4btn_up ();
    
		sleep_ms (50);
    
		if (select < 10)
			setupFreq();
		else
		if (select < 20)
			setupBFO(); 
		else
		if (select < 30 )
			setupCwDelay(); 
		else
		if (select < 40)
			setupKeyer();
		else
			break; //exit setup was chosen
    drawSetupMenu();
	}	

	wait4btn_up();
	setupExit();
}
