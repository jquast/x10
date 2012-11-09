/* Temporary structure for editing fields */
struct ptype_str {
		char housecode[3];
		char devicecode[4];
		char delay[4];
		char ontime[8];
};

/* structure for pump data */
struct ptype {
		int state;
		char housecode;
		char devicecode;
		int delay; /* in minutes */
		int ontime;	/* in seconds */
		time_t last;
		int retry; /* retry on failure count */
} pump;

/* Function prototypes */
void pump_on();

void pump_off();

int pump_check();

void pump_view();

void pump_clear();

struct ptype_str pump_2str (struct ptype);

struct ptype str2_pump (struct ptype_str);
