#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pbitx.h"
#include "e_storage.h"
#include "morse.h"
#include "hardware/adc.h"
#include "gui_driver.h"

/**
 * The user interface of the uuint8_tx consists of the encoder, the push-button on top of it
 * and the 16x2 LCD display.
 * The upper line of the display is constantly used to display frequency and status
 * of the radio. Occasionally, it is used to provide a two-line information that is 
 * quickly cleared up.
 */



#define BUTTON_SELECTED 1


#define MAX_BUTTONS 18
#define MAX_KEYS 17


const Button btn_set[MAX_BUTTONS] =
{ 
  {0, 64, 152, 18,  "VFOA", 'A'},
  {158, 64, 40, 18,  "**", '*'},
  {204, 64, 54, 18,  "RIT", 'R'},
  {264, 64, 54, 18, "<|>", 'S'},
  {0, 94, 52, 18, "USB", 'U'},
  {56, 94, 52, 18, "LSB", 'L'},
  {111, 94, 38, 18, "CW", 'M'},
  {154, 94, 52, 18, "WPM", 'W'},
  {211, 94, 52, 18, "TON", 'T'},
  {267, 94, 52, 18, "A/B", 'F'},
  {0, 124, 38, 18, "80", '8'},
  {40, 124, 38, 18, "40", '4'},
  {80, 124, 38, 18, "30", '3'},
  {120, 124, 38, 18, "20", '2'},
  {160, 124, 38, 18, "17", '7'},
  {200, 124, 38, 18, "15", '5'},
  {240, 124, 38, 18, "13", '6'},
  {280, 124, 38, 18, "10", '1'},
};


// Display Grid is divided in 8x8 touch points rectangles
const  grid_entry the_grid[MAX_BUTTONS] = 
{
	{0, 40, 47, 66}, 	// VFO A
	{0, 40, 17, 48}, 	// **
	{18, 40, 37, 48},	// RIT
	{38, 40, 56, 48}, 	// SPL
	{57, 40, 78, 48}, 	// USB
	{79, 40, 96, 48}, 	// LSB
	{0, 72, 17, 80}, 	// CW
	{18, 72, 37, 80},	// WPM
	{38, 72, 56, 80}, 	// TON
	{57, 72, 78, 80}, 	// FRQ
	{0, 56, 12, 72}, 	// 80m
	{13, 56, 27, 72}, 	// 40m
	{28, 56, 40, 72}, 	// 30m
	{41, 56, 54, 72}, 	// 20m
	{55, 56, 67, 72}, 	// 17m
	{68, 56, 80, 72}, 	// 15m
	{81, 56, 96, 72}, 	// 10m
	{94, 56, 109, 72}, 	// 10m
};



const Button keypad[MAX_KEYS] = {   
  {0, 80, 60, 36,  "1", '1'},
  {64, 80, 60, 36, "2", '2'},
  {128, 80, 60, 36, "3", '3'},
  {192, 80, 60, 36,  "",'-'},
  {256, 80, 60, 36,  "OK", 'K'},

  {0, 120, 60, 36,  "4", '4'},
  {64, 120, 60, 36,  "5", '5'},
  {128, 120, 60, 36,  "6", '6'},
  {192, 120, 60, 36,  "0", '0'},
  {256, 120, 60, 36,  "<-", 'B'},

  {0, 160, 60, 36,  "7", '7'},
  {64, 160, 60, 36, "8", '8'},
  {128, 160, 60, 36, "9", '9'},
  {192, 160, 60, 36,  "",'-'},
  {256, 160, 60, 36,  "Can", 'C'},
};

// Scaled to touch grids
const pad_grid_entry pad_grid[MAX_BUTTONS] = {   
  { 0, 37, '1'},
  {21, 37, '2'},
  {42, 37, '3'},
  {63, 37, '-'},
  {84, 37, 'K'},
  { 0, 56, '4'},
  {21, 56, '5'},
  {42, 56, '6'},
  {63, 56, '0'},
  {84, 56, 'E'},	
  { 0, 75, '7'},
  {21, 75, '8'},
  {42, 75, '9'},
  {63, 75, '-'},
  {84, 75, 'D'},
  {104, 93, '-'}   // extra End entry 
};

uint16_t pan_data[PAN_SZ];
void wait4btn_up(void);
bool getButton(char *text, Button *b);
void formatFreq(uint32_t f, char *buff); 





bool getButton(char *text, Button *b)
{
	bool r = false;
	
	for (int i = 0; i < MAX_BUTTONS; i++)
	{
		memcpy (b, btn_set + i, sizeof(Button));
		if (!strcmp(text, b->text))
		{
			r = true;
			break;
		}
	}
	
	return r;
}



/*
 * This formats the frequency given in f 
 */
void formatFreq(uint32_t f, char *buff) 
{
	char lbuff[30];
	
	memset(buff, 0, 10);
	memset(lbuff, 0, sizeof(lbuff));
	sprintf (lbuff, "%lu", f);
	
	//one mhz digit if less than 10 M, two digits if more
	if (f < 10000000l)
	{
		buff[0] = ' ';
		strncat(buff, lbuff, 4);    
		strcat(buff, ".");
		strncat(buff, &lbuff[4], 2);
	}
	else
	{
		strncat(buff, lbuff, 5);
		strcat(buff, ".");
		strncat(buff, &lbuff[5], 2);    
	}
}

void drawCommandbar(char *text)
{
  displayRawText(text, 68, 24, DISPLAY_WHITE, DISPLAY_NAVY, A_NORMAL);
}


uint16_t getValueByKnob(uint16_t minimum, uint16_t maximum, uint16_t step_size,  uint16_t initial, uint8_t *prefix, uint8_t *postfix)
{
	char buff[30], cbuff[30];
	int8_t knob;
	uint16_t knob_value;
	
	while (btnDown())
		sleep_ms (50);
	
	sleep_ms (50);
	knob_value = initial;
	
	strcpy(buff, (char *)prefix);
	itoa(knob_value, cbuff, 10);
	strcat(buff, cbuff);
	strcat(buff, (char *)postfix);
	drawCommandbar(buff);
	while(!btnDown() && gpio_get(PTT))
	{

		knob = enc_read();
		printf ("Knob = %d\n", knob);

		if (knob != 0)
		{
			if (knob_value > minimum && knob < 0)
				knob_value -= step_size;
			if (knob_value < maximum && knob > 0)
				knob_value += step_size;
		
			strcpy(buff, (char *)prefix);
			itoa(knob_value, cbuff, 10);
			strcat(buff, cbuff);
			strcat(buff, (char *)postfix);
			drawCommandbar(buff);
		}
	}
	return knob_value;
}

void printCarrierFreq(uint32_t freq)
{
	char buff[30], cbuff[30];
	memset(cbuff, 0, sizeof(cbuff));
	memset(buff, 0, sizeof(buff));
	
	sprintf (buff, "%lu", freq);
	
	strncat(cbuff, buff, 2);
	strcat(cbuff, ".");
	strncat(cbuff, &buff[2], 3);
	strcat(cbuff, ".");
	strncat(cbuff, &buff[5], 1);
	displayText((uint8_t *)cbuff, 110, 100, DISPLAY_CYAN, DISPLAY_NAVY, A_NORMAL);
}

void displayDialog(char *title, char *instructions)
{
	displayClear(DISPLAY_BLUE);
	displayRect(10,10,300,220, DISPLAY_WHITE);
	displayRect(12,12,296,216, DISPLAY_WHITE);
	displayRawText(title, 20, 20, DISPLAY_WHITE, DISPLAY_NAVY, A_BOLD);
	displayRawText(instructions, 20, 190, DISPLAY_WHITE, DISPLAY_NAVY, A_NORMAL);
}

void clearSweep (void)
{
	displayRect(32,150,256, 60, DISPLAY_WHITE);
	displayFillrect(33,151,254, 58, DISPLAY_BLACK);
}
void displaySweep (void)
{
	uint8_t val;
	uint16_t i;

	displayRect(159,150,1, 60, DISPLAY_RED);

	for (i = 0; i < PAN_SZ; i++)
	{
		val = 0xD2 - ((pan_data[i] >> 4) & 0x3C);
//		displayRect(i + 33, 151, 254, 58, DISPLAY_BLACK);
		displayRect(i + 32, val, 1, (210 - val), DISPLAY_GREENYELLOW);
	
	}

}


void displayVFO(uint8_t clr)
{
	uint16_t x, y;
//	uint16_t displayColor, displayBorder;
	Button b;
	char dbuff[30], buff[30], vbuff[10];
	
	static char vfoDisplay[12];
	uint8_t digit;
	bool dirty;

	if (clr == CLEAR_VFO)
	{
		memset(vfoDisplay, 0, 12);
	}
		getButton("VFOA", &b);
		
	
	if (active_vfo == VFO_A)
	{				
		strcpy(buff, "B:");
		strcpy(vbuff, "VFOA");
		formatFreq(vfo_b_freq, buff+2);
		formatFreq(frequency, dbuff);
	}
	else
	{
		strcpy(buff, "A:");
		strcpy(vbuff, "VFOB");
		formatFreq(vfo_a_freq, buff+2);
		formatFreq(frequency, dbuff);
	}
		
	
	if (clr)	
	{
		displayFillrect(b.x, b.y - 1, b.w, b.h + 2, DISPLAY_BLACK);
	}
		
	displayRect(b.x, b.y - 1, b.w , b.h + 2, DISPLAY_WHITE);


	x = b.x + 2;
	y = b.y + 2;

	displayRect(57, 1, 181, 33, DISPLAY_BLUE);
	displayText((uint8_t *)dbuff, 58, 2, DISPLAY_CYAN, DISPLAY_DARKGREEN, LU_NORMAL); 	
	displayText((uint8_t *)vbuff, 248, 4, DISPLAY_NAVY, DISPLAY_CYAN, A_BOLD);
	dirty = strcmp (vfoDisplay, buff);
//	printf ("display: %s %s\n", dbuff, vbuff);
	
	for (int i = 0; i <= 12; i++)
	{
		if (i < (int)strlen(buff)  &&  dirty)
		{
			digit = buff[i];

			displayChar(x, y, digit, DISPLAY_WHITE, DISPLAY_BLACK, A_BOLD);
			if (digit == ':' || digit == '.')
				x += 7;
			else
				x += 16;
			
		}
	}	
  
	strcpy(vfoDisplay, buff);
}




void btnDraw(Button *b)
{
	if (!strcmp(b->text, "VFOA"))
	{
		displayVFO(KEEP_VFO);
	}
	else
	{	
		displayFillrect(b->x, b->y, b->w ,b->h, DISPLAY_BLACK);

		if (
			(!strcmp(b->text, "RIT") && ritOn) || 
			(!strcmp(b->text, "USB") && (mode == USB)) || 
			(!strcmp(b->text, "LSB") && (mode == LSB)) || 
			(!strcmp(b->text, "**") &&  accel_vfo) || 
			(!strcmp(b->text, "A/B") && split_on))
				displayText((uint8_t *)(b->text), b->x + 4, b->y, DISPLAY_BLACK, DISPLAY_ORANGE, A_BOLD);   
		else
		if (!strcmp(b->text, "CW") && (mode == CW))
			displayText((uint8_t *)(b->text), b->x + 4, b->y, DISPLAY_BLACK, DISPLAY_ORANGE, A_BOLD );   
		else
			displayText((uint8_t *)(b->text), b->x + 4, b->y, DISPLAY_GREEN, DISPLAY_BLACK, A_BOLD);
	}
}

void displayRIT(void)
{
	char cbuff[30];

	if (ritOn)
	{
		strcpy(cbuff, "Tx: ");
		formatFreq(ritTxFrequency, &cbuff[2]);
		displayRawText(cbuff, 68, 24, DISPLAY_ORANGE, DISPLAY_NAVY, A_BOLD);
	}
	else 
	{
      displayRawText("          ", 68, 24, DISPLAY_WHITE, DISPLAY_NAVY, A_NORMAL);
	}
}

void fastTune(void)
{
	if (accel_vfo)
	{
		accel_vfo = false;
	}
	else
	{
		accel_vfo = true;
    }
}


// to complicated, should really be split to several functions! BA - KBW
void enterFreq(void)
{
	//force the display to refresh everything
	//display all the buttons
	uint32_t f;
	char cbuff[30];
    Button b;
	pad_grid_entry 	p;
	uint16_t i;
	uint16_t c_pos = 0;
	uint8_t id;


	printf ("enter keypad\n");
	displayClear(DISPLAY_BLUE);
	
	displayText((uint8_t *)("          "), 20, 42, DISPLAY_WHITE, DISPLAY_NAVY, A_NORMAL);
	displayText((uint8_t *)("kHz"), 120, 42, DISPLAY_WHITE, DISPLAY_NAVY, A_NORMAL);


	for (i = 0; i < MAX_KEYS; i++)
	{
		memcpy(&b, keypad + i, sizeof(Button));
		btnDraw(&b);
	}	

	memset(cbuff, 0, sizeof(cbuff));
//	f = frequency / 1000l;

	while(true)
	{
		if (readTouch())
		{
			scaleTouch(&ts_point);
//			printf ("ts.x %d ts.y %d X %d Y %d W %d H %d    %s\n",ts_point.x, ts_point.y,  b.x, b.y, b.w, b.h, b.text);

			for (i = 0; i < MAX_KEYS; i++)
			{
				p = pad_grid[i]; 
				id = p.inf;

//				printf ("ts.x %d ts.y %d p.x %d p.y %d   %c\n",ts_point.x, ts_point.y,  p.x, p.y, p.inf);
				if (ts_point.x > p.x  &&  ts_point.x < (p.x + 18)   &&  ts_point.y > p.y &&  ts_point.y < (p.y + 18))
				{
//					printf ("ts.x %d ts.y %d p.x %d p.y %d \t %c \t %s\n",ts_point.x, ts_point.y,  p.x, p.y, p.inf, cbuff);
				
					if ((f = atol(cbuff)) > 30000  &&  id >= '0'  &&  id <= '9')
						id = 'D';
					else
					switch (id)
					{
						case 'E':  // As in enter
						case 'K': // As in OK
							f = atol(cbuff);
							printf ("f is %ld\n", f);
							if(f > 100  &&  f < 30000)
							{
								f = f * 1000l;
								setfrequency(f);
								if (active_vfo == VFO_A)
									vfo_a_freq = f;
								else
									vfo_b_freq = f;
								saveVFOs();
							}
							goto THE_END;
							break;
							
						case 'D': // As in Delete
							cbuff[c_pos] = 0;
						
							if (c_pos > 0)
								c_pos--;      
							break;
							
						case '0': case '1': case '2': case '3': case '4':
						case '5': case '6': case '7': case '8': case '9': 
							cbuff[c_pos++] =p.inf;
							cbuff[c_pos] = 0;
							break;
					}
						displayText((uint8_t *)(cbuff), 20, 42, DISPLAY_WHITE, DISPLAY_NAVY, A_NORMAL);
						displayText((uint8_t *)("kHz"), 120, 42, DISPLAY_WHITE, DISPLAY_NAVY, A_NORMAL);
						
//				printf ("ts.x %d ts.y %d p.x %d p.y %d \t %c \t %s\n",ts_point.x, ts_point.y,  p.x, p.y, p.inf, cbuff);

				}
			}
//		printf ("ts.x %d ts.y %d p.x %d p.y %d np.x %d np.y %d    %c\n",ts_point.x, ts_point.y,  p.x, p.y, np.x, np.y, p.inf);
//		printf (" %d \t%d\n\n", atol(cbuff), c_pos);
		sleep_ms (500);
		}	 
//	printf ("%s\n", cbuff);	
	} 
	
THE_END:
//	sleep_ms (1000);
//	printf ("ts.x %d ts.y %d p.x %d p.y %d   %s\n",ts_point.x, ts_point.y,  p.x, p.y, cbuff);
//	printf (" %d \t%d\n\n", atol(cbuff), c_pos);
	displayClear(DISPLAY_NAVY);		
	guiUpdate(CLEAR_VFO);
}

void drawCWStatus(void)
{
	char buff[30], cbuff[30];
	
	displayFillrect(0, 201, 320, 39, DISPLAY_NAVY);
	strcpy(buff, "CW:");
	int wpm = 1200/cwSpeed;    
	itoa(wpm,cbuff, 10);
	strcat(buff, cbuff);
	strcat(buff, "WPM ");
	itoa(sideTone, cbuff, 10);
	strcat(buff, cbuff);
	strcat(buff, "hz");
	strcat(buff, " V:1.0");
	
	displayRawText(buff, 0, 200, DISPLAY_CYAN, DISPLAY_NAVY, A_NORMAL);  
}


void drawTx(void)
{
		printf ("mode %d\n", mode);

	if (inTx)
		displayText((uint8_t *)("TX"), 280, 24, DISPLAY_BLACK, DISPLAY_ORANGE, A_BOLD);  
	else
		displayFillrect(280, 24, 37, 28, DISPLAY_NAVY);
}

void drawStatusbar(void)
{
  drawCWStatus();
}


uint8_t get_s_value(uint8_t max)
{
	uint16_t s_val;
	uint16_t n;
	
	// Scale constant
	n = 0x1000 / max;
	
	adc_select_input(SMETER_IN);

	s_val = analogRead (SMETER_IN);
	
	s_val /= n;

    return s_val;	
}


void guiUpdate(bool redraw)
{
	displayVFO(redraw);
	
	for (int i = 0; i < MAX_BUTTONS; i++)
	{
		Button b;
		memcpy(&b, btn_set + i, sizeof(Button));
		btnDraw(&b);
		displayRect(b.x, b.y, b.w, b.h, DISPLAY_BLUE);
	}
	
	
	displayRIT();
	drawStatusbar();
}



void draw_s_meter (bool redraw)
{
	uint8_t ch1, ch2;
	uint8_t x, y;
	uint8_t s = get_s_value(11);
	uint16_t ch2_colour;

	if (redraw)
	{
		displayFillrect(2, 4, 50, 28, DISPLAY_BLUE);
		displayRect(2, 4, 50, 28, DISPLAY_GREEN);
	}
	
	if (s < 9)
	{
		ch1 = s;
		ch2 = 0;
		ch2_colour = DISPLAY_YELLOW;

	}
	else
	{
		ch1 = 8;
		ch2 = (s > 9) ? 10 : 9;
		ch2_colour = DISPLAY_RED;
		
	}	
 	x = 4;
	y = 10;
	
	displayChar(x, y, ch1, DISPLAY_YELLOW, DISPLAY_BLUE, S_METER);
	displayChar(x + 24, y, ch2, ch2_colour, DISPLAY_BLUE, S_METER);
		
}


// this builds up the top line of the display with frequency and mode
void updateDisplay(bool redraw)
{
   displayVFO(redraw);    
}


void ritToggle(Button *b)
{
	if (!ritOn)
	{
		ritEnable(frequency);
	}
	else
		ritDisable();

	btnDraw(b);
	displayRIT();
}



void splitToggle(Button *b)
{
	Button b2;

    split_on = !split_on;
	btnDraw(b);
	ritDisable();
  	getButton("RIT", &b2);
	btnDraw(&b2);
  	displayRIT();
	displayVFO(KEEP_VFO);
}

void vfoReset(void)
{
	Button b;

	if (active_vfo == VFO_A)
		vfo_b_freq = vfo_a_freq;
	else
		vfo_a_freq = vfo_b_freq;

	if (split_on)
	{
		getButton("<|>", &b);
		splitToggle(&b);
	}

	if (ritOn)
	{
		getButton("RIT", &b);
		ritToggle(&b);
	}

	displayVFO(CLEAR_VFO);

	saveVFOs();
}

void cwToggle(Button *b)	
{
	if (mode == CW)
	{
		setmode (USB);
	}
	else
	{
		setmode (CW);
		cw_keyer_init (800);
	}
	gpio_put(CW_KEY, OPEN_KEY);
	setfrequency(frequency);
	btnDraw(b);

	Button e;
	getButton("USB", &e);
	btnDraw(&e);

	getButton("LSB", &e);
	btnDraw(&e);
	
	printf ("mode %d\n", mode);

}

void sidebandToggle(Button *b)
{
	if (!strcmp(b->text, "USB"))
		setmode (USB);
	else
	if (!strcmp(b->text, "LSB"))
		setmode (LSB);
		
	Button e;
	getButton("USB", &e);
	btnDraw(&e);

	getButton("LSB", &e);
	btnDraw(&e);
	saveVFOs();
}


void redraw_menus(void)
{
    Button b;

    ritDisable();
    getButton("RIT", &b);
    btnDraw(&b);
    displayRIT();
    getButton("LSB", &b);
    btnDraw(&b);
    getButton("USB", &b);
    btnDraw(&b);  
    getButton("CW", &b);
    btnDraw(&b);  
    getButton("<|>", &b);
    btnDraw(&b);  
}


void switchBand(long bandfreq)
{
	uint32_t offset;
	
	if (3500000l <= frequency && frequency <= 4000000l)
		offset = frequency - 3500000l;
	else
	if (24800000l <= frequency && frequency <= 25000000l)
		offset = frequency - 24800000l;
	else 
		offset = frequency % 1000000l; 
	
	
	setfrequency(bandfreq + offset);
//	setTXFilters(bandfreq);
	updateDisplay(CLEAR_VFO); 
	saveVFOs();
}

int setCwSpeed(void)
{
//    int knob = 0;
    int wpm = 60;

	printf ("wpm %d cwSpeed %d\n", wpm, cwSpeed);
//    wpm = 1200/cwSpeed;
     
    wpm = getValueByKnob(20, 200, 1,  wpm, (uint8_t *)("CW: "), (uint8_t *)(" WPM"));
  
    cwSpeed = wpm;

    e_put(CW_SPEED, cwSpeed);
	set_cw_speed (wpm);
    sleep_ms(25);
	printf ("wpm %d cwSpeed %d\n", wpm, cwSpeed);
    drawStatusbar();      

	return wpm;
}

void setCwTone(void)
{
	char buff[30], cbuff[30];
	int16_t knob;
//	uint32_t prev_sideTone;
     
//	tone(CW_TONE, sideTone);

	//disable all clock 1 and clock 2 
	while (gpio_get(PTT) && !btnDown())
	{
		knob = enc_read();
		
		if (knob != 0)
		{
			if (knob > 0 && sideTone < 2000)
				sideTone += 10;
			else
			if (knob < 0 && sideTone > 100 )
				sideTone -= 10;
        
//			tone(sideTone);
			itoa(sideTone, cbuff, 10);
			strcpy(buff, "CW Tone: ");
			strcat(buff, cbuff);
			strcat(buff, " Hz");
			drawCommandbar(buff);

//			checkCAT();
			sleep_ms(10);
		}
	}
//	noTone(CW_TONE);
	e_put(CW_SIDETONE, sideTone);
	set_cw_mon_freq (sideTone);


	drawStatusbar();
}

void exec_command(Button *b)
{
	printf ("exec_cmd %s\n", b->text);
	
	if (!strcmp(b->text, "**"))
		fastTune();
	else
	if (!strcmp(b->text, "RIT"))
		ritToggle(b);
	else
	if (!strcmp(b->text, "LSB"))
		sidebandToggle(b);
	else
	if (!strcmp(b->text, "USB"))
		sidebandToggle(b);
	else
	if (!strcmp(b->text, "CW"))
		cwToggle(b);
	else
	if (!strcmp(b->text, "FRQ"))
		sweep_on = !sweep_on;
	else
	if (!strcmp(b->text, "VFOA"))
	{
		if (active_vfo == VFO_A)
			switchVFO(VFO_B);
		else
			switchVFO(VFO_A);
	}
	else
	if (!strcmp(b->text, "A=B"))
		vfoReset();
	else
	if (!strcmp(b->text, "80"))
		switchBand(3500000l);
	else
	if (!strcmp(b->text, "40"))
		switchBand(7000000l);
	else
	if (!strcmp(b->text, "30"))
		switchBand(10000000l);
	else
	if (!strcmp(b->text, "20"))
		switchBand(14000000l);
	else
	if (!strcmp(b->text, "17"))
		switchBand(18000000l);
	else
	if (!strcmp(b->text, "15"))
		switchBand(21000000l);
	else
	if (!strcmp(b->text, "13"))
		switchBand(24800000l);
	else
	if (!strcmp(b->text, "10"))
		switchBand(28000000l);  
	else
	if (!strcmp(b->text, "<|>"))
		sweep_on = !sweep_on;
	else
	if (!strcmp(b->text, "WPM"))
		setCwSpeed();
	else
	if (!strcmp(b->text, "TON"))
		setCwTone();

	guiUpdate (KEEP_VFO);
}



void checkTouch(void)
{
//	bool touch;
//	int total;
	grid_entry *pos;

  
	if (readTouch ())
	{
		scaleTouch(&ts_point);

  
		for (int i = 0; i < MAX_BUTTONS; i++)
		{
			pos = (grid_entry *)&(the_grid[i]);

			if (ts_point.x > pos->x0  && ts_point.x < pos->x1  &&  ts_point.y > pos->y0  &&  ts_point.y < pos->y1)
			{
//				printf ("posX %d posY %d\n", ts_point.x, ts_point.y);
				exec_command((Button *)&btn_set[i]);
			}
		}
	}
}

//returns true if the button is pressed
bool btnDown(void)
{
//	if (gpio_get(FBUTTON))
//		return false;
//	else
//		return true;
    return (gpio_get(FBUTTON) == 0);
}


void drawFocus(uint16_t ibtn, uint16_t color)
{
	Button b;

	memcpy(&b, btn_set + ibtn, sizeof(Button));
	displayRect(b.x, b.y, b.w, b.h, color);
}


void wait4btn_up(void)
{
  //wait for the button to be raised up
	while(btnDown())
		sleep_ms (50);
	sleep_ms(10);  //debounce
}

void btn_selected (uint8_t sel)
{
	Button b;
	memcpy(&b, btn_set + sel, sizeof(Button));
	
	exec_command(&b);
	
	//unfocus the buttons
//	drawFocus(select, DISPLAY_BLUE);
	if (active_vfo == VFO_A)
		drawFocus(0, DISPLAY_WHITE);
	else
		drawFocus(1, DISPLAY_WHITE);
	
	//wait for the button to be up and debounce
	wait4btn_up ();     
}


void do_commands(void)
{
	uint8_t select, prevButton, btnState;

	int8_t i;

	select=0;
	prevButton = 0;
	wait4btn_up();
 
//	guiUpdate();
	
	while (true)
	{
		//check if the knob's button was pressed
		btnState = btnDown();
		if (btnState)
		{
			btn_selected (select/10);
			break;
		}
		
		i = enc_read();
		
		if (i != 0)
		{
			
			if (i > 0)
			{	
		
				if (select + i < MAX_BUTTONS * 10)
					select += i;
			}
			
			if (i < 0 && select + i >= 0)
				select += i;      //caught ya, i is already -ve here, so you add it
			
			if (prevButton != select / 10)
			{
			
				//we are on a new button
				drawFocus(prevButton, DISPLAY_BLUE);
				drawFocus(select / 10, DISPLAY_WHITE);
				prevButton = select / 10;
			}
		}
	}

//	guiUpdate();
	wait4btn_up ();
// displayVFO (KEEP_VFO);	
//	checkCAT();
}

uint16_t analogRead (uint8_t p)
{
	uint16_t val;
	adc_select_input(p);
	val = adc_read ();
	
//	printf ("val 0x%.2X\n", (val >> 4));

	return val;
}
