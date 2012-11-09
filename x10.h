/* Bitflags that can be attached to a time download. */
#define TIME_MONITOR_CLEAR 1
#define TIME_TIMER_PURGE 2
#define TIME_BATTERY_TIMER_CLEAR 4

/* Bitflags for the Header:Code byte. */
#define HEADER_DEFAULT 0x04
#define HEADER_EXTENDED 0x01
#define HEADER_FUNCTION 0x02

/* housecodes */
#define HOUSECODE_A 0x06
#define HOUSECODE_B 0x0e
#define HOUSECODE_C 0x02
#define HOUSECODE_D 0x0a
#define HOUSECODE_E 0x01
#define HOUSECODE_F 0x09
#define HOUSECODE_G 0x05
#define HOUSECODE_H 0x0d
#define HOUSECODE_I 0x07
#define HOUSECODE_J 0x0f
#define HOUSECODE_K 0x03
#define HOUSECODE_L 0x0b
#define HOUSECODE_M 0x00
#define HOUSECODE_N 0x08
#define HOUSECODE_O 0x04
#define HOUSECODE_P 0x0c
/* devicecodes */
#define DEVICECODE_1 0x06
#define DEVICECODE_2 0x0e
#define DEVICECODE_3 0x02
#define DEVICECODE_4 0x0a
#define DEVICECODE_5 0x01
#define DEVICECODE_6 0x09
#define DEVICECODE_7 0x05
#define DEVICECODE_8 0x0d
#define DEVICECODE_9 0x07
#define DEVICECODE_10 0x0f
#define DEVICECODE_11 0x03
#define DEVICECODE_12 0x0b
#define DEVICECODE_13 0x00
#define DEVICECODE_14 0x08
#define DEVICECODE_15 0x04
#define DEVICECODE_16 0x0c
/* commands */
#define COMMAND_ALL_UNITS_OFF 0x00
#define COMMAND_ALL_LIGHTS_ON 0x01
#define COMMAND_ON 0x02
#define COMMAND_OFF 0x03
#define COMMAND_DIM 0x04
#define COMMAND_BRIGHT 0x05
#define COMMAND_ALL_LIGHTS_OFF 0x06
#define COMMAND_EXTENDED_CODE 0x07
#define COMMAND_HAIL_REQUEST 0x08
#define COMMAND_HAIL_ACKNOWLEDGE 0x09
#define COMMAND_PRESET_DIM1 0x0a
#define COMMAND_PRESET_DIM2 0x0b
#define COMMAND_EXTENDED_DATA_TRANSFER 0x0c
#define COMMAND_STATUS_ON 0x0d
#define COMMAND_STATUS_OFF 0x0e
#define COMMAND_STATUS_REQUEST 0x0f

/* cm11a device file descriptor */
int X10_FILENO;

/* tables for housecode letter->cm11a housecode translation */
char housecode_letter[16];
unsigned char housecode_table[16];

/* tables for devicecode->cm11a devicecode translation */
int devicecode_number[16];
unsigned char devicecode_table[16];

/* table of strings for interpreting cm11a command codes */
char *command_name[16];

/* poll events */
typedef struct event Event;

struct event {
		unsigned char command;
		unsigned char housecode;
		int devices;
		unsigned char device[16];
		time_t time;
		unsigned char extended1;
		unsigned char extended2;
};

/* cm11a commands */
struct Client_Command {
		int request;
		unsigned char command;
		unsigned char housecode;
		int value;
		int devices;
		unsigned char device[16];
};

/* Function prototypes */
int read_x10 ();

int read_x10_poll ();

int transmit_command (struct Client_Command *);

int x10_read (void *, int);

int x10_wait_read ();

int x10_wait_write ();

int x10_write (void *, int);

int x10_write_message (void *, int, unsigned char);
