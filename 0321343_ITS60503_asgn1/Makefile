CC = gcc
CFLAGS = -Wall -pedantic -ansi
TARGETS = mainexe parent child01 child02 child03

all: mainexe

mainexe: mainexe.c
	$(CC) $(CFLAGS) -o mainexe mainexe.c

clean:
	rm -rf $(TARGETS) *~ *.log