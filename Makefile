CC=gcc
CFLAGS=-std=gnu99 -Wall -Wextra -g3
LDFLAGS=-lncurses

all: tetris

tetris: tetris.c
	$(CC) $(CFLAGS) -o tetris tetris.c $(LDFLAGS)


.PHONY: clean
clean:
	rm -f *~ *.o tetris
