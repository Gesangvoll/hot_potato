CC=g++
CFLAGS= #-pedantic -Werror -Wall #-O3 -fPIC  -lpthread
#CFLAGS=-ggdb3 -fPIC

all: ringmaster player

ringmaster: ringmaster.cpp potato.hpp
	$(CC) $(CFLAGS) -o $@ ringmaster.cpp

player: player.cpp potato.hpp
	$(CC) $(CFLAGS) -o $@ player.cpp

clean:
	rm -rf ringmaster player *.o *~
