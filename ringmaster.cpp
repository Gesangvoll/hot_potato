#include "potato.hpp"
#include <cstring>
#include <iostream>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

using namespace std;

void input_format_check(int argc, char *argv[]) {
  if (argc != 4) {
    cerr << "Wrong Format!" << endl;
    exit(EXIT_FAILURE);
  }

  int port_num = atoi(argv[1]);
  if (port_num <= 1024 || port_num >= 65535) {
    cerr << "Unavailable Port Number!" << endl;
    exit(EXIT_FAILURE);
  }

  int num_players = atoi(argv[2]);
  if (num_players <= 1) {
    cerr << "Unavailable Number of Players!" << endl;
    exit(EXIT_FAILURE);
  }

  int num_hops = atoi(argv[3]);
  if (num_hops < 0 || num_hops > 512) {
    cerr << "Unavailable Number of Hops" << endl;
    exit(EXIT_FAILURE);
  }
}

int main(int argc, char *argv[]) {
  input_format_check(argc, argv);

  const char *port_num = argv[1];
  int num_players = atoi(argv[2]);
  int num_hops = atoi(argv[3]);

  const char *hostname = NULL;
  struct addrinfo hints;
  struct addrinfo *host_addrinfo;
  int master_fd;

  hints.ai_flags = AI_PASSIVE;
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  int getaddrinfo_status =
      getaddrinfo(hostname, port_num, &hints, &host_addrinfo);
  if (getaddrinfo_status != 0) {
    cerr << "Can not get addrinfo for localhost!" << hostname << ":" << port_num
         << endl;
    exit(EXIT_FAILURE);
  }

  master_fd = socket(host_addrinfo->ai_family, host_addrinfo->ai_socktype,
                     host_addrinfo->ai_protocol);
  if (master_fd < 0) {
    cerr << "Can not create master socket on " << master_fd << " on" << hostname
         << ":" << port_num << endl;
    exit(EXIT_FAILURE);
  }

  int flag = 1;
  int setsockopt_status =
      setsockopt(master_fd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(int));
  if (setsockopt_status != 0) {
    cerr << "Set Sockopt Fails" << endl;
    exit(EXIT_FAILURE);
  }

  int bindStatus =
      bind(master_fd, host_addrinfo->ai_addr, host_addrinfo->ai_addrlen);

  if (bindStatus != 0) {
    cerr << "Can not bind " << master_fd << endl;
    exit(EXIT_FAILURE);
  }

  int listenStatus = listen(master_fd, 100);
  if (listenStatus != 0) {
    cerr << "Error: Can not listen on " << hostname << ":" << port_num << endl;
    exit(EXIT_FAILURE);
  }
  cout << "Waiting for connection on port " << port_num << endl;

  /*********************** Loop of accept()*********************************/
  player players[num_players];

  for (int i = 0; i < num_players; i++) {
    memset(&players[i], 0, sizeof(player));
    struct sockaddr_storage client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    players[i].connected_socket_on_master =
        accept(master_fd, &players[i].player_addr, &client_addr_len);
    if (players[i].connected_socket_on_master == -1) {
      cerr << "I kept waiting for player # " << i
           << " before flowers were dead!" << endl;
      exit(EXIT_FAILURE);
    }

    char cur_player_port_num[6];
    ssize_t recv_size =
        recv(players[i].connected_socket_on_master, &cur_player_port_num,
             sizeof(cur_player_port_num), 0);
    if (recv_size == -1) {
      cerr << "Error: recv() could not recv player # " << i << endl;
      exit(EXIT_FAILURE);
    }
    memcpy(players[i].port, cur_player_port_num, sizeof(cur_player_port_num));
  }

  /***Setup players,tell players their id and neighbors,then recv response***/
  for (int i = 0; i < num_players; i++) {
    players[i].id = i;
    players[i].num_players = num_players;
    if (i == 0) {
      players[i].left_addr = players[num_players - 1].player_addr;
    } else {
      players[i].left_addr = players[i - 1].player_addr;
    }
    if (i == num_players - 1) {
      players[i].right_addr = players[0].player_addr;
      memcpy(players[i].right_port, players[i].port, 6);
    } else {
      players[i].right_addr = players[i + 1].player_addr;
      memcpy(players[i].right_port, players[i].port, 6);
    }
    int sendStatus = send(players[i].connected_socket_on_master, &players[i],
                          sizeof(player), 0);
    if (sendStatus == -1) {
      cerr << "Error: Could not send setup info to player #: " << i << endl;
      exit(EXIT_FAILURE);
    }
    int response_player_id;
    int recvStatus = recv(players[i].connected_socket_on_master,
                          &response_player_id, sizeof(int), MSG_WAITALL);
    if (recvStatus == -1) {
      cerr << "Error: Could not recv response from player # " << i << endl;
      exit(EXIT_FAILURE);
    }
    if (response_player_id == players[i].id && players[i].id == i) {
      cout << "Player " << i << " is ready to play" << endl;
    } else {
      cerr << "Error: player # " << i << " response setup wrongly" << endl;
      exit(EXIT_FAILURE);
    }
  }

  /**If num_hops is initially 0, tell players to shutdown and then shutdown*/
  if (num_hops == 0) {
    char shut_down[] = "shutdown";
    for (int i = 0; i < num_players; i++) {
      int sendStatus = send(players[i].connected_socket_on_master, shut_down,
                            sizeof(shut_down), 0);
      if (sendStatus == -1) {
        cerr << "Error: Could not send shutdown msg to player #: " << i << endl;
        exit(EXIT_FAILURE);
      }
      close(players[i].connected_socket_on_master);
    }
    close(master_fd);
    exit(EXIT_SUCCESS);
  }

  /******************************Start the Game****************************/
  srand((unsigned)time(NULL));
  int starter_player = rand() % num_players;
  potato potato;
  potato.hops_to_go = num_hops;
  cout << "Ready to start the game, sending potato to player " << starter_player
       << endl;
  int sendStatus = send(players[starter_player].connected_socket_on_master,
                        &potato, sizeof(potato), 0);
  if (sendStatus == -1) {
    cerr << "Error: Could not send start game msg to starter player #: "
         << starter_player << endl;
    exit(EXIT_FAILURE);
  }

  /****************************Select()***********************************/
  fd_set player_fd_set;
  FD_ZERO(&player_fd_set);
  int max_fd = players[0].connected_socket_on_master;
  for (int i = 0; i < num_players; i++) {
    FD_SET(players[i].connected_socket_on_master, &player_fd_set);
    if (players[i].connected_socket_on_master > max_fd) {
      max_fd = players[i].connected_socket_on_master;
    }
  }

  select(max_fd + 1, &player_fd_set, NULL, NULL, NULL);

  for (int i = 0; i < num_players; i++) {
    if (FD_ISSET(players[i].connected_socket_on_master, &player_fd_set)) {
      int recvStatuc = recv(players[i].connected_socket_on_master, potato.trace,
                            sizeof(potato.trace), 0);
      break;
    }
  }

  cout << "Trace of Potato:" << endl;
  for (int i = 0; i < num_hops; i++) {
    if (i == num_hops - 1) {
      cout << potato.trace[i] << endl;
    } else {
      cout << potato.trace[i] << ",";
    }
  }

  /************************Game End. Shutdown Gracefully***********************/
  char shut_down[] = "shutdown";
  for (int i = 0; i < num_players; i++) {
    int sendStatus = send(players[i].connected_socket_on_master, shut_down,
                          sizeof(shut_down), 0);
    if (sendStatus == -1) {
      cerr << "Error: Could not send shutdown msg to player #: " << i << endl;
      exit(EXIT_FAILURE);
    }
    close(players[i].connected_socket_on_master);
  }
  close(master_fd);
  exit(EXIT_SUCCESS);
}
