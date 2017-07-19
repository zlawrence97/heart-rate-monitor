# Ensure that these targest always run
.PHONY: test clean

CC=gcc
LDFLAGS=-lm
CFLAGS=-lsqlite3

OBJS=c-serial.o

all: c-serial.o

c-serial.o: c-serial.c c-serial-calcs.h db.h
	$(CC) $(CFLAGS) -c c-serial.c

build: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)


clean:
	# delete all compiled files
	rm -f c-serial.o
