#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define MAX_HOPS 512

typedef struct potato_t {
  int num_hops;
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
  char listen_port[6];
  char right_player_listen_port[6]; // Players connect towards right
} player;
