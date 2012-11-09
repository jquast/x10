/*
   $Id: fileio.c,v 1.1 2005/08/20 17:34:21 dingo Exp $
   */
#include <main.h>
#include <fileio.h>
#include <buffer.h>
#include <lights.h>
#include <pump.h>

int save_check(int force) {
	struct tm *tm;
	time_t td;
	int retval=0;
	time(&td);
	tm = localtime(&td);
	/* Save at *:59 */
	if (force ||
		(tm->tm_min == 59 &&
		 tm->tm_hour != lastsaved.hour &&
		 tm->tm_yday != lastsaved.yday)) {
		lastsaved.hour = tm->tm_hour;
		lastsaved.yday = tm->tm_yday;
		debug ("Saving data to disk");
		retval += save_data ();
		retval += save_buffertxt();
	}
	return retval;
}

/* Save all data */
int save_data() {
	int retval=0;
	retval+= save_buffer();
	retval+= save_lights();
	retval+= save_pump();
	return retval;
}

/* Load all data */
int load_data() {
	int retval=0;
	retval+= lastsaved.hour = lastsaved.yday = 0;
	retval+= load_buffer();
	retval+= load_lights();
	retval+= load_pump();
	return retval;
}

/* load: buffer.dat */
int load_buffer() {
	int err=0, i;
	FILE *fd;
	/* LOAD: buffer.dat */
	if ((fd = fopen("buffer.dat", "r"))!=NULL) {
		for(i=0;i<BUFLEN;i++)
			if(fread(&buffer[i], sizeof(struct btype), 1, fd)!=1)
				if(!feof(fd)) err++;
		fclose(fd);
	} else
		err++;
	if(err) {
		buffer_clear ();
		error("buffer.dat: %s", strerror(errno));
		return 1;
	} else
		debug("Loaded buffer.dat");
	return 0;
}

/* save: buffer.dat */
int save_buffer() {
	int err=0, i;
	FILE *fd;
	/* SAVE: buffer.dat */
	if ((fd = fopen("buffer.dat", "w"))!=NULL) {
		for(i=0;i<BUFLEN;i++)
			if(fwrite(&buffer[i], sizeof(struct btype), 1, fd)!=1)
				if(!feof(fd)) err++;
		fclose(fd);
	} else
		err++;
	if(err) {
		error("buffer.dat: %s", strerror(errno));
		return 1;
	} else
		debug("Saved buffer.dat");
	return 0;
}

/* save: buffer.log ASCII text */
int save_buffertxt() {
	int err=0, i;
	char timestr[32];
	FILE *fd;
	/* SAVE: buffer.dat */
	if ((fd = fopen("buffer.log", "w"))!=NULL) {
		for(i=BUFLEN;i>=0;i--)
			if(buffer[i].message[0] != '\0') {
				strftime(timestr, sizeof(timestr), "%m/%d %H:%M:%S",
						 localtime(&buffer[i].td));
				fprintf(fd, "%s %s %s\n", 
						timestr, buffer[i].objname, buffer[i].message);
			}
		fclose(fd);
	} else
		err++;
	if(err) {
		error("buffer.log: %s", strerror(errno));
		return 1;
	} else
		debug("Saved buffer.log");
	return 0;
}

/* load: lights.dat */
int load_lights() {
	int err=0, i;
	FILE *fd;
	/* LOAD: lights.dat */
	if ((fd = fopen("lights.dat", "r"))!=NULL) {
		for(i=0;i<LIGHTS;i++)
			if(fread(&light[i], sizeof(struct ltype), 1, fd)!=1)
				if(!feof(fd)) err++;
		fclose(fd);
	} else
		err++;
	if(err) {
		error("lights.dat: %s", strerror(errno));
		lights_clear(-1);
		return 1;
	} else
		debug("Saved lights.dat");
	return 0;
}

/* save: lights.dat */
int save_lights() {
	int err=0, i;
	FILE *fd;
	if ((fd = fopen("lights.dat", "w"))!=NULL) {
		for(i=0;i<LIGHTS;i++)
			if(fwrite(&light[i], sizeof(struct ltype), 1, fd)!=1)
				if(!feof(fd)) err++;
		fclose(fd);
	} else
		err++;
	if(err) {
		error("lights.dat: %s", strerror(errno));
		return 1;
	}
	else
		debug("Saved lights.dat");
	return 0;
}

/* load: pump.dat */
int load_pump() {
	FILE *fd;
	int err=0;
	err=0;
	if ((fd = fopen("pump.dat", "r"))!=NULL) {
		if(fread(&pump, sizeof(struct ptype), 1, fd)!=1)
			if(!feof(fd)) err++;
		fclose(fd);
	} else
		err++;
	if(err) {
		error("pump.dat: %s", strerror(errno));
		pump_clear ();
		return 1;
	} else
		debug("Loaded pump.dat");
	return 0;
}

/* save: pump.dat */
int save_pump() {
	int err=0;
	FILE *fd;
	if ((fd = fopen("pump.dat", "w"))!=NULL) {
		if(fwrite(&pump, sizeof(struct ptype), 1, fd)!=1)
			if(!feof(fd)) err++;
		fclose(fd);
	} else
		err++;
	if(err) {
		error("pump.dat: %s", strerror(errno));
		return 1;
	} else
		debug("Saved pump.dat");
	return 0;
}
