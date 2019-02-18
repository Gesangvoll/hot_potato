#include <cstring>
#include <iostream>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_HOPS 512

typedef struct potato_t {
  int hops_to_go;
  char trace[MAX_HOPS];
} potato;

typedef struct player_t {
  int id;
  int connected_socket_on_master;
  struct sockaddr player_addr;
  struct sockaddr left_addr;
  struct sockaddr right_addr;
  int num_players;
  char port[6];
  char right_port[6]; // Players connect towards right
} player;
