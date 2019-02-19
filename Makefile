CC=gcc
CFLAGS= -ggdb3 -pedantic -Werror -Wall -std=gnu99

all: ringmaster player

ringmaster: ringmaster.c potato.h
	$(CC) $(CFLAGS) -o $@ ringmaster.c

player: player.c potato.h
	$(CC) $(CFLAGS) -o $@ player.c

clean:
	rm -rf ringmaster player *.o *~
