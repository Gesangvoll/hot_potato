#include "potato.h"

void input_format_check(int argc, char *argv[]) {
  if (argc != 3) {
    printf("Wrong Format!\n");
    exit(EXIT_FAILURE);
  }

  int port_num = atoi(argv[2]);
  if (port_num <= 1024 || port_num >= 65535) {
    printf("This could not be the port num of ringmaster!\n");
    exit(EXIT_FAILURE);
  }
}

int main(int argc, char *argv[]) {
  srand(time(NULL));
  input_format_check(argc, argv);
  const char *master_hostname = argv[1];
  const char *master_port_num = argv[2];
  struct addrinfo hints;
  struct addrinfo *master_addrinfo;
  memset(&hints, 0, sizeof(hints));
  /*************************Connect to Ringmaster*********************/
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  int getaddrinfo_status =
      getaddrinfo(master_hostname, master_port_num, &hints, &master_addrinfo);
  if (getaddrinfo_status != 0) {
    printf("Can not get master addrinfo! %s : %s\n", master_hostname,
           master_port_num);
    exit(EXIT_FAILURE);
  }
  // printf("Get master addrinfo succeed!\n");
  int to_master_fd =
      socket(master_addrinfo->ai_family, master_addrinfo->ai_socktype,
             master_addrinfo->ai_protocol);
  if (to_master_fd < 0) {
    printf("Can not create to_master socket on %d on %s : %s\n", to_master_fd,
           master_hostname, master_port_num);
    exit(EXIT_FAILURE);
  }
  // printf("Create to_master_fd succeed!\n");

  int connectStatus = connect(to_master_fd, master_addrinfo->ai_addr,
                              master_addrinfo->ai_addrlen);
  if (connectStatus != 0) {
    printf("Can not connect to ringmaster %s : %s\n", master_hostname,
           master_port_num);
    exit(EXIT_FAILURE);
  }
  // printf("Connect to master succeed!\n");

  /****Create Listen Socket and Send Listen Port Number to Master**********/
  char player_name[64];
  gethostname(player_name, sizeof(player_name));
  int player_listen_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (player_listen_fd < 0) {
    printf("Can not create player listen socket!\n");
    exit(EXIT_FAILURE);
  }
  int flag = 1;
  int setsockopt_status = setsockopt(player_listen_fd, SOL_SOCKET, SO_REUSEADDR,
                                     &flag, sizeof(int));
  if (setsockopt_status != 0) {
    printf("Set Sockopt Fails!\n");
    exit(EXIT_FAILURE);
  }

  struct sockaddr_in player_sockaddr;
  player_sockaddr.sin_family = AF_INET;
  player_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  player_sockaddr.sin_port = htons(0);
  int bindStatus = bind(player_listen_fd, (struct sockaddr *)&player_sockaddr,
                        sizeof(player_sockaddr));
  if (bindStatus != 0) {
    printf("Player can not bind!\n");
    exit(EXIT_FAILURE);
  }

  int listenStatus = listen(player_listen_fd, 100);
  if (listenStatus != 0) {
    printf("Error: Can not listen on %s", player_name);
    exit(EXIT_FAILURE);
  }
  // Get listen socket's port num, for later sending to master
  struct sockaddr_in player_sockaddr_get_port;
  socklen_t player_sockaddr_get_port_len = sizeof(player_sockaddr_get_port);
  int player_listen_port_int;
  char player_listen_port[6];
  if (getsockname(player_listen_fd,
                  (struct sockaddr *)&player_sockaddr_get_port,
                  &player_sockaddr_get_port_len) == 0) {
    player_listen_port_int = ntohs(player_sockaddr_get_port.sin_port);
    sprintf(player_listen_port, "%d", player_listen_port_int);
  } else {
    printf("Can not getsockname!\n");
    exit(EXIT_FAILURE);
  }

  // printf("Listen Port to Send char * is %s\n", player_listen_port);

  ssize_t sendStatus = send(to_master_fd, player_listen_port, 6, 0);
  if (sendStatus == -1) {
    printf("Can not send port number msg to master!\n");
    exit(EXIT_FAILURE);
  }
  // printf("Listen Port Sent\n");
  /*****************************Get Setup Info*****************************/
  player player_setup;
  // printf("Before Get Setup info....\n");
  ssize_t recvStatus =
      recv(to_master_fd, &player_setup, sizeof(player_setup), MSG_WAITALL);
  // printf("GEt Setup INfo........\n");
  if (recvStatus == -1) {
    printf("Can not recv setup info from master\n");
    exit(EXIT_FAILURE);
  }
  // printf("Recv setup info succeed!\n");

  /**************************Tell Master Ready, Then Claim Connected**********/
  sendStatus = send(to_master_fd, (char *)&player_setup.id, 6, 0);
  if (sendStatus == -1) {
    printf("Can not send confirm msg to master!\n");
    exit(EXIT_FAILURE);
  }
  printf("Connected as player %d out of %d total players\n", player_setup.id,
         player_setup.num_players);

  /*****************************Connect Right******************************/
  struct addrinfo right_hints;
  struct addrinfo *right_player_addrinfo;
  memset(&right_hints, 0, sizeof(right_hints));
  right_hints.ai_family = AF_UNSPEC;
  right_hints.ai_socktype = SOCK_STREAM;

  struct sockaddr_in *right_player_sockaddr =
      (struct sockaddr_in *)(&player_setup.right_addr);
  const char *right_player_name = inet_ntoa(right_player_sockaddr->sin_addr);
  // printf("Right player name and its listen port %s : %s\n",
  // right_player_name,
  //       player_setup.right_player_listen_port);
  getaddrinfo_status =
      getaddrinfo(right_player_name, player_setup.right_player_listen_port,
                  &right_hints, &right_player_addrinfo);
  if (getaddrinfo_status != 0) {
    printf("Can not get addrinfo of right player\n");
    exit(EXIT_FAILURE);
  }
  int connect_fd = socket(right_player_addrinfo->ai_family,
                          right_player_addrinfo->ai_socktype,
                          right_player_addrinfo->ai_protocol);
  if (connect_fd < 0) {
    printf("Can not create connect fd on %s : %s\n", right_player_name,
           player_setup.right_player_listen_port);
    exit(EXIT_FAILURE);
  }

  connectStatus = connect(connect_fd, right_player_addrinfo->ai_addr,
                          right_player_addrinfo->ai_addrlen);
  if (connectStatus == -1) {
    printf("Can not connect to right player!\n");
    exit(EXIT_FAILURE);
  }
  /* printf("Connect to Right Succeed, %s : (right player listen
   * port)%s\n", */
  /*        right_player_name, player_setup.right_player_listen_port); */

  /****************************Accept
   * Left*************************************/
  struct sockaddr_storage left_player_sockaddr;
  socklen_t left_player_sockaddr_len = sizeof(left_player_sockaddr);
  int connected_listen_fd =
      accept(player_listen_fd, (struct sockaddr *)&left_player_sockaddr,
             &left_player_sockaddr_len);
  if (connected_listen_fd < 0) {
    printf("Player # %d kept waiting for too long!\n", player_setup.id);
    exit(EXIT_FAILURE);
  }
  struct sockaddr_in *left_player_sockaddr_in =
      (struct sockaddr_in *)&left_player_sockaddr;
  char to_print_name[100];
  inet_ntop(left_player_sockaddr_in->sin_family,
            &left_player_sockaddr_in->sin_addr.s_addr, to_print_name,
            sizeof(to_print_name));
  /* printf("Accept left player's name and port %s : %d\n",
   * to_print_name, */
  /*        ntohs(left_player_sockaddr_in->sin_port)); */

  /****************************Play the
   * Game*********************************/
  fd_set game_fd_set;
  FD_ZERO(&game_fd_set);
  FD_SET(to_master_fd, &game_fd_set);
  FD_SET(connected_listen_fd, &game_fd_set);
  FD_SET(connect_fd, &game_fd_set);
  int max_fd = to_master_fd;
  if (to_master_fd < connected_listen_fd) {
    max_fd = connected_listen_fd;
  }
  if (max_fd < connect_fd) {
    max_fd = connect_fd;
  }

  int direction = rand() % 2;
  int coming_fd;
  potato potato;
  while (1) {
    if (select(max_fd + 1, &game_fd_set, NULL, NULL, NULL) == -1) {
      printf("Select() Fail!\n");
      exit(EXIT_FAILURE);
    }
    coming_fd = -1;
    if (FD_ISSET(to_master_fd, &game_fd_set)) {
      coming_fd = to_master_fd;
      if (potato.hops_to_go == potato.num_hops) {
        // printf("Recv a potato from master! First or shutdown?\n");
      }
    } else if (FD_ISSET(connected_listen_fd, &game_fd_set)) {
      coming_fd = connected_listen_fd;
    } else if (FD_ISSET(connect_fd, &game_fd_set)) {
      coming_fd = connect_fd;
    } else {
      printf("Did not find any potato came!\n");
      exit(EXIT_FAILURE);
    }
    recvStatus = recv(coming_fd, &potato, sizeof(potato), MSG_WAITALL);
    if (coming_fd == to_master_fd) {
      // printf("From master: potato num_hops : %d, hops_to_go : %d\n",
      //      potato.num_hops, potato.hops_to_go);
    } else if (coming_fd == connect_fd) {
      // printf("Potato coming from right\n");
    } else if (coming_fd == connected_listen_fd) {
      // printf("Potato coming from left\n");
    }
    if (recvStatus == -1) {
      printf("Can not recv during game!\n");
      exit(EXIT_FAILURE);
    }
    // sleep(1);
    if (potato.hops_to_go == -1 || potato.hops_to_go == -2) {
      break; // Shutdown
    }
    // printf("Appending trace! now %d\n", potato.num_hops - potato.hops_to_go);
    // printf("Append value %d ", player_setup.id);
    potato.trace[potato.num_hops - potato.hops_to_go] = player_setup.id;
    potato.count++;
    potato.hops_to_go--;
    // printf("Hops to go: %d\n", potato.hops_to_go);
    direction = rand() % 2;
    if (potato.hops_to_go == 0) {
      printf("I'm it\n");

      sendStatus = send(to_master_fd, &potato, sizeof(potato),
                        0); // Game End
      if (sendStatus == -1) {
        printf("Can not send end msg to master!\n");
        exit(EXIT_FAILURE);
      }
    } else {

      if (direction == 1) { // Right
        if (player_setup.id == player_setup.num_players - 1) {
          printf("Sending potato to %d\n", 0);
        } else {
          printf("Sending potato to %d\n", player_setup.id + 1);
        }

        sendStatus = send(connect_fd, &potato, sizeof(potato), 0);
        if (sendStatus == -1) {
          printf("Can not send potato ro right!\n");
          exit(EXIT_FAILURE);
        }
      } else {
        if (player_setup.id == 0) {
          printf("Sending potato to %d\n", player_setup.num_players - 1);
        } else {
          printf("Sending potato to %d\n", player_setup.id - 1);
        }

        sendStatus = send(connected_listen_fd, &potato, sizeof(potato), 0);
        // sleep(1);
        if (sendStatus == -1) {
          printf("Can not send potato ro left!\n");
          exit(EXIT_FAILURE);
        }
      }
    }

    FD_SET(to_master_fd, &game_fd_set);
    FD_SET(connected_listen_fd, &game_fd_set);
    FD_SET(connect_fd, &game_fd_set);
    int max_fd = to_master_fd;
    if (to_master_fd < connected_listen_fd) {
      max_fd = connected_listen_fd;
    }
    if (max_fd < connect_fd) {
      max_fd = connect_fd;
    }
  }
  close(to_master_fd);
  close(connected_listen_fd);
  close(connect_fd);
  exit(EXIT_SUCCESS);
}
