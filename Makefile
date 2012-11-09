OBJS = ansi.o main.o fileio.o window.o x10.o cm11a.o lights.o pump.o buffer.o
SRCS = ansi.c main.c fileio.c window.c x10.c cm11a.c lights.c pump.c buffer.c
CFLAGS = -g -I. -O1 -Wall -Wuninitialized -Wformat -Wcomment -Wswitch -Wunused -Wreturn-type
# CFLAGS = -I. -Os 
DT= date +%m.%d.%y-%H:%M
DATE != $(DT)
default	: $(OBJS)
	$(CC) $(CFLAGS) $(SRCS) -o x10control
backup : 
	tar -cz -f ~/x10control-$(DATE).tgz *.c *.h *.dat Makefile
clean :
	rm *.o *.ln *.core
$(OBJS)	: $(.PREFIX).c
	$(CC) $(CFLAGS) -c $(.PREFIX).c

