CC=gcc
CFLAGS=-std=c99 -Wall -Wextra -g
LFLAGS=-lncurses

all: tetris

tetris: tetris.c
	$(CC) $(CFLAGS) -o tetris tetris.c $(LFLAGS)


.PHONY: clean
clean:
	rm -f *~ *.o tetris
