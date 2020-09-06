CC=gcc
CFLAGS=-std=c99 -Wall -Wextra -g
LDFLAGS=-lncurses

all: tetris

tetris: tetris.c
	$(CC) $(CFLAGS) -o tetris tetris.c $(LDFLAGS)


.PHONY: clean
clean:
	rm -f *~ *.o tetris
