/*
   $Id: lights.c,v 1.1 2005/08/20 17:34:21 dingo Exp $
   */
#include <main.h>
#include <buffer.h>
#include <lights.h>
#include <x10.h>
#include <cm11a.h>

/* Turn specified light on */
int light_on(int n) {
	struct Client_Command cc;
	/* Build command struct */
	cc.command = COMMAND_ON;
	cc.housecode = housecode_table[ tolower(light[n].housecode) -'a'];
	cc.device[0] = devicecode_table[light[n].devicecode -1];
	cc.devices = 1;
	/* Transmit signal */

	if (!transmit_command (&cc)) {
		/* failure, reset device */
		error("command not sent!\b");
		cm11a_close ();
		if((X10_FILENO = cm11a_open ()) < 0)
			error("Failed to initialize cm11a device!");
		light[n].retry +=1;
		return 0;
	} else {
		/* success */
		mesg("Turned %s on [%c,%i]",
			 light[n].name, light[n].housecode, light[n].devicecode);
		if (light[n].retry)
			mesg("Retried %i times", light[n].retry);
		light[n].state = 1;
		light[n].retry = 0;
		return 1;
	}
}

/* Turn specified light off, apply decay */
int light_off(int n) {
	struct Client_Command cc;
	debug("%02X, %02X", 
		  housecode_table[tolower(light[n].housecode)-'a'], 
		  devicecode_table[light[n].devicecode-1]);
	cc.command = COMMAND_OFF;
	cc.housecode = housecode_table[ tolower(light[n].housecode) -'a'];
	cc.device[0] = devicecode_table[light[n].devicecode -1];
	cc.devices = 1;

	if (!transmit_command (&cc)) {
		/* failure, reset device */
		error("command not sent!\b");
		cm11a_close ();
		if((X10_FILENO = cm11a_open ()) < 0)
			error("Failed to initialize cm11a device!");
		light[n].retry +=1;
		return 0;
	} else {
		/* success */
		mesg("Turned %s off [%c,%i]",
			 light[n].name, light[n].housecode, light[n].devicecode);
		if (light[n].retry)
			mesg("Retried %i times", light[n].retry);
		light[n].state = 0;
		light[n].retry = 0;
		if (light[n].decay)
			/* +/- offtime by light[n]->(int)decay */
			decay(n);
		return 1;
	}
}

int light_check_on(int x) {
	int ontime, offtime, realtime;
	struct tm *tm;
	time_t td;
	time(&td);
	tm = localtime(&td);
	realtime = (tm->tm_hour * 60) + tm->tm_min;
	ontime = (light[x].ontime.tm_hour * 60) + light[x].ontime.tm_min;
	offtime = (light[x].offtime.tm_hour * 60) + light[x].offtime.tm_min;
	if (light[x].ontime.tm_min == tm->tm_min &&
		light[x].ontime.tm_hour == tm->tm_hour)
	{	/* scheduled to turn oN at the apropriate time */
		mesg ("Light %s scheduled to turn oN now", light[x].name);
		light_on (x);
		return 1;
	} else if ((!tm->tm_sec%10) && ( /* check every 10 seconds */
		((ontime > offtime) && /* past midnight */
		 (realtime > ontime || realtime < offtime-light[x].decay)
		) ||
		((ontime < offtime) && /* throughout day */
		 (realtime > ontime && realtime < offtime-light[x].decay))))
	{	/* inapropriate time to be oFF */
		error ("Light %s should be oN now\a", light[x].name);
		light_on (x);
		return 1;
	}
	return 0;
}

int light_check_off(int x) {
	int ontime, offtime, realtime;
	struct tm *tm;
	time_t td;
	time(&td);
	tm = localtime(&td);
	realtime = (tm->tm_hour * 60) + tm->tm_min;
	ontime = (light[x].ontime.tm_hour * 60) + light[x].ontime.tm_min;
	offtime = (light[x].offtime.tm_hour * 60) + light[x].offtime.tm_min;

	if (light[x].offtime.tm_min == tm->tm_min &&
		light[x].offtime.tm_hour == tm->tm_hour) 
	{	/* scheduled to turn oFF at the apropriate time */
		mesg ("Light %s scheduled to turn oFF now", light[x].name);
		return 1;
	} else if ((!tm->tm_sec%10) && ( /* check every 10 seconds */
		((ontime > offtime) && /* past midnight */
		 (realtime > offtime && realtime < ontime)
		) ||
		((ontime < offtime) && /* throughout day */
		 (realtime > offtime || realtime < ontime))))
	{	/* inapropriate time to be oN */
		mesg ("Light %s should be oFF now", light[x].name);
		return 1;
	}
	return 0;
}

/* Check each light, turn oN or oFF if the time is right to do so,
 * return number of lights turned oN or oFF */
int lights_check() {
	register int x;
	int retval=0;
	for (x=0;x<LIGHTS;x++) {
		if (light[x].state == OFF) {
			/* light is off, check if it should be on */
			if (light_check_on(x))
				retval +=light_on(x);
		} else if (light[x].state == ON) {
			/* light is on, check if it should be off */
			if (light_check_off(x))
				retval +=light_off(x);
		} else {
			/* light is disabled */
		}
	}
	return retval;
}

/* Calculate a new offtime to decrease/increase the light period gradually */
void decay(int n) {
	char otime[9], ntime[9];
	time_t td;
	struct tm *tm;
	/* Get current time */
	time(&td);
	tm = localtime(&td);
	/* Set hour:min to offtime */
	tm->tm_hour = light[n].offtime.tm_hour;
	tm->tm_min = light[n].offtime.tm_min;
	tm->tm_sec = 0;
	/* Convert to seconds */
	td = mktime(tm);
	/* apply decay */
	td+= 60*(light[n].decay);
	strftime(otime, sizeof(otime), "%H:%M", tm);
	/* apply new time to light struct */
	tm = localtime(&td);
	light[n].offtime = *tm;
	strftime(ntime, sizeof(ntime), "%H:%M", &light[n].offtime);
	debug("time decay on light %i: %s->%s", n, otime, ntime);
}

/* clear out the lights structure */
void lights_clear(int n) {
	int x;
	time_t td;
	time(&td);

	/* zero out lights struct */
	bzero(&light, sizeof(light));
	for( (n >=0 ? (x = 0) : (x = n));
		 (n >=0 ? (x == n) : (x < LIGHTS));
		 x++) {
		/* set on and offtimes */
		localtime_r(&td, &light[x].offtime);
		localtime_r(&td, &light[x].ontime);

		/* disable state */
		light[x].state = -1;

		/* default name */
		snprintf(light[x].name, sizeof(light[x].name), "unused");
	}
	return;
}

int light_duration(int x) {
	if (light[x].ontime.tm_hour > light[x].offtime.tm_hour) return
		 /* light stays on past midnight */
			((24*60) - (light[x].ontime.tm_hour*60)+light[x].ontime.tm_min)
			+ ((light[x].offtime.tm_hour*60)+light[x].offtime.tm_min);
	else return
			((light[x].offtime.tm_hour*60)+light[x].offtime.tm_min)
			- ((light[x].ontime.tm_hour*60)+light[x].ontime.tm_min);
}

void lights_view(int x) {
	char strontime[12], strofftime[12];
	int duration = light_duration (x);
	strftime(strontime, sizeof strontime, "%H:%M", &light[x].ontime);
	strftime(strofftime, sizeof strofftime, "%H:%M", &light[x].offtime);
	if (light[x].state == -1)
		mesg("%i] %-13s disabled",
			 x, light[x].name);
	else {
		mesg("%i] %-13s [%c:%i], %i.%i:%i.%i, %s-%s %s%im/day",
			 x, light[x].name,
			 light[x].housecode, light[x].devicecode,
			 duration/60,duration%60, (1440-duration)/60,(1440-duration)%60,
			 strontime, strofftime,
			 light[x].decay > 0 ? "+":"-", abs(light[x].decay));
	}
}

/* convert light struct to a struct of strings for editing */
struct ltype_str light_2str (struct ltype light) {
	struct ltype_str str;
	snprintf(str.housecode, sizeof(str.housecode), "%c", toupper(
			 light.housecode));
	snprintf(str.devicecode, sizeof(str.devicecode), "%i",
			 light.devicecode);
	snprintf(str.name, sizeof(str.name), "%s",
			 light.name);
	strftime(str.ontime, sizeof(str.ontime), "%H:%M",
			 &light.ontime);
	strftime(str.offtime, sizeof(str.offtime), "%H:%M",
			 &light.offtime);
	snprintf(str.decay, sizeof(str.decay), "%2.2f",
			 light.decay);
	return str;
}

/* convert stuct of strings to light struct after editing */
struct ltype str2_light (struct ltype_str str) {
	struct ltype lt;
	if(!strptime(str.ontime, "%H:%M", &lt.ontime))
		error ("Bad ontime format '%s', must be of 'hh:mm'", str.ontime);
	if(!strptime(str.offtime, "%H:%M", &lt.offtime))
		error ("Bad offtime format '%s', must be of 'hh:mm'", str.offtime);
	snprintf(lt.name, sizeof(lt.name), "%s", str.name);
	lt.housecode = (char) toupper(str.housecode[0]);
	lt.devicecode = (int) atoi(str.devicecode);
	lt.decay = (long) atol(str.decay);
	return lt;
}
