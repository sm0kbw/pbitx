#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <pico/stdlib.h>
#include <hardware/spi.h>
#include "pbitx.h"
#include "e_storage.h"
#include "ili9341.h"
#include "gui_driver.h"

#define MAX_VBUFF 16

struct Point ts_point;

void utftCmd(uint8_t cmd);
void utftData(uint8_t d);
void test_font (void);
const uint8_t *get_font (uint8_t select);
void setrotation(uint8_t *m);




void utftAddress(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
	utftCmd(ILI9341_CASET); // 0x2A Column Address Set
	utftData(x1 >> 8);
	utftData(x1);
	utftData(x2 >> 8);
	utftData(x2);
	utftCmd(ILI9341_PASET); // 0x2B Page Address Set
	utftData(y1 >> 8);
	utftData(y1);
	utftData(y2 >> 8);
	utftData(y2);
	utftCmd(ILI9341_RAMWR); // 0x2C Memory Write               
}



void displayPixel(uint16_t x, uint16_t y, uint16_t c)
{  
	utftCmd(ILI9341_RAMWR); // 0x2C Memory Write
	utftAddress(x,y,x,y);
	utftData(c>>8);
	utftData(c);
}



void quickFill(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
{
	uint32_t  ncount;
	int k;


	uint8_t vbuff[MAX_VBUFF];


	ncount = (uint32_t)(x2 - x1+1) * (y2-y1+1);
	//set the window

	utftCmd(0x02c); //write_memory_start  
	utftAddress(x1,y1,x2,y2);
	gpio_put(TFT_CS, 0);
	gpio_put(TFT_RS, 1);
  
	while(ncount)
	{
		k = 0;
		for (int i = 0; i < MAX_VBUFF/2; i++)
		{
			vbuff[k++] = color >> 8;
			vbuff[k++] = color & 0xff;
		}

		if (ncount > MAX_VBUFF/2)
		{
			spi_write_blocking (SPI_PORT, vbuff, MAX_VBUFF);
			ncount -= MAX_VBUFF/2;
		}  
		else
		{
			spi_write_blocking (SPI_PORT, vbuff, (int)ncount * 2);
			ncount = 0;      
		}
//		checkCAT();
	}
	gpio_put(TFT_CS, 1);
}


void displayClear(uint16_t color)
{  
	quickFill(0,0,319,239, color);
}

void displayHline(uint16_t x, uint16_t y, uint16_t len, uint16_t  c)
{  
	quickFill(x,y,x+len,y,c);
}


void displayVline(uint16_t  x, uint16_t y, uint16_t len, uint16_t  c)
{ 
	quickFill(x,y,x,y+len,c);
}
 

void displayRect(uint16_t x,uint16_t y,uint16_t w,uint16_t h,uint16_t c)
{
	displayHline(x  , y  , w, c);
	displayHline(x  , y+h, w, c);
	displayVline(x  , y  , h, c);
	displayVline(x+w, y  , h, c);
}

void displayFillrect(uint16_t x, uint16_t y,uint16_t  w,uint16_t  h,uint16_t  c)
{
	quickFill(x,y,x+w,y+h, c);
}



void displayInit(void)
{
	spi_init(SPI_PORT, TFT_BAUD);
	spi_set_format (SPI_PORT, 8, SPI_POL, SPI_PHA, SPI_MSB_FIRST);
//	hw_write_masked(&spi_get_hw(spi_default)->cr0, (1 - 1) << SPI_SSPCR0_SCR_LSB, SPI_SSPCR0_SCR_uint8_tS);	
	hw_write_masked(&spi_get_hw(spi_default)->cr0, 0, SPI_SSPCR0_SCR_BITS);	

	gpio_set_function(SPI_SCK, GPIO_FUNC_SPI);
	gpio_set_function(SPI_TX, GPIO_FUNC_SPI);
	gpio_set_function(SPI_RX, GPIO_FUNC_SPI);
	gpio_set_function(TFT_CS,   GPIO_FUNC_SIO); 
	gpio_init(TFT_CS);
	gpio_put(TFT_CS, 1);
	gpio_set_dir(TFT_CS, GPIO_OUT);

	gpio_init(TFT_RS);
	gpio_put(TFT_RS, 1);
	gpio_set_dir(TFT_RS, GPIO_OUT);
	
	gpio_put(TFT_CS, 0);
	utftCmd(0x01);
	sleep_ms (200);

	utftCmd(0xCB);  
	utftData(0x39); 
	utftData(0x2C); 
	utftData(0x00); 
	utftData(0x34); 
	utftData(0x02); 
	
	utftCmd(0xCF);  
	utftData(0x00); 
	utftData(0XC1); 
	utftData(0X30); 
	
	utftCmd(0xE8);  
	utftData(0x85); 
	utftData(0x00); 
	utftData(0x78); 
	
	utftCmd(0xEA);  
	utftData(0x00); 
	utftData(0x00); 
	
	utftCmd(0xED);  
	utftData(0x64); 
	utftData(0x03); 
	utftData(0X12); 
	utftData(0X81); 
	
	utftCmd(0xF7);  
	utftData(0x20); 
	
	utftCmd(0xC0);    //Power control 
	utftData(0x23);   //VRH[5:0] 
	
	utftCmd(0xC1);    //Power control 
	utftData(0x10);   //SAP[2:0];BT[3:0] 
	
	utftCmd(0xC5);    //VCM control 
	utftData(0x3e);   //Contrast
	utftData(0x28); 
	
	utftCmd(0xC7);    //VCM control2 
	utftData(0x86);   //--
	
	sleep_ms(100);	// need to be stable before rotate command
	utftCmd(0x36);    // Memory Access Control 
	utftData(0x28);   // Make this horizontal display   
	
	utftCmd(0x3A);    
	utftData(0x55); 
	
	utftCmd(0xB1);    
	utftData(0x00);  
	utftData(0x18); 
	
	utftCmd(0xB6);    // Display Function Control 
	utftData(0x08); 
	utftData(0x82);
	utftData(0x27);  
	
	utftCmd(0x11);    //Exit Sleep 
	sleep_ms(120); 
		
	utftCmd(0x29);    //Display on 
	utftCmd(0x2c); 
	
	gpio_put(TFT_CS, HIGH);
  
	xpt2046_Init();
  
	readTouchCalibration();  
}


void utftCmd(uint8_t cmd)
{   
	gpio_put(TOUCH_CS, HIGH);
	gpio_put (TFT_CS, LOW);
	gpio_put (TFT_RS, LOW);
	spi_write_blocking (SPI_PORT, &cmd, (size_t)1);
	gpio_put (TFT_RS, HIGH);
	gpio_put (TFT_CS, HIGH);
	sleep_us (5);
}

void utftData(uint8_t d)
{
	gpio_put (TFT_RS, HIGH);
	gpio_put(TFT_CS, LOW);
	spi_write_blocking (SPI_PORT, &d, (size_t)1);
	gpio_put (TFT_CS, HIGH);
	sleep_us (5);
}


uint16_t rgb2num(uint8_t r, uint8_t g, uint8_t b)
{
    return (((r >> 3) & 0x1f) << 11) | (((g >> 2) & 0x3f) << 5) | ((b >> 3) & 0x1f);
}


const uint8_t *get_font (uint8_t select)
{
	const uint8_t *font;
	switch (select)
	{
		case A_BOLD:
			font = arial_bold;
			break;

		case A_ITALIC:
			font = arial_italic;
			break;

		case U_NORMAL:
			font = ubuntu;
			break;

		case U_BOLD:
			font = ubuntu_bold;
			break;

		case LU_NORMAL:
			font = ubuntu;
			break;

		case LU_BOLD:
			font = ubuntu_bold;
			break;

		case SEGMENT:
			font = sixteensegment;
			break;


		case BIG_SEGMENT:
			font = SixteenSegmentBig;
			break;

		case S_METER:
			font = s_meter;
			break;

		case A_NORMAL:
		default:
			font = arial_normal;
			break;
	}

	return font;
}


void displayChar(int16_t x, int16_t y, uint8_t c, uint16_t color, uint16_t bg, uint8_t use_font)
{
 
	uint16_t base;
	uint16_t k;
	uint8_t uint8_ts, uint8_t_cnt;
	uint8_t vbuff[64];
	uint8_t dx, dy;
	int8_t w, h;
	const uint8_t *font;
		
	font = get_font (use_font);

	w = *font;
	h = *(font + 1);
	uint8_t_cnt = 0;

	base = 4 + (c * (uint8_t)((w * h)/8));

	if (use_font != S_METER)
	{
		if (c >= 0x20  &&  c < 0x7F)
			base = 4 + (c - 0x20) * (uint8_t)((w * h)/8);
	}

	
	for(dy=0; dy < h; dy++) 
	{
		k = 0;
		for(dx = 0; dx < w; dx++) 
		{
			if(!(uint8_t_cnt++ & 7))  // if (uint8_ts++ == 8)
			{
				uint8_ts = *(font + base++);
			}
			if(uint8_ts & 0x80) 
			{
				vbuff[k++] = color >> 8;
				vbuff[k++] = color & 0xff;
			}
			else 
			{
				vbuff[k++] = bg >> 8;
				vbuff[k++] = bg & 0xff;
			}
			uint8_ts <<= 1;
		}
		
		utftAddress(x, y, x + w, y + 1);

		gpio_put(TFT_RS, HIGH);
		gpio_put(TFT_CS, LOW);

		spi_write_blocking (SPI_PORT, vbuff, k);
		
		y++;
	}
		
	gpio_put(TFT_CS, HIGH);
//	checkCAT();
}


void displayRawText(char *text, uint16_t x1, uint16_t y1, uint16_t color, uint16_t background, uint8_t use_font)
{
	uint16_t w, h;
	const uint8_t *font;
		
	font = get_font (use_font);

	w = *font;
	h = *(font + 1);

	
	while(*text)
	{

    uint8_t c = *text++;
    
    	if ((c >= 0x20  &&  c < 0x7F)  ||  (use_font == S_METER)) 
		{
  
			if((w > 0) && (h > 0))
			{ // Is there an associated uint8_tmap?
				displayChar(x1, y1+TEXT_LINE_HEIGHT, c, color, background, use_font);
//				checkCAT();
			}
			x1 += w; 
		}
   }
}

// The generic routine to display one line on the LCD 
// void displayText(uint8_t *text, int x, int y, int color, int background, int border, uint8_t font) 
//void displayText(uint8_t *text, uint16_t x, uint16_t y, uint16_t color, uint16_t background, uint16_t border, uint8_t font) 
void displayText(uint8_t *text, uint16_t x, uint16_t y, uint16_t color, uint16_t background, uint8_t font) 
{
	const uint8_t *f;
	uint16_t w, h;
	uint8_t c; //, len;
//	uint16_t x0, y0, x1, y1, dx, dy;

	f = get_font (font);

	w = *f;
	h = *(f + 1);
//	len = strlen (text) * w;
	
// 	dx = w / 8;
//	dy = h / 8;
//	x0 = (x > dx) ? x - dx : x;
//	y0 = (y > dy) ? y - dy : y;
	
//	x1 = ((x + len + dx) < D_WIDTH) ? x + len + dx : x;
//	y1 = ((y + dy) < D_HEIGHT) ? h + dy : 0;
	
//	x1 += w / 4;
//	y1 += h / 4;

	while(*text != '\0')
	{
		c = *text++;
    
        if((w > 0) && (h > 0)) 
		{ 
            displayChar(x, y, c, color, background, font);
 //           checkCAT();
        }
        if (c == '.'  ||  c == ':')
		{
			if (font == LU_NORMAL)
				x += 12;
			else
				x += 7;
		}
		else
			x += w;    
	}
}



// Borrowed from Adafruits ILI9341 lib

void setrotation(uint8_t *m)
{ 
	uint8_t rot;
	
	rot = *m & 0x3; // can't be higher than 3

	switch (rot) 
	{
		case 0:
			*m = (MADCTL_MX | MADCTL_BGR);
			break;

		case 1:
			*m = (MADCTL_MV | MADCTL_BGR);
			break;

		case 2:
			*m = (MADCTL_MY | MADCTL_BGR);
			break;

		case 3:
			*m = (MADCTL_MX | MADCTL_MY | MADCTL_MV | MADCTL_BGR);
		break;
	}
	utftCmd(ILI9341_MADCTL);  
	utftData(rot);   
}