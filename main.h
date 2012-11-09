#include <stdio.h>
#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/ttycom.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>

#define CMDLEN 70

int debugmode;
int housecode;
struct termios ott;

/* Function prototypes */
void process (char *);

int inkey(void);

void shutdown(void);
