/*
   $Id: main.c,v 1.1 2005/08/20 17:34:22 dingo Exp $
   */
#include <main.h>
#include <window.h>
#include <buffer.h>
#include <lights.h>
#include <fileio.h>
#include <cm11a.h>
#include <ansi.h>
#include <pump.h>
#include <x10.h>

/* poll the keyboard and cm11a for input */
int inkey(void) {
	int retval;
	struct timeval timeout;
	fd_set fdset;
	/* check lights and pumps for on/off status */
	lights_check ();
	if (pump.state != -1)
		pump_check ();
	/* save data every hour */
	if (save_check (0))
		error("error saving data");
	FD_ZERO(&fdset);
	FD_SET(STDIN_FILENO, &fdset);
	FD_SET(X10_FILENO, &fdset);
	/* 1 second timeout */
	timeout.tv_sec = 1;
	timeout.tv_usec = 0;
	if ( (retval=select(X10_FILENO+1, &fdset, NULL, NULL, &timeout)) == -1)
		error ("select() failed: %i", strerror);
	else {
		if (FD_ISSET(X10_FILENO, &fdset) != 0)
			/* X10 device */
			read_x10 ();
		if (FD_ISSET(STDIN_FILENO, &fdset) != 0)
			/* Keyboard */
			return fgetc(stdin);
		/* timeout */
		return 0;
	}
	/* should never happen */
	return -255;
}

/* Main Program */
int main(int argc, char **argv) {
	char cmd[128];
	char ibuf[2];
	int ch, running = 1, refresh = 2;

	cmd[0] = '\0';
	housecode = HOUSECODE_E; /* !! argv */
	debugmode = 1; /* !! argv */

	/* Initialization */
	cls ();
	if (load_data ())
		error("error loading data");
	ott = set_stdin ();
	if((X10_FILENO=cm11a_open ())<0)
		/* failed to initialize cm11a device */
		shutdown ();
	if (!cm11a_setclock (TIME_MONITOR_CLEAR))
		/* failed setting clock */
		running=0;
	if (!cm11a_status ())
		/* failed status request */
		running=0;

	/* begin MAIN loop */
	while (running) {
		/* redraw entire window every 60 seconds for console cleanup */
		if (!(time(NULL)%60))
			refresh=2;
		/* window refresh */
		if (refresh > 1)
			refresh_main_major ();
		if (refresh > 0)
			refresh_main_minor (cmd);
		refresh = 0;
		/* User input  */
		if((ch = inkey ())==0)
			/* no input */
			refresh=1;
		else if (isprint (ch) &&
				 ( isalpha (ch) ||
				   isdigit (ch) ||
				   isspace (ch)) ) {
			/* valid input */
			snprintf(ibuf, sizeof(ibuf), "%c", ch);
			if (strlen(cmd) < CMDLEN)
				strncat(cmd, ibuf, sizeof(cmd)-1-strlen(cmd));
			else
				putchar('\a');
			refresh=1;
		} else if (ch == 3)
			/* ^c, exit */
			running--;
		else if ((ch == 12)) {
			/* ^l, refresh */
			cls ();
			refresh = 2;
		} else if ((ch == 13 || ch == '\n' || ch == '\r') && strlen(cmd) > 0) {
			/* <CR>, process command */
			process (cmd);
			refresh = 2;
		} else if ((ch == 127 || ch == (int) '\b') && strlen(cmd) > 0) {
			/* backspace */
			cmd[strlen(cmd)-1] = '\0';
			refresh = 1;
		} else
			putchar('\a');
	} /* end MAIN loop */

	/* de-initialize and exit */
	reset_stdin ();
	shutdown ();
	return 0;
}

void process(char *cmd) {
	char *p;
	int x;
	/* BEGIN Command interpretor */
	if (process) {
		p = cmd;
		if (isdigit (*p)) {
			/*
			 * (N) (...)
			 */
			x = atoi (cmd);
			p++;
			if (!(x >= 0 && x < LIGHTS))
				error ("bad numeric, %i", x);
			else if (strlen(cmd) == 1) {
				/*
				 * N<cr>        toggle light on or off
				 */
				if (light[x].state == 0)
					light_on (x);
				else if (light[x].state == 1)
					light_off (x);
				else
					error("light %s is disabled", light[x].name);
			} else if (*p == ' ' && strlen(p) > 2) {
				/*
				 * (N) on|off|program|view|enable|disable|clear
				 */
				p++;
				if (!strncmp(p, "on", 2))
					light_on(x);
				else if (!strncmp(p, "off", 3))
					light_off(x);
				else if (!strncmp(p, "program", 6))
					program_light(x);
				else if (!strncmp(p, "view", 4))
					lights_view (x);
				else if (!strncmp(p, "enable", 6)) {
					if (light[x].state == -1)
						light[x].state = 0;
					else
						mesg ("light %s already is enabled (in state %s)",
							  light[x].name, light[x].state ? "off" : "on");
				} else if (!strncmp(p, "disable", 7)) {
					if (light[x].state == -1)
						mesg ("light %s already disabled", light[x].name);
					else
						light[x].state = -1;
				} else if (!strncmp(p,"clear",5))
					lights_clear (x);
				else
					error ("unknown light command:%s", p);
			} else
				error ("short command for numeric:%s", p);
		} else if (!strncmp(p,"p",1)) {
			if (strlen(cmd) == 1) {
				/*
				 * p<CR>        toggle pump on or off
				 */
				if (pump.state==0)
					pump_on ();
				else if (pump.state==1)
					pump_off ();
				else if (pump.state==-1)
					mesg ("Pump is disabled");
				else
					mesg ("Pump in unknown state %i", pump.state);
			} else if (strlen(cmd) > 3 && cmd[1] == ' ') {
				/*
				 * p on|off|program|view|disable|enable|clear
				 */
				p+=2;
				if (!strncmp(p, "off", 3))
					pump_off ();
				else if (!strncmp(p, "on", 2))
					pump_on ();
				else if (!strncmp(p, "program", 7))
					program_pump ();
				else if (!strncmp(p, "view", 4))
					pump_view ();
				else if (!strncmp(p, "enable", 6)) {
					if (pump.state == -1)
						pump.state = 0;
					else
						mesg("pump already enabled, in state %s",
							 pump.state ? "off" : "on");
				} else if (!strncmp(p, "disable", 7)) {
					if (pump.state == -1)
						mesg("pump already disabled");
					else
						pump.state = -1;
				} else if (!strncmp(p,"clear",5))
					pump_clear (-1);
			}
		} else if (!strncmp(p,"all ",4) && strlen(p) > 5) {
			/*
			 * all on|off|view
			 */
			p+=4;
			if (!strncmp(p, "off", 3))
				for(x=0;x<LIGHTS;x++)
					light_off(x);
			else if (!strncmp(p, "on", 2))
				for(x=0;x<LIGHTS;x++)
					light_on(x);
			else if (!strncmp(p, "view", 4)) {
				for(x=0;x<LIGHTS;x++)
					lights_view (x);
				pump_view ();
			} else
				error ("unknown command for all:%s", p);
		} else if ((!strncmp(p,"x ",2)) && strlen(p) > 3) {
			/*
			 * x status|setclock|reset|reinit
			 */
			p+=2;
			if (!strncmp(p,"status",6))
				cm11a_status ();
			else if (!strncmp(p,"setclock",8))
				cm11a_setclock (0);
			else if (!strncmp(p,"reset",5))
				cm11a_setclock (1);
			else if (!strncmp(p,"reinit",4)) {
				cm11a_close ();
				if((X10_FILENO = cm11a_open ()) < 0)
					error("Failed to initialize cm11a device!");
			} else
				error ("unknown x10 command, %s", p);
		} else if ((!strncmp(p,"b ",2)) && strlen(p) > 3) {
			/*
			 * b view|clear|save|dump
			 */
			p+=2;
			if (!strncmp(p,"view",4)) {
				lastlog ();
			} else if (!strncmp(p,"clear",5)) {
				buffer_clear ();
				draw_buf ();
			} else if (!strncmp(p,"save",4)) {
				save_buffer ();
			} else if (!strncmp(p,"dump",4)) {
				save_buffertxt ();
			}
		} else if (!strncmp(p,"e",1)) {
			popbox ("hello, world", 2);
		} else if (!strncmp(p,"debug",5)) {
			/* toggle debug mode */
			if (debugmode) debugmode=0;
			else debugmode = 1;
			mesg("debug mode %s", (debugmode?"on":"off"));
		} else if (!strncmp(p,"save",4)) {
			save_check (1);
		} else if (!strncmp(p,"help",4)
				   || !strncmp(p,"?",1)) {
			/* help command */
			echo (" (n) on|off|view|program|disable|enable|clear");
			echo (" p on|off|view|program|disable|enable|clear");
			echo (" x status|setclock|reset|reinit");
			echo (" b view|clear|save|dump|clear");
			echo (" all on|off|view");
			echo ("debug (toggles debug mode)");
			echo ("save (saves all data to disk)");
		} else if (strlen(p) > 0)
			error ("unknown command: %s", p);

		/* clear out command box */
		bar(ROWS-2);
		/* deplete cmd */
		cmd[0] = '\0';
	}
}

void shutdown(void) {
	error("system shutdown NOW!");
	if (cm11a_close ())
		error("cannot close cm11a device!");
	if (save_data ())
		error("all data was not saved to disk!");
	reset_stdin ();
	pos(1,24); bcolor(BLACK); fcolor (GREY, NORMAL);
	fflush(stdout);
	fpurge(stdin);
	puts("\n\n");
}
