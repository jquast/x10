#define DEVFILE "/dev/cua00"

/* Function prototypes */
unsigned char cm11a_chksum (void *, int);

int cm11a_open ();

int cm11a_close ();

void x10_build_time (char *, int, int);

char *cm11a_day (int);

int cm11a_status ();

int cm11a_setclock (int);
