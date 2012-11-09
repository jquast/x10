/*
   $Id: cm11a.c,v 1.1 2005/08/20 17:34:21 dingo Exp $
   */
#include <main.h>
#include <x10.h>
#include <cm11a.h>
#include <buffer.h>

/* return checksum of cm11a message, used to match against what the cm11a
 * reports after a successful transmit */
unsigned char cm11a_chksum(void *buf, int len) {
	int i, checksum = 0;
	for(i=0; i < len; i++)
		checksum += ((u_char *) buf)[i] & 0xff;
	return (unsigned char) checksum;
}

/* Create file descriptor for cm11a serial device
 *      Baud Rate:      4,800bps
 *      Parity:         None
 *      Data Bits:      8
 *      Stop Bits:      1
 */
int cm11a_open() {
	struct termios termios;
	int x10; /* file descriptor (return value) */

	x10mesg("initializing %s as cm11a device", DEVFILE);

	if((x10=open(DEVFILE, O_RDWR | O_NOCTTY | O_NDELAY)) < 0) {
		error("Could not open device %s:%s",
			  DEVFILE, strerror(errno));
		return -1;
	}

	/* nonblocking */
	if(fcntl(x10, F_SETFL, O_NONBLOCK) == -1) {
		error("Could not set non-blocking mode on device %s:%s",
			  DEVFILE, strerror(errno));
		return -1;
	}

	/* Get current tty settings. */
	if(tcgetattr(x10, &termios) != 0) {
		error("Could not get tty attributes from device %s:%s",
			  DEVFILE, strerror(errno));
		return -1;
	}

	/* Enable receiver. */
	termios.c_cflag |= CLOCAL | CREAD;

	/* Set to 8N1. */
	termios.c_cflag &= ~PARENB;
	termios.c_cflag &= ~CSTOPB;
	termios.c_cflag &= ~CSIZE;
	termios.c_cflag |=  CS8;

	/* Accept raw data. */
	cfmakeraw(&termios);

	/* Return after 1 character available */
	termios.c_cc[VMIN]=1;

	/* cm11a transfers at 4800bps */
	if(cfsetspeed(&termios, B4800) != 0) {
		error("Could not set device %s to 4800bps:%s",
			  DEVFILE, strerror(errno));
		return -1;
	}

	/* Apply changes */
	if(tcsetattr(x10, TCSANOW, &termios) != 0) {
		error("Could not set device %s attributes:%s",
			  DEVFILE, strerror(errno));
		return -1;
	}
	return x10;
}

/* Close the file descriptor for the cm11a serial device */
int cm11a_close () {
	struct termios termios;

	x10mesg ("shutting down device %s", DEVFILE);

	if(tcgetattr(X10_FILENO, &termios) != 0)
		error("Could not get device attributes:%s", strerror);

	/* setting baud rate to 0 clears DTR */
	cfsetspeed(&termios, B0);

	if(tcsetattr(X10_FILENO, TCSAFLUSH, &termios) != 0) {
		error("Could not set device %s attributes:%s",
			  DEVFILE, strerror(errno));
	}

	return close(X10_FILENO);
}

/*
 * Build time structure to send to cm11a device
 *
 *  0x9b         Set Interface Clock Request                     (byte 0)
 *
 *  Bit range
 *  47-40        Current time (seconds)                          (byte 1)
 *  39-32        Current time (minutes ranging from 0 to 119)    (byte 2)
 *  31-24        Current time (hours/2, ranging from 0 to 11)    (byte 3)
 *  23-15        Current year day (MSB is bit 15)                (byte 4+.1)
 *  14-8         Day mask (SMTWTFS)                              (byte 5-.1)
 *  7-4          Monitored house code                            (byte 6...)
 *  3               Reserved
 *  2               Timer purge flag
 *  1               Battery timer clear flag
 *  0               Monitored status clear flag
 */
void x10_build_time(char *data, int house_code, int flags) {
	time_t td;
	struct tm *tm;
	int x;

	(void) time(&td);
	tm = localtime(&td);

	/* byte Zero    :Command code */
	data[0]= (unsigned char) 0x9b;

	/* byte One     :Seconds */
	data[1]= (char) tm->tm_sec;

	/* byte Two     :Minutes from 0 to 119 (Alternate of hours) */
	data[2]= (char) tm->tm_min;
	if(tm->tm_hour % 2)
		data[2] += (char) 60;

	/* byte Three   :Hours/2. */
	data[3]= (char) tm->tm_hour/2;

	/* Byte 4&5     :day_of_week and day_of_year */
	data[5]= (char) 0x01;
	data[6]= (char) house_code;

	for(x=tm->tm_wday;x<6;x++)
		data[5]= data[5] << 1;

	if(tm->tm_yday>256) {
		/* MSB is part of day_of_week byte */
		data[4]= tm->tm_yday -256;
		data[5]= data[5] | 0x80;
	} else
		data[4]= tm->tm_yday;

	/* byte Six     :flags bits.
	   (bit 2 timer purge,
	    bit 1 battery timer,
	    bit 0 monitored status clear) */
	data[6] |= (char) flags;

	return;
}

/* return day of week for use with cm11a_status */
char *cm11a_day (int b) {
	switch (b) {
		case 1: return "Sun"; break;
		case 2: return "Mon"; break;
		case 4: return "Tue"; break;
		case 8: return "Wed"; break;
		case 16: return "Thu"; break;
		case 32: return "Fri"; break;
		case 64: return "Sat"; break;
		default: return "ERR"; break;
	}
}

/* cm11a status request
 *
 * reports device battery usage, date/time, firmware, housecode, 
 * and monitored device status (not implemented)
 *
 * Bit range    Description
 * 111 to 96    Battery timer (set to 0xffff on reset)          (Byte 0-1)
 * 95 to 88     Current time (seconds)                          (Byte 2 )
 * 87 to 80     Current time (minutes ranging from 0 to 119)    (Byte 3)
 * 79 to 72     Current time (hours/2, ranging from 0 to 11)    (Byte 4)
 * 71 to 63     Current year day (MSB bit 63)                   (Byte 5+)
 * 62 to 56     Day mask (SMTWTFS)                              (Byte 6-)
 * 55 to 52     Monitored house code                            (Byte 7 lo)
 * 51 to 48     Firmware revision level 0 to 15                 (Byte 7 hi)
 * 47 to 32     Currently addressed monitored devices           (Byte 8-9)
 * 31 to 16     On / Off status of the monitored devices        (Byte 10-11)
 * 15 to 0      Dim status of the monitored devices             (Byte 12-13)
 */
int cm11a_status() {
	int batlife;
	char data[14] = {
		(char) 0x8b,
		'\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0'
	};

	x10mesg ("device status request");

	/* write status request */
	if (!x10_write (&data,1)) {
		error ("status write FAILED");
		return 0;
	}

	/* recieve status structure into data buffer */
	if (!x10_read(&data, 14) == 14) {
		error ("status read FAILED");
		return 0;
	}

	/* battery life */
	batlife = (data[1] << 8) || (unsigned int) data[0];
	if (batlife == 0xffff)
		debug ("Unknown battery usage");
	else
		x10mesg ("Battery usage: %dh%dm", batlife/60, batlife % 60);
	if (batlife > 2880)
		/* x10 says cm11a lasts up to 400 hours on battery
		 * ...48 hours seems more reasonable. */
		error ("you should replace the battery soon.", batlife);

	/* firmware and housecode */
	debug ("Firmware revision %d, Monitored housecode %X",
		   data[7] & 0x0f, (data[7] & 0x0f) >> 5);

	/* date and time */
	x10mesg ("day %00d, %s %02d:%02d:%02d",
		   data[5] + (((data[6] & 0x80) >> 7) * 256),       /* day of year */
		   cm11a_day( data[6] & 0x07f),                     /* weekday    */
		   ((data[4] & 0xff) * 2) + ((data[3] & 0xff) /60), /* hours     */
		   (data[3] & 0xff) % 60,                           /* minutes  */
		   (data[2] & 0xff)                                 /* seconds */
		  );

	/* success */
	return 1;
}

int cm11a_setclock(int flags) {
	char data[7] = { 0,0,0,0,0,0,0 };

	switch(flags) {
		case TIME_MONITOR_CLEAR:
			x10mesg("resetting Monitor housecode"); break;
		case TIME_TIMER_PURGE:
			x10mesg("purging timer"); break;
		case TIME_BATTERY_TIMER_CLEAR:
			x10mesg("resetting battery timer"); break;
		case 0:
			x10mesg("Synchronizing clock"); break;
		default:
			error("Unknown flags %i in cm11a_setclock", flags); return 0;
	}

	/* build cm11a time structure */
	x10_build_time(data, housecode, flags);

	/* write message + checksum */
	if (!x10_write_message(&data, 7, cm11a_chksum (&data[1],6) )) {
		/* failure */
		error("setclock FAIL");
		return 0;
	}

	/* success */
	return 1;
}
