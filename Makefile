# Ensure that these targest always run
.PHONY: test clean

CC=gcc
LDFLAGS=-lm
CFLAGS=-lsqlite3

OBJS=terminal-code.o

all: terminal-code.o

terminal-code.o: terminal-code.c terminal-code.h db.h
	$(CC) $(CFLAGS) -c terminal-code.c

build: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)


clean:
	# delete all compiled files
	rm -f terminal-code.o
