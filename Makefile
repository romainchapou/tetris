CFLAGS=-std=gnu99 -Wall -Wextra -O3
LDFLAGS=-lncurses

all: tetris

tetris: tetris.c
	$(CC) $(CFLAGS) -o tetris tetris.c $(LDFLAGS)


.PHONY: clean
clean:
	rm -f *~ *.o tetris
