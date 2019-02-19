CC=gcc
CFLAGS= -ggdb3 -pedantic -Werror -Wall -std=gnu++98

all: ringmaster player

ringmaster: ringmaster.c potato.h
	$(CC) $(CFLAGS) -o $@ ringmaster.cpp

player: player.c potato.h
	$(CC) $(CFLAGS) -o $@ player.cpp

clean:
	rm -rf ringmaster player *.o *~
