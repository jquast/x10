/*
   $Id: pump.c,v 1.1 2005/08/20 17:34:22 dingo Exp $
   */
#include <main.h>
#include <pump.h>
#include <buffer.h>
#include <ansi.h>
#include <x10.h>
#include <cm11a.h>

void pump_on() {
	time_t td;
	struct Client_Command cc;
	unsigned char hc = housecode_table[ tolower(pump.housecode) -'a'];
	unsigned char dc = devicecode_table[ pump.devicecode -1];
	time(&td);
	cc.command = COMMAND_ON;
	cc.housecode = hc;
	cc.device[0] = dc;
	cc.devices = 1;
	if (!transmit_command (&cc)) {
		/* failure, reset device */
		error("command not sent!");
		cm11a_close ();
		if((X10_FILENO = cm11a_open ()) < 0)
			error("Failed to initialize cm11a device!");
		pump.retry +=1;
	} else {
		/* success */
		mesg("Turned pump on");
		pump.state = 1;
		pump.last = td;
		pump.retry = 0;
	}
	return;
}

void pump_off() {
	time_t td;
	struct Client_Command cc;
	unsigned char hc = housecode_table[ tolower(pump.housecode) -'a'];
	unsigned char dc = devicecode_table[ pump.devicecode -1];
	time(&td);
	cc.command = COMMAND_OFF;
	cc.housecode = hc;
	cc.device[0] = dc;
	cc.devices = 1;
	if (!transmit_command (&cc)) {
		/* failure, reset device */
		error("command not sent!");
		cm11a_close ();
		if((X10_FILENO = cm11a_open ()) < 0)
			error("Failed to initialize cm11a device!");
		pump.retry +=1;
	} else {
		/* success */
		mesg("Turned pump off");
		pump.state = 0;
		pump.last = td;
		pump.retry = 0;
	}
	return;
}

int pump_check() {
	time_t td;
	time(&td);
	if (pump.retry > 0) {
		error("Retrying pump for command %s #%i",
			  pump.state == 1 ? "on" : "off", pump.retry);
		if (pump.state == 0)
			pump_on ();
		else if (pump.state == 1)
			pump_off ();
	} else if (pump.state == -1)
		/* pump is disabled */
		return 1;
	else if (pump.state == 0 && /* pump off */
			 (pump.last + pump.delay*60 - td <= 0)) /* ontime reached */
	{
		pump_on();
		return 0;
	} else if (pump.state == 1 && /* pump on */
			   (pump.last + pump.ontime - td <= 0)) /* offtime reached */
	{
		pump_off();
		return 0;
	}
	return 1;
}
void pump_view () {
	mesg("pump turns on for %im%is every %ih%im",
		 pump.ontime/60, pump.ontime%60, pump.delay/60, pump.ontime%60);
	mesg("pump turned %s %im%is ago",
		 pump.state>0?"on":"off",
		 (time(NULL)-pump.last)/60,
		 (time(NULL)-pump.last)%60 );
}

void pump_clear() {
	time_t td;
	time(&td);
	bzero(&pump, sizeof(pump));
	/* non-zeros */
	pump.state = -1;
	pump.housecode = 'A';
	pump.devicecode = 1;
	pump.last = td;
	pump.delay = 5;
	pump.ontime = 80;
	pump.retry = 0;
}

struct ptype_str pump_2str (struct ptype pump) {
	struct ptype_str str;
	snprintf(str.housecode, sizeof(str.housecode), 
			 "%c", pump.housecode);
	snprintf(str.devicecode, sizeof(str.devicecode), 
			 "%i", pump.devicecode);
	snprintf(str.ontime, sizeof(str.ontime), 
			 "%i", pump.ontime);
	snprintf(str.delay, sizeof(str.delay), 
			 "%i", pump.delay);
	return str;
}

struct ptype str2_pump (struct ptype_str str) {
	struct ptype lt;
	lt.housecode = (char) str.housecode[0];
	lt.devicecode = atoi(str.devicecode);
	lt.ontime = atol(str.ontime);
	lt.delay = atol(str.delay);
	return lt;
}
