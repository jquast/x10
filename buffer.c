/*
   $Id: buffer.c,v 1.1 2005/08/20 17:34:21 dingo Exp $
   */
#include <main.h>
#include <window.h>
#include <buffer.h>
#include <ansi.h>

/* add new object entry to buffer */
void add_buffer(char *object, char *str, int color, int attr) {
	time_t td;
	int n;
	/* shift buffer */
	for(n=BUFLEN-1;n>0;n--)
		if ( buffer[n-1].message[0] != 0) {
			strncpy(buffer[n].message, buffer[n-1].message,
					sizeof(buffer[n].message));
			strncpy(buffer[n].objname, buffer[n-1].objname,
					sizeof(buffer[n].objname));
			buffer[n].color = buffer[n-1].color;
			buffer[n].attr = buffer[n-1].attr;
			buffer[n].td = buffer[n-1].td;
		}
	/* add new entry */
	time(&td);
	snprintf(buffer[0].message, sizeof(buffer[0].message), "%s", str);
	snprintf(buffer[0].objname, sizeof(buffer[0].objname), "%s", object);
	buffer[0].td = td;
	buffer[0].color = color;
	buffer[0].attr = attr;

	/* refresh buffer view */
	draw_buf();
}

/* display logfile in fullscreen pager-like view */
void lastlog() {
	char strtime[24];
	int n;
	int y=1;
	int ch;
	bcolor (BLACK); fcolor (GREY, NORMAL);
	cls ();
	for(n = BUFLEN; n >= 0; n--) {
		if (buffer[n].td!=NULL) {
			pos(1, y);
			strftime(strtime, sizeof strtime, "%D %H:%M:%S",
					 localtime(&buffer[n].td));
			fcolor (CYAN, NORMAL); 
			printf("%s ", strtime);
			fcolor (buffer[n].color, buffer[n].attr); 
			printf("%s ", buffer[n].objname);
			fcolor (GREY, NORMAL); 
			printf("%s\n", buffer[n].message);
			y+=1;
			if (y == ROWS-1) {
				pos(1, y); puts("more"); pos(1,y); fflush(stdout);
				while (!(ch=inkey())) { /* wait for keypress */ ; }
				y=1;
				cls ();
				if((char) ch == 'q') return;
			}
		}
	}
	pos(1, y); puts("end"); pos(1,y); fflush(stdout);
	while (!(ch=inkey())) { /* wait for keypress */ ; }
	cls ();
}

/* Message handlers, work like printf */
void lt_msg(char *obj, char *fmt, ...) {
	/* 'object' message (yellow) */
	va_list ap;
	char str[80];
	va_start(ap, fmt);
	vsnprintf(str, 80, fmt, ap);
	add_buffer(obj, str, BROWN, BRIGHT);
	va_end (ap);
}

void mesg(char *fmt, ...) {
	/* general system message (green) */
	va_list ap;
	char str[80];
	va_start(ap, fmt);
	vsnprintf(str, 80, fmt, ap);
	add_buffer("system", str, GREEN, BRIGHT);
	va_end (ap);
}

void debug(char *fmt, ...) {
	/* debug (programmer's) message (brown) */
	if (debugmode) {
		va_list ap;
		char str[80];
		va_start(ap, fmt);
		vsnprintf(str, 80, fmt, ap);
		add_buffer("debug", str, BROWN, NORMAL);
		va_end (ap);
	}
}

void x10mesg(char *fmt, ...) {
	/* x10/cm11a device messages (teal) */
	va_list ap;
	char str[80];
	va_start (ap, fmt);
	vsnprintf(str, sizeof(str), fmt, ap);
	add_buffer("x10", str, CYAN, BRIGHT);
	va_end (ap);
}

void error(char *fmt, ...) {
	/* error messages (red) */
	va_list ap;
	char str[80];
	va_start (ap, fmt);
	vsnprintf(str, sizeof(str), fmt, ap);
	add_buffer("error", str, RED, BRIGHT);
	va_end (ap);
}

void echo(char *fmt, ...) {
	/* plain old text messages (grey) */
	va_list ap;
	char str[80];
	va_start (ap, fmt);
	vsnprintf(str, sizeof(str), fmt, ap);
	add_buffer("\0", str, GREY, NORMAL);
	va_end (ap);
}

/* clear log structs */
void buffer_clear() {
	bzero(&buffer, sizeof(buffer));
}

