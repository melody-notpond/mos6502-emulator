##
## MOS6502 Emulator
## makefile
##
## jenra
## October 1 20202
##

CC = gcc
CFLAGS = -Wall -O0 -ggdb3

CODE = src/

all: *.o
	$(CC) $(CFLAGS) -o m6502 *.o

*.o: $(CODE)main.c $(CODE)m6502-src/*.c
	$(CC) $(CFLAGS) -c $?

clean:
	-rm *.o
