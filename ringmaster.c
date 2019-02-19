#include "potato.h"

void input_format_check(int argc, char *argv[]) {
  if (argc != 4) {
    printf("Wrong Format!\n");
    exit(EXIT_FAILURE);
  }

  int port_num = atoi(argv[1]);
  if (port_num <= 1024 || port_num >= 65535) {
    printf("Unavailable Port Number!\n");
    exit(EXIT_FAILURE);
  }

  int num_players = atoi(argv[2]);
  if (num_players <= 1) {
    printf("Unavailable Number of Players!\n");
    exit(EXIT_FAILURE);
  }

  int num_hops = atoi(argv[3]);
  if (num_hops < 0 || num_hops > 512) {
    printf("Unavailable Number of Hops\n");
    exit(EXIT_FAILURE);
  }
}

int main(int argc, char *argv[]) {
  input_format_check(argc, argv);
  potato shutdown_msg;
  const char *port_num = argv[1];
  const int num_players = atoi(argv[2]);
  int num_hops = atoi(argv[3]);
  printf("%s", port_num);
  const char *hostname = NULL;
  struct addrinfo hints;
  struct addrinfo *host_addrinfo;
  int master_fd;
  memset(&hints, 0, sizeof(hints));

  hints.ai_flags = AI_PASSIVE;
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  int getaddrinfo_status =
      getaddrinfo(hostname, port_num, &hints, &host_addrinfo);
  if (getaddrinfo_status != 0) {
    printf("Can not get addrinfo for localhost : %s\n", port_num);
    exit(EXIT_FAILURE);
  }

  master_fd = socket(host_addrinfo->ai_family, host_addrinfo->ai_socktype,
                     host_addrinfo->ai_protocol);
  if (master_fd < 0) {
    printf("Can not create master socket on %d on %s : %s\n", master_fd,
           hostname, port_num);
    exit(EXIT_FAILURE);
  }

  int flag = 1;
  int setsockopt_status =
      setsockopt(master_fd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(int));
  if (setsockopt_status != 0) {
    printf("Set Sockopt Fails\n");
    exit(EXIT_FAILURE);
  }

  int bindStatus =
      bind(master_fd, host_addrinfo->ai_addr, host_addrinfo->ai_addrlen);

  if (bindStatus != 0) {
    printf("Can not bind %d\n", master_fd);
    exit(EXIT_FAILURE);
  }

  int listenStatus = listen(master_fd, 100);
  if (listenStatus != 0) {
    printf("Error: Can not listen on %s : %s\n", hostname, port_num);
    exit(EXIT_FAILURE);
  }
  printf("Waiting for connection on port %s\n", port_num);

  /*********************** Loop of accept()*********************************/
  player players[num_players];

  for (int i = 0; i < num_players; i++) {
    memset(&players[i], 0, sizeof(player));
    struct sockaddr_storage client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    players[i].connected_socket_on_master =
        accept(master_fd, &players[i].player_addr, &client_addr_len);
    if (players[i].connected_socket_on_master == -1) {
      printf("I kept waiting for player # %d before flowers were dead!\n", i);
      exit(EXIT_FAILURE);
    }

    char cur_player_port_num[6];
    // player.cpp: 100
    ssize_t recv_size =
        recv(players[i].connected_socket_on_master, &cur_player_port_num,
             sizeof(cur_player_port_num), 0);
    if (recv_size == -1) {
      printf("Error: recv() could not recv player # %d's port number\n", i);
      exit(EXIT_FAILURE);
    }
    memcpy(players[i].listen_port, cur_player_port_num,
           sizeof(cur_player_port_num));
    printf("Player # %d's listen port is %s\n", i, players[i].listen_port);
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
      memcpy(players[i].right_player_listen_port, players[i].listen_port, 6);
    } else {
      players[i].right_addr = players[i + 1].player_addr;
      memcpy(players[i].right_player_listen_port, players[i].listen_port, 6);
    }
    ssize_t sendStatus = send(players[i].connected_socket_on_master,
                              &players[i], sizeof(players[i]), 0);
    if (sendStatus == -1) {
      printf("Error: Could not send setup info to player # %d\n", i);
      exit(EXIT_FAILURE);
    }
    int response_player_id;
    ssize_t recvStatus = recv(players[i].connected_socket_on_master,
                              &response_player_id, sizeof(int), MSG_WAITALL);
    if (recvStatus == -1) {
      printf("Error: Could not recv response from player # %d\n", i);
      exit(EXIT_FAILURE);
    }
    if (response_player_id == players[i].id && players[i].id == i) {
      printf("Player # %d is ready to play\n", i);
    } else {
      printf("Error: player # %d response setup wrongly\n", i);
      exit(EXIT_FAILURE);
    }
  }

  /**If num_hops is initially 0, tell players to shutdown and then shutdown*/
  if (num_hops == 0) {
    shutdown_msg.hops_to_go = -1;
    strcpy(shutdown_msg.trace, "shutdown");

    for (int i = 0; i < num_players; i++) {
      ssize_t sendStatus = send(players[i].connected_socket_on_master,
                                &shutdown_msg, sizeof(shutdown_msg), 0);
      if (sendStatus == -1) {
        printf("Error: Could not send shutdown msg to player # %d\n", i);
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
  potato.num_hops = num_hops;
  potato.hops_to_go = num_hops;
  printf("Ready to start the game, sending potato to player # %d",
         starter_player);
  ssize_t sendStatus = send(players[starter_player].connected_socket_on_master,
                            &potato, sizeof(potato), 0);
  if (sendStatus == -1) {
    printf("Error: Could not send start game msg to starter player # %d \n",
           starter_player);
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
      ssize_t recvStatus = recv(players[i].connected_socket_on_master,
                                potato.trace, sizeof(potato.trace), 0);
      if (recvStatus == -1) {
        printf("Error: Could not recv final potato from player # %d\n", i);
        exit(EXIT_FAILURE);
      }
      break;
    }
  }

  printf("Trace of Potato:\n");
  for (int i = 0; i < num_hops; i++) {
    if (i == num_hops - 1) {
      printf("%d", potato.trace[i]);
    } else {
      printf("%d,", potato.trace[i]);
    }
  }

  /************************Game End. Shutdown Gracefully***********************/
  shutdown_msg.hops_to_go = -1;
  strcpy(shutdown_msg.trace, "shutdown");

  for (int i = 0; i < num_players; i++) {
    ssize_t sendStatus = send(players[i].connected_socket_on_master,
                              &shutdown_msg, sizeof(shutdown_msg), 0);
    if (sendStatus == -1) {
      printf("Error: Could not send shutdown msg to player # %d\n ", i);
      exit(EXIT_FAILURE);
    }
    close(players[i].connected_socket_on_master);
  }
  close(master_fd);
  exit(EXIT_SUCCESS);
}
