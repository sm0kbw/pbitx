// headerfile contents
#ifndef DISPATCH
#define DISPATCH

#define QUE_SIZE		20
#define CALL_TAB_SIZE 	0x20

#define CIV_CMD_POS	4
#define CIV_SUB_POS	5
#define CIV_ARG_POS	6


#define CIV_SEPARATE	    0x2D


#define SET_FREQ_DATA		0x00
#define SET_MODE_DATA		0x01
#define GET_BAND_EDGE		0x02
#define GET_DISP_FREQ		0x03
#define GET_OP_MODE			0x04
#define	SET_OP_FREQ		    0x05
#define SET_MOD_MODE		0x06
#define SEL_VFO_MODE		0x07
#define SEL_MEM_MODE		0x08
#define MEM_WRITE			0x09
#define MEM_2_VFO			0x0A
#define MEM_CLEAR			0x0B
#define READ_DUPLEX_FREQ	0x0C
#define SET_DUPLEX_FREQ		0x0D
#define SCAN_MODE			0x0E
#define SPLIT_MODE			0x0F
#define SET_TUNE_STEP		0x10
#define SET_REC_GAIN		0x11
#define NMT_CALL			0x12
#define ANN_INFO			0x13
#define SET_MISC_MODE		0x14
#define READ_REC_DATA		0x15
#define SET_REC_MODE		0x16
#define UNIMPLEMENTED		0x17
#define POWER_ON			0x18
#define READ_RIG_ID		    0x19
#define CTCSS				0x1B
#define TX_ON_OFF			0x1C
#define DTMF				0x1F

#define VFO_MODE			0

typedef  void (*call_ptr)(void);

extern const call_ptr call_table [CALL_TAB_SIZE];
extern uint8_t outque[QUE_SIZE];
extern uint8_t inque[QUE_SIZE];

void set_freq_data (void);
void set_mode_data (void);
void get_band_edge (void);
void get_disp_freq (void);
void get_op_mode (void);
void set_op_freq (void);
void set_mod_mode (void);
void sel_vfo_mode (void);
void sel_mem_mode (void);
void mem_write (void);
void mem_2_vfo (void);
void mem_clear (void);
void get_duplex_freq (void);
void set_duplex_freq (void);
void set_scan_mode (void);
void split_mode (void);
void set_tune_step (void);
void set_rec_gain (void);
void ann_info (void);
void set_misc_mode (void);
void read_rec_data (void);
void set_rec_mode (void);
void power_on (void);
void read_rig_id (void);
void ctcss (void);
void tx_on_off (void);
void dtmf (void);
void antenna_switch (void);

void unimplemented (void);



void dispatch (void);

#endif
// headerfile end

