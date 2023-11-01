#ifndef _NANO_GUI_H_
#define _NANO_GUI_H_

#include "ili9341.h"

//screen
#define D_WIDTH    320
#define D_HEIGHT   240


// Fonts: 
// Arial fonst
#define A_NORMAL		0
#define A_ITALIC		1
#define A_BOLD			2
#define U_NORMAL		3
#define U_BOLD			4
#define LU_NORMAL		5
#define LU_BOLD			6
#define SEGMENT			7
#define BIG_SEGMENT		8
#define S_METER			9


#define STANDARD     	A_NORMAL

#define SPI_PORT spi0
//#define SPI_SPEED 62000*1000
#define TFT_BAUD 60000*1000
#define TOUCH_BAUD 2500*1000
#define SPI_POL  (spi_cpol_t)0
#define SPI_PHA  (spi_cpha_t)0




/* UI functions */
struct Point {
  int x, y;
};
extern struct Point ts_point;

extern const uint8_t arial_normal[];
extern const uint8_t arial_bold[];
extern const uint8_t arial_italic[];
extern const uint8_t ubuntu[];
extern const uint8_t ubuntu_bold[];
extern const uint8_t l_ubuntu[];
extern const uint8_t l_ubuntu_bold[];
extern const uint8_t sixteensegment[];
extern const uint8_t SixteenSegmentBig[];
extern const uint8_t s_meter[];


void displayInit();
void displayClear(uint16_t color);
void displayPixel(uint16_t x, uint16_t y, uint16_t c);
void displayHline(uint16_t  x, uint16_t y, uint16_t len, uint16_t  c);
void displayVline(uint16_t  x, uint16_t y, uint16_t len, uint16_t  c);
void displayRect(uint16_t x,uint16_t y,uint16_t w,uint16_t h,uint16_t c);
void displayFillrect(uint16_t x, uint16_t y,uint16_t  w,uint16_t  h,uint16_t  c);
void displayuint8_t(int16_t x, int16_t y, uint8_t c, uint16_t color, uint16_t bg, uint8_t use_font);
int displayTextExtent(uint8_t *text);
void displayRawText(char *text, uint16_t x1, uint16_t y1, uint16_t color, uint16_t background, uint8_t use_font);
void displayText(uint8_t *text, uint16_t x, uint16_t y, uint16_t color, uint16_t background, uint8_t font); 
void displayChar(int16_t x, int16_t y, uint8_t c, uint16_t color, uint16_t bg, uint8_t use_font);

bool readTouch();

void setupTouch();
void scaleTouch(struct Point *p);




// Color definitions
//#define DISPLAY_NAVY		0x000F  ///<   0,   0, 123
#define DISPLAY_NAVY		0x0008  ///<   0,   0, 123
#define DISPLAY_BLACK		0x0000  ///<   0,   0,   0
#define DISPLAY_DARKGREEN   0x03E0  ///<   0, 125,   0
#define DISPLAY_DARKCYAN    0x03EF  ///<   0, 125, 123
#define DISPLAY_MAROON      0x7800  ///< 123,   0,   0
#define DISPLAY_PURPLE      0x780F  ///< 123,   0, 123
#define DISPLAY_OLIVE       0x7BE0  ///< 123, 125,   0
#define DISPLAY_LIGHTGREY   0xC618  ///< 198, 195, 198
#define DISPLAY_DARKGREY    0x7BEF  ///< 123, 125, 123
#define DISPLAY_BLUE        0x001F  ///<   0,   0, 255
#define DISPLAY_GREEN       0x07E0  ///<   0, 255,   0
#define DISPLAY_CYAN        0x07FF  ///<   0, 255, 255
#define DISPLAY_RED         0xF800  ///< 255,   0,   0
#define DISPLAY_MAGENTA     0xF81F  ///< 255,   0, 255
#define DISPLAY_YELLOW      0xFFE0  ///< 255, 255,   0
#define DISPLAY_WHITE       0xFFFF  ///< 255, 255, 255
#define DISPLAY_ORANGE      0xFD20  ///< 255, 165,   0
#define DISPLAY_GREENYELLOW 0xAFE5  ///< 173, 255,  41
#define DISPLAY_PINK        0xFC18  ///< 255, 130, 198

#define TEXT_LINE_HEIGHT 16
#define TEXT_LINE_INDENT 5

#define BUTTON_PUSH
#define BUTTON_CHECK
#define BUTTON_SPINNER


#endif // _NANO_GUI_H_


