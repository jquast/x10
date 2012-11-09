/*
   $Id: x10.c,v 1.1 2005/08/20 17:34:22 dingo Exp $
   */
#include <main.h>
#include <buffer.h>
#include <cm11a.h>
#include <ansi.h>
#include <lights.h>
#include <pump.h>
#include <x10.h>

char housecode_letter[]={
	'm','e','c','k','o','g','a','i','n','f','d','l','p','h','b','j'};

unsigned char housecode_table[] = {
	HOUSECODE_A, HOUSECODE_B, HOUSECODE_C, HOUSECODE_D, HOUSECODE_E,
	HOUSECODE_F, HOUSECODE_G, HOUSECODE_H, HOUSECODE_I, HOUSECODE_J,
	HOUSECODE_K, HOUSECODE_L, HOUSECODE_M, HOUSECODE_N, HOUSECODE_O,
	HOUSECODE_P };

int devicecode_number[]={
	13, 5, 3, 11, 15, 7, 1, 9, 14, 6, 4, 12, 16, 8, 2, 10 };

unsigned char devicecode_table[] = {
	DEVICECODE_1, DEVICECODE_2, DEVICECODE_3, DEVICECODE_4, 
	DEVICECODE_5, DEVICECODE_6, DEVICECODE_7, DEVICECODE_8, 
	DEVICECODE_9, DEVICECODE_10, DEVICECODE_11, DEVICECODE_12, 
	DEVICECODE_13, DEVICECODE_14, DEVICECODE_15, DEVICECODE_16 };

char *command_name[] = {
	"ALL_UNITS_OFF", "ALL_LIGHTS_ON", "ON", "OFF", "DIM", "BRIGHT", "ALL_LIGHTS_OFF", "EXTENDED_CODE", "HAIL_REQUEST", "HAIL_ACKNOWLEDGE", "PRESET_DIM1", "PRESET_DIM2", "EXTENDED_DATA_TRANSFER", "STATUS_ON", "STATUS_OFF", "STATUS_REQUEST" };

void status_display_event(Event *event) {
	int i;
	char string_buffer[64], ddbuf[32], dbuf[16];
	string_buffer[0] = ddbuf[0] = dbuf[0] = '\0';

	/* Housecode */
	snprintf(string_buffer, sizeof(string_buffer),
			 "Housecode %c, ", toupper( housecode_letter[ event->housecode]));

	/* Devices */
	if(event->devices) {
		for(i=0; i < event->devices; i++) {
			snprintf(dbuf, sizeof(dbuf), "%i,",
					 devicecode_number[event->device[i]]);
			strncat(ddbuf, dbuf,
					sizeof(ddbuf)-strlen(ddbuf)-1);
		}
		snprintf(dbuf, sizeof(dbuf),
				 (event->devices > 1 ? "Devices %s" : "Device %s"), ddbuf);
		strncat(string_buffer, dbuf,
				sizeof(string_buffer)-strlen(string_buffer)-1);
	}

	/* Command */
	snprintf(dbuf, sizeof(dbuf), " %s", command_name[event->command]);
	strncat(string_buffer, dbuf,
			sizeof(string_buffer)-strlen(string_buffer)-1);

	/* DIM / BRIGHT */
	if(event->command == COMMAND_DIM || event->command == COMMAND_BRIGHT) {
		/* Print the dim/bright level. */
		snprintf(dbuf, sizeof(dbuf), ", Dim/Bright %i", event->extended1);
		strncat(string_buffer, dbuf,
				sizeof(string_buffer)-strlen(string_buffer)-1);
	}

	/* Was it an extended? */
	else if(event->command == COMMAND_EXTENDED_DATA_TRANSFER) {
		/* Print the two bytes. */
		snprintf(dbuf, sizeof(dbuf), ", extended %02X,%02X", 
				 event->extended1, event->extended2);
		strncat(string_buffer, dbuf,
				sizeof(string_buffer)-strlen(string_buffer)-1);
	}

	/* Display */
	x10mesg(string_buffer);

	return;
}

/* Check events for changes of state and change our own accordingly,
 * in case another transmitter is active, we will be aware of the proper
 * state of things. Because of this feature of the cm11a, we could set up two
 * devices for redundancy, if reliability was an important issue */
void event_to_states(Event *event) {
	register int i, x;

	/* check lights */
	for(x=0; x < LIGHTS; x++)
		if(toupper(light[x].housecode) ==
		   toupper(housecode_letter[event->housecode]))
			for(i=0; i < event->devices; i++)
				if(light[x].devicecode==devicecode_number[event->device[i]])
				{
					if (event->command==COMMAND_ON) {
						light[x].state=1;
						x10mesg("ON command to light '%s' overheard",
								light[x].name);
					}
					else if (event->command==COMMAND_OFF) {
						light[x].state=0;
						x10mesg("OFF command to light '%s' overheard",
								light[x].name);
					} else {
						x10mesg("unknown command '%s' to light '%s' overheard",
								command_name[ event->command],
								light[x].name);
					}
				}
	/* check pump */
	if (toupper(pump.housecode) == toupper(housecode_letter[event->housecode]))
		for(i=0; i < event->devices; i++)
			if(pump.devicecode == devicecode_number[event->device[i]])
			{
				pump.last = event->time;
				if (event->command==COMMAND_ON) {
					x10mesg("ON command to pump overheard");
					pump.state=1;
				}
				else if (event->command==COMMAND_OFF) {
					x10mesg("OFF command to pump overheard");
					pump.state=0;
				} else
					x10mesg("unknown command %s to pump overheard",
							command_name[ event->command]);
			}
	return;
}

/* Transmit a client command to the x10 hardware. */
int transmit_command (struct Client_Command *client_command) {
	int i;
	unsigned char data[2];

	/* Transmit all the addresses for devicecodes. */
	for(i=0; i < client_command->devices; i++) {
		debug("Addressing Housecode:%c Devicecode:%i for Cmd %s",
				toupper( housecode_letter[ client_command->housecode]),
				devicecode_number[ client_command->device[i]],
				command_name[ client_command->command]
			   );

		/*  Set the header byte to be an address.*/
		data[0]=HEADER_DEFAULT;

		/* The second byte is the house:device.*/
		data[1]=(client_command->housecode << 4) | client_command->device[i];

		/* Write the address to the x10 hardware. */
		if(!x10_write_message(&data, 2, cm11a_chksum(&data,2) )) {
			error("Couldn't set address.");
			return 0;
		}
	}

	/* Send the command to the x10 hardware. */

	/* Set the header to be a function. */
	data[0]=HEADER_DEFAULT | HEADER_FUNCTION;

	/* If this is a dim/bright, the header also has the value. */
	if(client_command->command == COMMAND_DIM
	   || client_command->command == COMMAND_BRIGHT) {
		data[0]=(data[0] | client_command->value << 3) & 0xff;
	}

	/* The second byte is house:command. */
	data[1]=(client_command->housecode << 4) | client_command->command;

	/* Send the message to the x10 hardware. */
	if(!x10_write_message(&data, 2, cm11a_chksum(&data,2) )) {
		error("Couldn't send function.");
		return 0;
	}
	return 1;
}

/*
 * Handle reading and interpreting data from the x10.
 */

unsigned char address_data[16];
int address_buffer_count=0;
unsigned char address_buffer_housecode;

/* Handle reading and handling requests from the x10. */
int read_x10() {
	unsigned char command;

	/* Read the byte sent by the x10 hardware. */
	if(x10_read(&command, 1) != 1) {
		error("Could not read command byte.");
		return 0;
	}

	/* Is this a data poll? */
	if(command == 0x5a) {
		debug("Received poll from x10.");
		return read_x10_poll();
	}

	/* Is this a power-fail time request poll? */
	else if(command == 0xa5) {
		x10mesg("Received power failure time request");
		if (!cm11a_setclock(TIME_TIMER_PURGE) ) {
			error("Timeout trying to send power-fail time request.");
			return 0;
		}
	}

	/* It was an unknown command (probably static or leftovers). */
	else {
		error("Unknown command byte from x10: %02x.", command);
		return 0;
	}

	return 1;
}

/*
 * Handles a data poll request from the x10. 
 */
int read_x10_poll() {
	unsigned char x10_data[8];
	unsigned char command;
	unsigned char buffer_size;
	unsigned char function_byte;
	char string_data[80], dbuf[80];
	int i,j;
	Event event;

	/* Acknowledge the x10's poll. */
	command=0xc3;
	if(x10_write(&command, 1) != 1) {
		debug("Error writing poll acknowledgement");
		return 1;
	}

	debug("Poll acknowledgement sent, waiting to receive");

	/* Get the size of the request. */
	if(x10_read(&buffer_size, 1) != 1) {
		debug("Did not get buffer size response");
		return 1;
	}

	/* Must have at least 2 bytes or it's just weird. */
	if(buffer_size < 2) {
		debug("Short request from x10.");
		return 1;
	}

	/* Read in the function byte. */
	if(x10_read(&function_byte, 1) != 1) {
		debug("Could not read function byte.");
		return 1;
	}

	/* Read in the buffer from the x10. */
	if(x10_read(&x10_data, buffer_size - 1) != buffer_size - 1) {
		debug("Gave up while reading the buffer.");
		return 1;
	}

	/* Print packet info to debug. */
	snprintf(string_data, sizeof(string_data),"data (size:%i)(func:%02X)",
			 buffer_size, function_byte);
	for(i=0; i < buffer_size - 1; i++) {
		snprintf(dbuf, sizeof(dbuf), " %02x", x10_data[i]);
		strncat(string_data, dbuf, sizeof(string_data)-strlen(string_data)-1);
	}
	debug(string_data);

	/* Decode the packet. */
	for(i=0; i < buffer_size - 1; i++) {
		if(function_byte & 1) {
			/* function */
			if((x10_data[i] >> 4) != address_buffer_housecode) {
				debug("Function housecode and address housecode mismatch.");
				address_buffer_count=0;
			}

			/* Package the event. */
			event.command=x10_data[i] & 0x0f;
			event.housecode=x10_data[i] >> 4;
			event.devices=address_buffer_count;
			event.time=time(NULL);
			for(j=0; j < address_buffer_count; j++) {
				event.device[j]=address_data[j];
			}

			/* Was it a function that needs an extra byte? */
			if(event.command == COMMAND_DIM
			   || event.command == COMMAND_BRIGHT) {
				/* We'd better have an extra byte. */
				if((i + 1) >= (buffer_size - 1)) {
					debug("Missing extra byte of function.");
					continue;
				}
				/* Save the extra byte and advance 1. */
				event.extended1=x10_data[++i];
				function_byte=function_byte >> 1;
			}

			/* Was it a function that needs two extra bytes? */
			else if(event.command == COMMAND_EXTENDED_DATA_TRANSFER) {
				/* We'd better have two extra bytes. */
				if((i + 2) >= (buffer_size - 1)) {
					debug("Missing extra 2 bytes of function.");
					continue;
				}
				/* Save the extra bytes and advance 2. */
				event.extended1=x10_data[++i];
				event.extended2=x10_data[++i];
				function_byte=function_byte >> 2;
			}

			/* display event details */
			status_display_event(&event);

			/* Take action on this event. */
			event_to_states(&event);

			/* Flush the address buffer. */
			address_buffer_count=0;
		}

		/* This was an address byte. */
		else {
			/*
			 * If this is the first byte, set the address
			 * housecode.
			 */
			if(address_buffer_count == 0) {
				address_buffer_housecode=x10_data[i] >> 4;
			}

			/*
			 * If this address doesn't match the housecode of
			 * the addresses we were storing, we start over with
			 * this address.  This should match how the hardware
			 * handles things like A1,B2,function.
			 */
			else if(address_buffer_housecode != x10_data[i] >> 4) {
				debug("Address buffer housecode mismatch.");
				address_buffer_housecode=x10_data[i] >> 4;
				address_buffer_count=0;
			}

			/* The address buffer better not be full. */
			else if(address_buffer_count == 16) {
				debug("Address buffer overflow.");
				/* Just ignore this address. */
				continue;
			}

			/* Save it on the address buffer. */
			address_data[address_buffer_count++]=x10_data[i] & 0x0f;
		}

		/* Shift the function byte, we've handled this one. */
		function_byte=function_byte >> 1;
	}
	/* partial success */
	return 0;
}

/*
 * Wait for the x10 hardware to provide us with some data.
 */
int x10_wait_read() {
	fd_set read_fd_set;
	struct timeval tv;
	int retval;

	/* Wait for data to be readable. */
	for(;;) {

		/* Make the call to select to wait for reading. */
		FD_ZERO(&read_fd_set);
		FD_SET(X10_FILENO, &read_fd_set);
		tv.tv_sec = 5;
		tv.tv_usec = 0;
		retval=select(X10_FILENO+1, &read_fd_set, NULL, NULL, &tv);

		/* Did select error? */
		if(retval == -1) {

			/* If it's an EINTR, go try again. */
			if(errno == EINTR) {
				error("Signal recieved in read select, restarting.");
				continue;
			} else
				error("read select: %s", strerror(errno));
		}
		if(retval)
			/* success */
			return 1;
		else
			/* failure */
			return 0;
	}
}

/* 
 * Wait for the x10 hardware to be writable.
 */
int x10_wait_write() {
	fd_set write_fd_set;
	struct timeval tv;
	int retval;

	/* Wait for data to be writable. */
	for(;;) {

		/* Make the call to select to wait for writing. */
		FD_ZERO(&write_fd_set);
		FD_SET(X10_FILENO, &write_fd_set);
		tv.tv_sec = 1;
		tv.tv_usec = 0;
		retval=select(X10_FILENO+1, NULL, &write_fd_set, NULL, &tv);

		/* Did select error? */
		if(retval == -1) {

			/* If it's an EINTR, go try again. */
			if(errno == EINTR) {
				debug("Signal recieved in write select, restarting.");
				continue;
			}

			/* It was something weird. */
			error("Error in write select: %s", strerror(errno));
		}

		/* Can we write data? */
		if(retval) {	

			/* We can write some data, return ok. */
			return(1);
		}

		/* No data writable. */
		else {

			/* We can't write any data, this is a fail. */
			return(0);
		}
	}
}


/* 
 * Read data from the x10 hardware.
 *
 * Basically works like read(), but with a select-provided readable check
 * and timeout.
 * 
 * Returns the number of bytes read.  This might be less than what was given
 * if we ran out of time.
 */
int x10_read(void *buf, int count) {
	int bytes_read, retval, i;
	char string_buffer[32], dbuf[32];

	/* Read the request into the buffer. */
	for(bytes_read=0; bytes_read < count;) {

		/* Wait for data to be available. */
		if(!x10_wait_read()) {
			error("Gave up waiting for x10 to be readable.");
			return(bytes_read);
		}

		/* Get as much of it as we can.  Loop for the rest. */
		retval=read(X10_FILENO, (char *) buf + bytes_read, 
					(size_t) count - bytes_read);
		if(retval == -1) {
			error("Failure reading x10 buffer: %s", strerror(errno));
		}
		bytes_read += retval;
		debug("Read %i bytes, %i left", retval, count - bytes_read);
	}

	snprintf(string_buffer, sizeof(string_buffer), 
			 "read %02x", ((char *) buf)[0]);
	for(i=1;i<bytes_read;i++) {
		snprintf(dbuf, sizeof(dbuf), ",%02x", ((char *) buf)[i]);
		strncat(string_buffer, dbuf,
				sizeof(string_buffer)-strlen(string_buffer)-1);
	} debug(string_buffer);

	/* We're all done. */
	return(bytes_read);
}


/* 
 * Write data to the x10 hardware.
 *
 * Basically works like write(), but with a select-provided writeable check
 * and timeout.
 * 
 * Returns the number of bytes written.  This might be less than what was
 * given if we ran out of time.
 */
int x10_write(void *buf, int count) {
	int bytes_written;
	int retval;

	int i;
	char string_buffer[64], dbuf[64];
	snprintf(string_buffer, sizeof(string_buffer), 
			 "write %02x", ((char *) buf)[0]);
	for(i=1;i<count;i++) {
		snprintf(dbuf, sizeof(dbuf), ":%02x", ((char *) buf)[i]);
		strncat(string_buffer, dbuf,
				sizeof(string_buffer)-strlen(string_buffer)-1);
	} debug(string_buffer);

	/* Write the buffer to the x10 hardware. */
	for(bytes_written=0; bytes_written < count;) {

		/* Wait for data to be writeable. */
		if(!x10_wait_write()) {
			error("Gave up waiting for x10 to be writeable.");
			return(bytes_written);
		}

		/* Get as much of it as we can.  Loop for the rest. */
		retval=write(X10_FILENO, (char *) buf + bytes_written, 
					 (size_t) count - bytes_written);
		if(retval == -1) {
			error("Failure writing x10 buffer.");
		}
		bytes_written += retval;
		debug("Wrote %i bytes, %i left", retval, count - bytes_written);
	}

	/* We're all done. */
	return(bytes_written);
}

/*
 * Write a message to the x10 hardware.
 *
 * The data will be sent, a checksum from the x10 hardware will be expected,
 * a response to the checksum will be sent, and the x10 should signal us
 * ready.
 *
 * Sometimes the cm11a will kick into poll mode while we're trying to send
 * it a request then promptly ignore us until we do something about it.  To
 * handle this, if that looks like what is happening, we go deal with the
 * x10 then come back and try again.
 *
 * If it works, we return true, false otherwise.
 */
int x10_write_message(void *buf, int count, unsigned char check) {
	unsigned char sum=0;
	unsigned char temp;
	int try_count;

	/* retry twice */
	for(try_count=1; try_count < 3; try_count++) {

		/* Send the data. */
		if(x10_write(buf, count) != count) {
			debug("Failed to send data on try %i.", try_count);
			continue;
		}

		/* Get the checksum byte */
		if(x10_read(&sum, 1) != 1) {
			debug("Failed to get the checksum byte on try %i.", try_count);
			continue;
		}

		/* Make sure the checksums match. */
		if(sum != check) {
			error("Checksum mismatch (%02x,%02x)", check, sum);

			/* Does this look like it was really a poll? */
			if(sum == 0x5a) read_x10_poll();

			/* re-transmit message */
			continue;
		} else
			debug("checksum OK %02X==%02X", check, sum);

		/* Send a go-ahead to the x10 hardware. */
		/* PC responds with 0x00 when checksum matches */
		temp=0x00;
		if(x10_write(&temp, 1) != 1) {
			error("Failed to send go-ahead on try %i.", try_count);
			continue;
		}

		/* Get the ready byte from the x10 hardware. */
		if(x10_read(&temp, 1) != 1) {
			error("Failed to get the 'ready' byte on try %i.", try_count);
			continue;
		}

		/* It had better be 0x55, the 'ready' byte. */
		if(temp != 0x55) {
			error("Exptected ready byte, got %02x on try %i.", temp, try_count);

			/* Does this look like it was really a poll? */
			if(temp == 0x5a) read_x10_poll();

			/* Retry sending. */
			continue;
		}

		/* We made it, return true. */
		return(1);
	}

	/* We gave up and failed. */
	return(0);
}
