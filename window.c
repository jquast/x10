/*
   $Id: window.c,v 1.1 2005/08/20 17:34:22 dingo Exp $
   */
#include <main.h>
#include <window.h>
#include <buffer.h>
#include <lights.h>
#include <ansi.h>
#include <pump.h>

void reset_stdin() {
	(void) tcsetattr (STDIN_FILENO, TCSAFLUSH, &ott);
}

struct termios set_stdin() {
	struct termios tt, rtt;
	bzero(&tt, sizeof(tt));
	bzero(&rtt, sizeof(rtt));

	/* get terminal attributes */
	if(tcgetattr (STDIN_FILENO, &tt)==-1) {
		error("Unable to get stdin tty attributes");
		return tt;
	}

	/* duplicate state */
	rtt = tt;

	/* define new state */
	cfmakeraw(&rtt);

	/* set new terminal state */
	if(tcsetattr (STDIN_FILENO, TCSAFLUSH, &rtt)==-1) {
		error("Unable to set stdin tty attributes");
		return tt;
	}

	/* return old terminal state */
	return tt;
}

/* Draw screen skeleton */
void refresh_main_major() {
	/* Top line, Time and program version */
	fcolor (GREY, NORMAL); bcolor (BLUE); bar(1);
	pos(2,1); puts ("Time:");
	pos(68,1); puts ("dingo@efnet");

	/* buffer window */
	box(1, BUF_Y, COLUMNS, BUF_HEIGHT, GREY, BLACK, NORMAL, ' ');
	draw_buf();

	/* command window */
	fcolor (GREY, BRIGHT); bcolor (BLACK); bar(ROWS-2);
	fflush(stdout);
}


void popbox(char *string, int timeout) {
	int width = strlen(string)+4;
	box( (COLUMNS/2)-(width/2), (ROWS/2)-2,
		 width, 4,
		 BLACK, RED, NORMAL, '*');
	fflush(stdout);
	while (inkey()) {;}
	sleep(1);
}

/* Draw information, states, etc. */
void refresh_main_minor(char *cmd) {
	char strtime[32];
	time_t td;
	int x;
	double loc;

	/* current system time */
	time(&td);
	strftime(strtime, sizeof strtime, "%a %m/%d %H:%M:%S",
			 localtime(&td));
	pos (8, 1); fcolor (BROWN, BRIGHT); bcolor (BLUE); 
	printf ("%s", strtime);

	/* light states */
	for(x=0;x<LIGHTS;x++) {
		if (light[x].state == -1) { /* disabled */
			box((2 + (BUTTONPADD * x)), 19, 
					 3, 3, GREY, RED, BRIGHT, x+'0');
			pos(1+(BUTTONPADD * x), 17); fcolor (RED, NORMAL);
			puts(light[x].name);
		} else if (light[x].state == 1) { /* on */
			box((2 + (BUTTONPADD * x)), 18, 
					 3, 3, GREY, GREEN, BRIGHT, x+'0');
			box((2 + (BUTTONPADD * x)), 21, 
					 3, 1, RED, BLACK, NORMAL, ' ');
			pos(1+(BUTTONPADD * x), 17); fcolor (BROWN, BRIGHT);
			puts(light[x].name);
		} else if (light[x].state == 0) { /* off */
			box((2 + (BUTTONPADD * x)), 18,
					 3, 1, GREEN, BLACK, NORMAL, ' ');
			box((2 + (BUTTONPADD * x)), 19,
					 3, 3, BROWN, RED, BRIGHT, x+'0');
			pos(1+(BUTTONPADD * x), 17); fcolor (BROWN, NORMAL);
			puts(light[x].name);
		}
	}

	if (pump.state != -1) {
		/* pump diagram */
		pos(PUMP_X, PUMP_Y);
		/* meter outline */
		fcolor(GREY, BRIGHT); bcolor(BLACK); putchar('['); 
		fcolor(BLACK, BRIGHT); bcolor(BLACK);
		for(x=0;x<PUMP_WIDTH+1;x++) putchar('.'); 
		fcolor(GREY, BRIGHT); bcolor(BLACK); putchar(']'); 
		pos(PUMP_X+2, PUMP_Y+1); fcolor(GREY, NORMAL);
		if (!pump.state) { /* off */
			loc = PUMP_WIDTH * ((float) ((pump.last + (pump.delay * 60)) - td)
							  / (float) (pump.delay * 60) );
			/* meter read */
			printf(" on in %4.1fm",
				   (float)((pump.last + (pump.delay * 60)) - td)/60);
			/* meter mark */
			pos(PUMP_X+PUMP_WIDTH-(int)loc, PUMP_Y); fcolor(RED, BRIGHT); 
			putchar('!');
		} else { /* on */
			loc = PUMP_WIDTH * ((float) ((pump.last + (pump.ontime)) - td)
							  / (float) (pump.ontime) );
			/* meter read */
			printf ("off in %4is", 
					(pump.last + (pump.ontime)) - td);
			/* meter mark */
			pos(PUMP_X+PUMP_WIDTH-(int)loc, PUMP_Y); fcolor(BLUE, BRIGHT); 
			putchar ('!');
		}
	} else {
		pos(PUMP_X+2, PUMP_Y+1); fcolor(GREY, NORMAL);
		puts ("pump disabled");
	}

	/* command line */
	fcolor (GREY, BRIGHT); bcolor (BLACK);
	pos(3,ROWS-2); 
	fcolor (GREY, NORMAL); printf ("cmd >%s \b",cmd);
	pos(3+strlen(cmd)+5,ROWS-2);
	fflush(stdout);
}

int field_input(int x, int y, void *field, int maxlen) {
	int input = 1, len;
	char ch, buff[16], ibuf[2];
	snprintf(buff, sizeof(buff), "%s", (char *)field);

	fcolor(GREY, BRIGHT); bcolor(RED); fflush(stdout);

	/* begin field entry */
	while (input) {
		len = strlen(buff);
		pos(x,y);
		printf(":%s \b", buff);
		fflush(stdout);
		/* input */
		if ((ch = inkey())==0)
			/* do nothing */ ;
		else if ((ch == '\n' || ch == '\r' || ch == '\t' || ch == 9) && len > 0)
			/* return or tab */
			input--;
		else if (isprint(ch) && len < maxlen-1) {
				snprintf(ibuf, sizeof(ibuf), "%c", ch);
				strncat(buff, ibuf, sizeof(buff)-1-strlen(buff));
		} else if ((ch == 127 || ch == (int) '\b')) {
			/* backspace */
			if (len > 0) {
				buff[len-1] = '\0';
				len--;
			} else putchar ('\a');
		} else putchar ('\a'); 
	} /* end field entry */
	fcolor(BLACK, NORMAL); bcolor(RED);

	/* copy buffer back into field */
	snprintf((char *)field, maxlen, "%s", buff);
	return 0;
}

void program_light(int n) {
	struct ltype_str old, new;
	struct ltype lnew;
	int y = BUF_Y-1;

	/* convert light structure into strings */
	new = old = light_2str(light[n]);

	/* black on red background */
	fcolor(BLACK, NORMAL); bcolor (RED); bar(y);

	/* field entry */
	mesg ("Programming light %s", light[n].name);
	pos(1, y); puts("   House code:    (A-P)  ");
	field_input (16, y, &new.housecode, 2);
	pos(1, y); puts("  Device code:    (1-16) ");
	field_input (16, y, &new.devicecode, 3);
	pos(1, y); puts("         Name:           ");
	field_input (16, y, &new.name, 13);
	pos(1, y); puts("   Start Time:         (hh:mm) ");
	field_input (16, y, &new.ontime, 6);
	pos(1, y); puts("     End Time:         (hh:mm) ");
	field_input (16, y, &new.offtime, 6);
	pos(1, y); puts("   Decay Rate:         (N or -N for +/- mins per day");
	field_input (16, y, &new.decay, 6);

	/* display differences */
	if (strncmp(old.housecode, new.housecode, sizeof(old.housecode)))
		mesg("* Change housecode %c->%c", old.housecode[0], new.housecode[0]);
	if (strncmp(old.devicecode, new.devicecode, sizeof(old.devicecode)))
		mesg("* Change devicecode %s->%s", old.devicecode, new.devicecode);
	if (strncmp(old.name, new.name, sizeof(old.name)))
		mesg("* Change name %s->%s", old.name, new.name);
	if (strncmp(old.ontime, new.ontime, sizeof(old.ontime)))
		mesg("* Change start time %s->%s", old.ontime, new.ontime);
	if (strncmp(old.offtime, new.offtime, sizeof(old.offtime)))
		mesg("* Change offtime %s->%s", old.offtime, new.offtime);
	if (old.decay[0] != new.decay[0])
		mesg("* Change decay %s->%s", old.decay, new.decay);

	fcolor(BLACK, BLINK); bcolor (GREEN); bar(y);
	fcolor(BLACK, NORMAL); bcolor (GREEN);

	/* Confirm changes */
	pos(30, y); puts("Save Changes? y/n");

	if(tolower(fgetc(stdin)) == 'y') {
		mesg("* Saving changes");
		/* convert strings back into light structures */
		lnew = str2_light (new);
		lnew.state = light[n].state;
		light[n] = lnew;
	} else error("* Changes left unsaved");

	fcolor (GREY, NORMAL); bcolor (BLACK); bar(y);

	fflush(stdout);
}

void program_pump() {
	struct ptype_str old, new;
	struct ptype pnew;
	int y = BUF_Y-1;

	/* convert light structure into strings */
	new = old = pump_2str(pump);

	/* black on red background */
	fcolor(BLACK, NORMAL); bcolor (RED); bar(y);

	/* field entry */
	pos(1, y); puts("   House code:    (A-P)  ");
	field_input (16, y, &new.housecode, 2);
	pos(1, y); puts("  Device code:    (1-16) ");
	field_input (16, y, &new.devicecode, 3);
	pos(1, y); puts("        Delay:         (minutes)");
	field_input (16, y, &new.delay, 6);
	pos(1, y); puts("      On Time:         (seconds)");
	field_input (16, y, &new.ontime, 6);

	/* display differences */
	if (strncmp(old.housecode, new.housecode, sizeof(old.housecode)))
		mesg("* Change housecode %c->%c", old.housecode[0], new.housecode[0]);
	if (strncmp(old.devicecode, new.devicecode, sizeof(old.devicecode)))
		mesg("* Change devicecode %s->%s", old.devicecode, new.devicecode);
	if (strncmp(old.ontime, new.ontime, sizeof(old.ontime)))
		mesg("* Change on time %s->%s", old.ontime, new.ontime);
	if (strncmp(old.delay, new.delay, sizeof(old.delay)))
		mesg("* Change delay %s->%s", old.delay, new.delay);

	fcolor(BLACK, BLINK); bcolor (GREEN); bar(y);
	fcolor(BLACK, NORMAL); bcolor (GREEN);

	/* Confirm changes */
	pos(30, y); puts("Save Changes? y/n");

	if(tolower(fgetc(stdin)) == 'y') {
		mesg("* Saving changes");
		/* convert strings back into pump structure */
		pnew = str2_pump (new);
		pnew.state = pump.state;
		pump = pnew;
	} else error("* Changes left unsaved");

	fcolor (GREY, NORMAL); bcolor (BLACK); bar(y);

	fflush(stdout);
}

/* Display buffer window */
void draw_buf() {
	char strtime[32];
	register int n, x;

	for(n=0;n<BUF_HEIGHT-2 && buffer[n].message[0] != '\0'; n++) {
		pos(2, BUF_Y+BUF_HEIGHT-2-n);

		if (buffer[n].objname[0] != NULL) {
			strftime(strtime, sizeof strtime, "%m/%d %H:%M",
					 localtime(&buffer[n].td));

			/* time, cyan */
			fcolor (CYAN, NORMAL);
			printf("<%11s>", strtime);

			/* object, specified color */
			fcolor (buffer[n].color, buffer[n].attr);
			printf("%6s ", buffer[n].objname);

			/* message, white */
			fcolor (GREY, NORMAL);
			printf("%s.", buffer[n].message);

			/* delete artifacts with trailing spaces */
			for(x=24+strlen(buffer[n].message); x < COLUMNS -2; x++)
				putchar(' ');
		}
		else {
			/* Plain old echo (un-timestampped) */
			printf(" %s", buffer[n].message);

			for(x =strlen(buffer[n].message); x < COLUMNS -3; x++)
				putchar(' ');

		}
	}
	fflush(stdout);
}
