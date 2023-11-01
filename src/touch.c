#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "hardware/spi.h"
#include "pico/stdlib.h"

#include "pbitx.h"
#include "gui_driver.h"
#include "e_storage.h"

#define Z_THRESHOLD_INT 75
#define MSEC_THRESHOLD  3
#define READ_X_NOADC  	0xD0  // get X values & ADC off
#define READ_X  		0xD1  // get X values
#define READ_Y  		0x91  // get Y values
#define READ_Z1 		0xB1 // get Z1 value
#define READ_Z2 		0xC1 // get Z2 value
#define MSB_uint8_t_MASK    0x7F // uint8_t mask for MSB byte
#define Z_THRESHOLD     32
#define HIT_LIMIT		4

/* touch functions */

/**
Command byte mode uint8_ts used in this project
Lower Command Nibble (4 uint8_ts) for this project
Note:
     /DFR = 0 is differential mode
         Ratiometric conversion - has lowest noise for ADC readings
     PD1 = 0 turns off reference, required for this mode (/DFR=0)
     PD0 = 1 enable ADC
     PD0 = 0 power down device
     MODE = 0 12 uint8_ts
     Summary for lower nibble, we only use:
         MODE SER//DFR PD1 PD0
            0        0   0   0	  12uint8_t, /DFR, REF off, ADC off
            0        0   0   1     12uint8_t, /DFR, REF off, ADC on
Upper Command Nibble (4 uint8_ts) for this project
Note:
     S = 1 Command byte
     S = 0 read data used to clock out data after first command byte
     Summary for upper nibble, we only use
         S  A2 A1 A0     ADC+IN    MEASUREMENT    DRIVERS
         1   0  0  0      TEMP0
         1   0  0  1         XP          Y-POS      YN YP
         1   0  1  0       VBAT
         1   0  1  1         XP         Z1-POS      XN YP
         1   1  0  0         YN         Z2-POS      XN YP
         1   1  0  1         YP          X-POS      XN XP
         1   1  1  0      AUXIN
         1   1  1  1       TEMP
    We only need 4 commands for reading position or touch information
        0x91 Read Y position
        0xb1 Read Z1
        0xc1 Read Z2
        0xd1 Read X position
*/



//filled from a test run of calibration routine
static uint16_t slope_x=104, slope_y=137, offset_x=28, offset_y=29;
static int16_t xraw=0, yraw=0, zraw=0;

bool xpt2046_Init(void)
{
	printf ("Init touch\n");

	gpio_init(TOUCH_CS);
	gpio_set_dir (TOUCH_CS, GPIO_OUT);
	gpio_put(TOUCH_CS, HIGH);
	gpio_put(TFT_CS, HIGH);

//  pinMode(CS_PIN, OUTPUT);
//  digitalWrite(CS_PIN, HIGH);
	return true;
}

void readTouchCalibration(void)
{
	printf ("readTouchCalibration\n");

	
	slope_x = e_get (SLOPE_X); 
	slope_y = e_get (SLOPE_Y); 
	offset_x = e_get (OFFSET_X); 
	offset_y = e_get (OFFSET_Y);   
	
	// for debugging
//	printf("readTouchCalib...\n slope_x %d slope_y %d\n offset_x %d offset_y %d\n", slope_x, slope_y, offset_x, offset_y);
}

void writeTouchCalibration(void)
{
	e_put (SLOPE_X, slope_x);
	e_put(SLOPE_Y, slope_y);
	e_put(OFFSET_X, offset_x);
	e_put(OFFSET_Y, offset_y);    
}



// static int16_t touch_besttwoavg(int16_t x, int16_t y, int16_t z)
// {
// 	int16_t da, db, dc;
// 	int16_t reta = 0;
// 	if ( x > y ) da = x - y; else da = y - x;
// 	if ( x > z ) db = x - z; else db = z - x;
// 	if ( z > y ) dc = z - y; else dc = y - z;
// 	
// 	if ( da <= db && da <= dc )
// 		reta = (x + y) >> 1;
// 	else 
// 	if ( db <= da && db <= dc )
// 		reta = (x + z) >> 1;
// 	else 
// 		reta = (y + z) >> 1;   //    else if ( dc <= da && dc <= db ) reta = (x + y) >> 1;
// 	
// 	return (reta);
// }


static void touch_update(void)
{
	int z;
	uint16_t tx_str[10] = {0xB1, 0xC1, 0x91, 0x91, 0xD1, 0x91, 0xD1, 0x91, 0xD0, 0};

	uint16_t rx_str[10];
  
 
  
	spi_set_baudrate (SPI_PORT, TOUCH_BAUD); 

	gpio_put(TFT_CS, HIGH);
	gpio_put(TOUCH_CS, LOW);
//	sleep_ms (40);

	spi_write16_read16_blocking (SPI_PORT, tx_str, rx_str, (size_t)10);
//	spi_write_read_blocking (SPI_PORT, tx_str, rx_str, (size_t)10);

	z = (rx_str[1]) - (rx_str[2]) + 256;
	
 	rx_str[8] >>= 3;
	rx_str[9] >>= 3;
	
	gpio_put(TOUCH_CS, HIGH);

	spi_set_baudrate (SPI_PORT, TFT_BAUD);
		
	zraw = z;
	xraw = (rx_str[4] + rx_str[6]) / 2;
	yraw = (rx_str[5] + rx_str[7]) / 2 ;
	
//	if (zraw > 384)
//		printf ("xraw %d yraw %d zraw %d\n", xraw, yraw, zraw);
}



bool readTouch(void)
{
	struct Point old_point;
	static uint8_t x_equal, y_equal;
	bool val = false;

	
	old_point = ts_point;
	
	touch_update();
	ts_point.x = xraw;
	ts_point.y = yraw;
	
// Strange, had to change to much harder filtering when going to RP Pico from Arduino /BA - KBW
	if (old_point.x == ts_point.x)
		x_equal++;
	else
		x_equal -= (x_equal > 0) ? 1 : 0;

	if (old_point.y == ts_point.y)
		y_equal++;
	else
		y_equal -= (y_equal > 0) ? 1 : 0;

	if (zraw > Z_THRESHOLD) 
	{


		if (x_equal > HIT_LIMIT  &&  y_equal > HIT_LIMIT)
		{
			val = true;
		}

//		if (val)
//		printf ("ts_x %d ts_y %d zraw %d x_eq %d y_eq %d\n", ts_point.x, ts_point.y, zraw, x_equal, y_equal);
	}
	else
		ts_point.x = ts_point.y = 0;

	return val;
}


void scaleTouch(struct Point *p)
{
//	long x, y;

	p->x = 255 - p->x - 15;
	p->y = 255 - p->y - 15;
}

// not used BA
void setupTouch(){
  int x1, y1, x2, y2, x3, y3, x4, y4;
  
  displayClear(DISPLAY_BLUE);
  displayText((uint8_t *)"Click on the cross", 20,100, DISPLAY_WHITE, DISPLAY_BLACK, A_NORMAL);

  // TOP-LEFT
  displayHline(10,20,20,DISPLAY_WHITE);
  displayVline(20,10,20, DISPLAY_WHITE);

  while(!readTouch())
    sleep_ms(50);
  while(readTouch())
    sleep_ms(50);
   x1 = ts_point.x;
   y1 = ts_point.y; 

  //rubout the previous one
  displayHline(10,20,20,DISPLAY_BLACK);
  displayVline(20,10,20, DISPLAY_BLACK);

  sleep_ms(1000);
   
  //TOP RIGHT
  displayHline(290,20,20,DISPLAY_WHITE);
  displayVline(300,10,20, DISPLAY_WHITE);

  while(!readTouch())
    sleep_ms(100); 
  while(readTouch())
    sleep_ms(100);
   x2 = ts_point.x;
   y2 = ts_point.y; 

  displayHline(290,20,20,DISPLAY_BLACK);
  displayVline(300,10,20, DISPLAY_BLACK);

  sleep_ms(1000);

  //BOTTOM LEFT
  displayHline(10,220,20,DISPLAY_WHITE);
  displayVline(20,210,20, DISPLAY_WHITE);
  
  while(!readTouch())
  {	
	sleep_ms(100);
  }
   x3 = ts_point.x;
   y3 = ts_point.y; 
     
  while(readTouch())
  {	
	sleep_ms(100);
  }
  displayHline(10,220,20,DISPLAY_BLACK);
  displayVline(20,210,20, DISPLAY_BLACK);

  sleep_ms(1000);

  //BOTTOM RIGHT
  displayHline(290,220,20,DISPLAY_WHITE);
  displayVline(300,210,20, DISPLAY_WHITE);

  while(!readTouch())
  {	
	sleep_ms(100);
  }
   x4 = ts_point.x;
   y4 = ts_point.y; 
     
  
  displayHline(290,220,20,DISPLAY_BLACK);
  displayVline(300,210,20, DISPLAY_BLACK);

  // we average two readings and divide them by half and store them as scaled integers 10 times their actual, fractional value
  //the x points are located at 20 and 300 on x axis, hence, the delta x is 280, we take 28 instead, to preserve fractional value,
  //there are two readings (x1,x2) and (x3, x4). Hence, we have to divide by 28 * 2 = 56 
  slope_x = ((x4 - x3) + (x2 - x1))/56; 
  //the y points are located at 20 and 220 on the y axis, hence, the delta is 200. we take it as 20 instead, to preserve the fraction value 
  //there are two readings (y1, y2) and (y3, y4). Hence we have to divide by 20 * 2 = 40
  slope_y = ((y3 - y1) + (y4 - y2))/40;
  
  //x1, y1 is at 20 pixels
  offset_x = x1 + -((20 * slope_x)/10);
  offset_y = y1 + -((20 * slope_y)/10);
 
  writeTouchCalibration();
  displayClear(DISPLAY_BLUE);
}
