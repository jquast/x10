#define BUFLEN 4096

struct btype {
		time_t td;
		char objname[16];
		char message[100];
		unsigned short int color;
		unsigned short int attr;
} buffer[BUFLEN];

/* Function prototypes */
void add_buffer (char *, char *, int, int);

void lastlog ();

void x10mesg (char *, ...);

void error (char *, ...);

void mesg (char *, ...);

void debug (char *, ...);

void echo (char *, ...);

void lt_msg (char *, char *, ...);

void buffer_clear ();
