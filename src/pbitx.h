#ifndef PBITX_VERSION
#define PBITX_VERSION


#define HIGH    1
#define LOW     0

#define IO_ENABLE  22

#define DEC  10

#define ACTIVE_UART uart0
#define UART_SPEED  115200

// RP Pico pin defines
#define SPI_RX			0
#define TFT_CS   		1
#define SPI_SCK  		2
#define SPI_TX 			3
#define TFT_RS   		4
#define TOUCH_CS  		5
#define TX_RX 			6      
#define CW_TONE 		7    
#define TX_LPF_B		8  
#define TX_LPF_C		9   
#define CW_KEY 			10    
#define TX_LPF_A		11  
#define UNUSED_A  		12	
#define I2C_SDA			14
#define I2C_SCL			15
#define UART_TX  		16	
#define UART_RX  		17	
#define ENC_A  			18
#define ENC_B  			19
#define FBUTTON  		20
#define PTT  			21
#define SPARE	  		22
#define ADC_KEY			26
#define SMETER			27
#define PAN				28

// Analog ports
#define SMETER_IN  		SMETER - 26
#define ANALOG_KEYER  	ADC_KEY - 26
#define PAN_SPEC		PAN - 26

#define PAN_SZ	255

// addresses for values in e_storage, that is fake EEPROM in flash
#define VFO_MODE_LSB 2
#define VFO_MODE_USB 3
#define LSB_CAL 4
#define USB_CAL 8
#define VFO_A 16
#define VFO_B 20
#define CW_SIDETONE 24
#define CW_SPEED 28
#define SLOPE_X 32 // the screen calibration parameters 
#define SLOPE_Y 36
#define OFFSET_X 40
#define OFFSET_Y 44
#define CW_DELAYTIME 48
#define MASTER_CAL 128
#define VFO_A_MODE  238 // 2: LSB, 3: USB
#define VFO_B_MODE  242
#define CW_KEY_TYPE 254 // handkey, iambic a, iambic b : 0,1,2f


#define	 KEEP_VFO   0
#define  CLEAR_VFO	1
#define CW_SPEED 28
#define TX_SSB 0
#define TX_CW 1
#define IAMBICA 0x00 // 0 for Iambic A, 1 for Iambic B
#define IAMBICB 0x10 // 0 for Iambic A, 1 for Iambic B
#define OPEN_KEY 	0
#define CLOSED_KEY 	1

// the screen calibration parameters : int slope_x=104, slope_y=137, offset_x=28, offset_y=29;
#define SLOPE_X 32
#define SLOPE_Y 36
#define OFFSET_X 40
#define OFFSET_Y 44


// NUmbers corrected to be correct with CI-V
#define LSB		0
#define USB		1
#define CW		3

#define STRAIGHT		0
#define DUMB_BUGG		1
#define IAMBIC_BUGG		2



typedef struct {
  int x, y, w, h;
  char *text;
  char id;
} Button;

typedef struct {
  uint8_t x0, y0, x1, y1;} grid_entry;

typedef struct { uint8_t x, y; char inf;} pad_grid_entry;



  //frequency is the current frequency on the dial
extern uint32_t frequency;
extern uint32_t ritRxFrequency;
extern uint32_t ritTxFrequency;

//extern int32_t calibration;

extern uint8_t active_vfo;

extern uint8_t mode;
extern uint32_t sideTone;
extern uint32_t usbCarrier;
extern uint32_t cwTimeout;
extern int cwDelayTime;
extern bool Iambic_Key;
extern bool cw_monitor;


void setmode (uint8_t m);
uint8_t getmode (void);
void tone (void);
void no_tone (void);
void set_cw_mon_freq (uint32_t frq);

extern bool inTx;
extern bool menuOn;

extern bool split_on;
extern bool txCAT; 

extern bool ritOn;
extern uint32_t vfo_a_freq, vfo_b_freq, sideTone, usbCarrier;
extern uint8_t mode_vfoa, mode_vfob;
extern unsigned long firstIF;
extern uint32_t time_tick;

extern bool keyDown;
extern bool accel_vfo;

extern int cwSpeed; //this is actuall the dot period in milliseconds
extern uint8_t cw_mode;
extern uint16_t pan_data[];
extern bool sweep_on;

void saveVFOs(void);
void ritEnable(uint32_t f);
void ritDisable(void);
uint32_t split_freq (void);
bool in_split (void);

void setupFreq(void);
void setupBFO(void);

uint32_t millis (void);
void displayVFO(uint8_t clr);
void wait4btn_up(void);
bool xpt2046_Init(void);
void startTx(bool soft);
void stopTx(bool soft);
void checkCAT(void);


//displays a nice dialog box with a title and instructions as footnotes
void displayDialog(char *title, char *instructions);
void guiUpdate(bool vfo_redraw);
void setfrequency(unsigned long f);
uint32_t getfrequency(void);
void drawTx(void);
void doSetup2(void);
void redraw_menus(void);
void draw_s_meter (bool redraw);
uint16_t analogRead (uint8_t pin);
void clearSweep (void);
void displaySweep (void);

bool btnDown(void);
void readTouchCalibration(void);
void do_commands(void);
void checkTouch(void);

void enc_setup(void);
int enc_read(void);

void set_calibration (uint32_t cal);
uint32_t get_calibration (void);
void load_calibration (void);


void si5351bx_setfreq(uint8_t clknum, uint32_t fout);
void si5351_set_calibration(int32_t cal);
void initOscillators(void);
void printCarrierFreq(uint32_t freq);

void updateDisplay(bool redraw);

void switchVFO(int vfoSelect);
void swap_vfo (void);
uint32_t get_vfo_freq (uint8_t v);
void eq_vfo_ab (uint8_t v);
void sel_active_vfo (uint8_t v);


void get_paddle_state (void);
void cw_keyer_init (uint32_t frq);
void do_cw (void);
void set_cw_speed (uint16_t spd);


#endif