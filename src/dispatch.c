

// CI-V command interpreter:
// calling: dispatch (char cmd, char *data);
//
// Where cmd has to be a valid CI-V command and
// data should point to a valid argument string.
// The string has to be the clean data segment
// of a CI-V command ending with 0xFD.
//
// dispatch will return a data segment without a ending 0xFD.
// ett avslutande 0xFD
//
// Just frequency/vfo handling, modes and TX/RX switch is implemented.
//
// The interpreter is based on a function pointer table.
//
// Original written by SM0KBW  2005-03-16 for a duabander 2m/70cm FM rig
// Adapted for KBW´s Ubitx V6 port to Raspberry Pi Pico 2023-10-29


#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "pbitx.h"
#include "dispatch.h"



#define AF1_DELTA			70
#define AF2_DELTA			1	
#define DURATION			10000

#define CIV_CMD_POS			4
#define CIV_ARG_POS			5
#define CIV_SUB_POS			6

#define CIV_RIG_ADDR		0xA1

#define OK_NUM				0xFB
#define NG_NUM				0xFA
#define END_NUM				0xFD

#define AF_TIME_CONST		223718


typedef struct _ch_data { uint32_t tx_frq; uint8_t stat;} ch_data;
typedef struct _mem_data { uint32_t rx_frq; uint32_t tx_frq; uint8_t data1; uint8_t data2;} mem_data;


// Using a table with function pointers to interpret CIV commands
const call_ptr call_table [CALL_TAB_SIZE] =
{

	set_freq_data, 		// SET_FREQ_DATA		0x00
	set_mode_data,		// SET_MODE_DATA		0x01
	get_band_edge,		// GET_BAND_EDGE		0x02
	get_disp_freq, 		// GET_DISP_FREQ		0x03
	get_op_mode,		// GET_OP_MODE			0x04
	set_op_freq,		// SET_OP_FREQ			0x05
	set_mod_mode,		// SET_MOD_MODE			0x06
	sel_vfo_mode,		// SEL_VFO_MODE			0x07
	sel_mem_mode,		// SEL_MEM_MODE			0x08
	mem_write,			// MEM_WRITE			0x09
	mem_2_vfo,			// MEM_2_VFO			0x0A
	mem_clear,			// MEM_CLEAR			0x0B
	get_duplex_freq,	// READ_DUPLEX_FREQ		0x0C
	set_duplex_freq,	// SET_DUPLEX_FREQ		0x0D
	set_scan_mode,		// SET_SCAN_MODE		0x0E
	split_mode,			// SPLIT_MODE			0x0F
	set_tune_step,		// SET_TUNE_STEP		0x10
	set_rec_gain,		// SET_REC_GAIN			0x11
	antenna_switch,		//	Antenna_switch		0x12
	ann_info,			// ANN_INFO				0x13
	set_misc_mode,		// SET_MISC_MODE		0x14
	read_rec_data,		// READ_REC_DATA		0x15
	set_rec_mode,		// SET_REC_MODE			0x16
	unimplemented,		//						0x17
	power_on,			//						0x18
	read_rig_id,		// READ_RIG_ID			0x19
	unimplemented,		//						0x1A	
	unimplemented,		//						0x1B	
	tx_on_off	,		// Transmit On/Off		0x1C
	unimplemented,		//						0x1D	
	unimplemented,		//						0x1E	
	unimplemented,		//						0x1F	

};





uint8_t out_index;
uint8_t outque[QUE_SIZE];
uint8_t inque[QUE_SIZE];
uint8_t out_len;
uint8_t rx_base;

uint8_t vfo_table[2];
uint8_t active_vfo;

uint8_t buff [10];
uint8_t mem_mode;
uint8_t watch;

void set_ok_str (uint8_t pos);
void set_ng_str (uint8_t pos);

// CIV_CMD_POS	4
// CIV_SUB_POS	5
// CIV_ARG_POS	6

void dispatch (void)
{
	uint8_t i;
	out_index = 0;

//	printf ("0x%.2X 0x%.2X 0x%.2X 0x%.2X 0x%.2X 0x%.2X 0x%.2X 0x%.2X 0x%.2X 0x%.2X\n", inque[0], inque[1], inque[2], inque[3], inque[4], inque[5], inque[6], inque[7], 
// inque[8], inque[9]);

//	printf ("dispatch cmd num %.2X\n", inque[CIV_CMD_POS]);

	// prepare return string
	
	for (i = 0; i < 5; i++)
	{
		outque[i] = inque[i];
	}
//	swap radio and controller address;
	outque[2] = inque[3];
	outque[3] = inque[2];


	// command is uint8_t #4 in the CI-V command sequence
	if (inque[CIV_CMD_POS] < CALL_TAB_SIZE)
		(call_table [inque[CIV_CMD_POS]])();
	else
		set_ng_str (CIV_CMD_POS);
		
//printf ("0x%.2X 0x%.2X 0x%.2X 0x%.2X 0x%.2X 0x%.2X 0x%.2X 0x%.2X 0x%.2X 0x%.2X 0x%.2X 0x%.2X 0x%.2X 0x%.2X 0x%.2X 0x%.2X 0x%.2X 0x%.2X 0x%.2X 0x%.2X\n", 
//			outque[0], outque[1], outque[2], outque[3], outque[4], outque[5], outque[6], outque[7], outque[8], outque[9],
//			outque[10], outque[11], outque[12], outque[13], outque[14], outque[15], outque[16], outque[17], outque[18], outque[19]);

}



// SET_FREQ_DATA		0x00
// Same as 05 without return CIV string

void set_freq_data (void)
{
	set_op_freq ();  // 05
	out_len = 0;  // No return str

}



// SET_MODE_DATA		0x01
// 0xFE 0xFE 0x00 0xA1 0x01 0x03 0x00 0xFD - CW mode 
// And nothing should be returned
void set_mode_data (void)
{
//	printf ("arg %d\n", inque[CIV_ARG_POS]);
	
	switch (inque[CIV_ARG_POS])
	{
		case 0:
			setmode (LSB);
			break;
			
		case 1:
			setmode (USB);
			break;
			
		case 3:
			setmode (CW);
			break;
	}
	redraw_menus ();
	out_len = 0;  // No return str
}



// GET_BAND_EDGE		0x02
void get_band_edge (void)
{
	uint8_t i;
	
// hard coded 3.5MHz to 30 MHz	
	i = CIV_ARG_POS;
	outque[i++] = 0x00;
	outque[i++] = 0x00;
	outque[i++] = 0x35;
	outque[i++] = 0x00;
	outque[i++] = 0x00;

	outque[i++] = CIV_SEPARATE;
	
	outque[i++] = 0x00;
	outque[i++] = 0x00;
	outque[i++] = 0x00;
	outque[i++] = 0x30;
	outque[i++] = 0x00;

//	set_ok_str (CIV_CMD_POS);
		
	outque[i] = END_NUM;
	out_len =  CIV_SUB_POS + 17;

}



// 0xFE 0xFE 0xA1 0xE0 0x03 0xFD
// GET_DISP_FREQ		0x03

void get_disp_freq (void)
{
	uint16_t i;
	uint32_t freq;

	freq = getfrequency ();
//	printf ("freq %d\n", freq);
	
	int2bcd (freq, buff);

	for (i = 0; i < 5; i++)
	{
		outque[CIV_ARG_POS + i] = buff[i];
	}
	
	outque[CIV_ARG_POS + i++] = END_NUM;
	out_len =  i + CIV_ARG_POS;

//	for (i = 0; i < 10; i++)
//		printf ("0x%.2X : ", buff[i]);
//	printf ("\n");
}




// GET_OP_MODE			0x04

// Just narrow FM

void get_op_mode (void)
{
	uint8_t m;
	
	m = getmode ();
	
	outque[CIV_ARG_POS] = m;	
	outque[CIV_ARG_POS + 1] = 0x02;	// Narrow bandwidth
	set_ok_str (CIV_ARG_POS + 2);
}


// 0xFE 0xFE 0x00 0xA1 0x00 0x00 0x00 0x00 0x00 0x35 0x00 0xFD
// 0xFE 0xFE 0x00 0xA1 0x00 0x00 0x00 0x00 0x00 0x71 0x00 0xFD
// SET_OP_FREQ			0x05
// Copy from ch_table to vfo_table
void set_op_freq (void)
{
	uint32_t freq;
 
//	printf ("op_freq\n");
	freq = bcd2int (CIV_ARG_POS + 1, inque);
//	printf ("freq %d\n", freq);
	setfrequency ((unsigned long)freq);

	set_ok_str (CIV_CMD_POS);
}


// 0xFE 0xFE 0xA1 0xE0 0x06 0xFD
// SET_MOD_MODE			0x06

// This rig can't change BW 
void set_mod_mode (void)
{
	set_ng_str (CIV_CMD_POS);
	
}

// 0xFE 0xFE 0xA1 0xE0 0x07 0xB0 0xFD
// SEL_VFO_MODE			0x07

void sel_vfo_mode (void)
{
	uint8_t tmp;

	if (inque[CIV_ARG_POS] == 0x00)	  // Enable vfo mode and select vfo A
	{
		if (active_vfo != VFO_A)
		{
			switchVFO(VFO_A);
		}
		set_ok_str (CIV_CMD_POS);
	}
	else
	if (inque[CIV_ARG_POS] == 0x01)    // Enable vfo mode and select vfo B
	{
		if (active_vfo != VFO_B)
		{
			switchVFO(VFO_B);
		}
		set_ok_str (CIV_CMD_POS);
	}
	else
	if (inque[CIV_ARG_POS] == 0xA0)    // Equal vfo A and vfo B
	{
		eq_vfo_ab (VFO_A);
		set_ok_str (CIV_CMD_POS);
	}
	else
	if (inque[CIV_ARG_POS] == 0xB0)    // Swap vfo A & B
	{
		if (active_vfo == VFO_A)
		{
			swap_vfo ();
		}
		else
		{
			sel_active_vfo (VFO_A);
		}

		set_ok_str (CIV_CMD_POS);

	}
	else
		set_ng_str (CIV_CMD_POS);
}



// SEL_MEM_MODE			0x08

// Select memory channel already pointed to or given in sub cmd
// For now only 0 - 99 is supported
void sel_mem_mode (void)
{
	set_ng_str (CIV_CMD_POS);
}



// MEM_WRITE			0x09
// Copy from vfo_table to scan_table
void mem_write (void)
{
	set_ng_str (CIV_CMD_POS);
}



// MEM_2_VFO			0x0A

void mem_2_vfo (void)
{
	set_ng_str (CIV_CMD_POS);
}


// MEM_CLEAR			0x0B

void mem_clear (void)
{
	set_ng_str (CIV_CMD_POS);
}




// READ_DUPLEX_FREQ		0x0C
// Just taking the offset off current vfo
void get_duplex_freq (void)
{
	set_ng_str (CIV_CMD_POS);
}



// SET_DUPLEX_FREQ		0x0D
// Sets current vfo offset
 void set_duplex_freq (void)
{ 	
	set_ng_str (CIV_CMD_POS);
}	



// SET_SCAN_MODE		0x0E

// From reference manual:
// 0x00 Scan stops
// 0x01 Programmed scan or memory scan starts
// 0x02 Programmed scan starts
// 0x03 delta F scan starts
// 0x04 Auto memory write scan starts
// 0x12 Fine programmed scan starts
// 0x13 Fine delta F scan starts
// 0x22 memoory scan starts
// 0x23 Selected number memory scan starts
// 0x24 Selected mode memory scan starts
// 0x42 Priority scan or basic windo scan starts

// from IC 910
// 0x00 stop scan
// 0x01 Start scan
// 0xD0 Set scan resume OFF
// 0xD3 Set scan resume ON

// This function mimics the IC910 scan table
void set_scan_mode (void)
{
	set_ng_str (CIV_CMD_POS);
}



// SPLIT_MODE			0x0F

 void split_mode (void)
{
//	uint32_t f;
//	uint8_t i;
//
//	f = split_freq ();
//
//	int2bcd (f, buff);
//	for (i = 0; i < 5; i++)
//	outque[i + CIV_SUB_POS] = buff[i];
//
//	outque[i++ + CIV_SUB_POS] = END_NUM;
//	out_len =  i + CIV_SUB_POS;
//	set_ok_str (CIV_CMD_POS);
//
//
//	set_ok_str (CIV_CMD_POS);
//
//	if (*(inque + CIV_SUB_POS) == 0x10)				// 0x10 Cancel duplex operation
//		dup_mode = DUP_MODE_OFF;
//	else
//	if (*(inque + CIV_SUB_POS) == 0x11)				// 0x11 Select - duplex operation
//		dup_mode = DUP_MODE_NEG;
//	else
//	if (*(inque + CIV_SUB_POS) == 0x12)				// 0x12 Select + duplex operation
//		dup_mode = DUP_MODE_POS;
//	else
		set_ng_str (CIV_CMD_POS);
}



// SET_TUNE_STEP		0x10
// We just uses 25 KHz for now
 void set_tune_step (void)
{
	set_ng_str (CIV_CMD_POS);
}



// SET_REC_GAIN			0x11
// Full attenuation or full gain - preamp on/off

 void set_rec_gain (void)
{
	set_ng_str (CIV_CMD_POS);
}


void antenna_switch (void)
{ 
	set_ng_str (CIV_CMD_POS);
}



// ANN_INFO				0x13
// speach control, use CW for audio read out?

void ann_info (void)
{
	set_ng_str (CIV_CMD_POS);
}



// SET_MISC_MODE		0x14
// select AF, RF and squelsh level
void set_misc_mode (void)
{
	set_ng_str (CIV_CMD_POS);
}



// READ_REC_DATA		0x15

 void read_rec_data (void)
{
	set_ng_str (CIV_CMD_POS);
}
//
//
//
//// SET_REC_MODE			0x16
//
void set_rec_mode (void)
{
	set_ng_str (CIV_CMD_POS);
}


//  Unimplemented		0x17

 void unimplemented (void)
{
	set_ng_str (CIV_SUB_POS);
}


//  Power on/off		0x18
void power_on (void)
{
	set_ng_str (CIV_CMD_POS);
}



// READ_RIG_ID			0x19
void read_rig_id (void)
{
	*(outque + CIV_CMD_POS + 1) = CIV_RIG_ADDR;
	set_ok_str (CIV_SUB_POS + 2);
}

//  linked to Unimplemented		0x1A



// 0xFE 0xFE 0xA1 0xE0 0x1C 0x00 0xFD  | 0xFE 0xFE 0xA1 0xE0 0x1C 0x01 0xFD
// Transmit On/Off 0x1C
// 0==Tx Off and  1==Tx on


void tx_on_off (void)
{
	set_ok_str (CIV_CMD_POS);

	if (inque[CIV_ARG_POS] == 0x00)
		stopTx (true);
	else
	if (inque[CIV_ARG_POS] == 0x01)
		startTx (true);
	else
		set_ng_str (CIV_CMD_POS);
}



void set_ok_str (uint8_t pos)
{
	*(outque + pos++) = OK_NUM;	
	*(outque + pos++) = END_NUM;	
	out_len = pos;
}


void set_ng_str (uint8_t pos)
{
	*(outque + pos++) = NG_NUM;	
	*(outque + pos++) = END_NUM;	
	out_len = pos;
}
