#include "potato.hpp"

using namespace std;

void input_format_check(int argc, char *argv[]) {
  if (argc != 3) {
    cerr << "Wrong Format!" << endl;
    exit(EXIT_FAILURE);
  }

  int port_num = atoi(argv[2]);
  if (port_num <= 1024 || port_num >= 65535) {
    cerr << "This could not be the port num of ringmaster!" << endl;
    exit(EXIT_FAILURE);
  }
}

int main(int argc, char *argv[]) {
  input_format_check(argc, argv);
  const char *master_hostname = argv[1];
  const char *master_port_num = argv[2];
  struct addrinfo hints;
  struct addrinfo *master_addrinfo;

  /*************************Connect to Ringmaster*********************/
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  int getaddrinfo_status =
      getaddrinfo(master_hostname, master_port_num, &hints, &master_addrinfo);
  if (getaddrinfo_status != 0) {
    cerr << "Can not get master addrinfo!" << master_hostname << ":"
         << master_port_num << endl;
    exit(EXIT_FAILURE);
  }
  int to_master_fd =
      socket(master_addrinfo->ai_family, master_addrinfo->ai_socktype,
             master_addrinfo->ai_protocol);
  if (to_master_fd < 0) {
    cerr << "Can not create to_master socket on " << to_master_fd << " on "
         << master_hostname << " : " << master_port_num << endl;
    exit(EXIT_FAILURE);
  }

  int connectStatus = connect(to_master_fd, master_addrinfo->ai_addr,
                              master_addrinfo->ai_addrlen);
  if (connectStatus != 0) {
    cerr << "Can not connect to ringmaster " << master_hostname << " : "
         << master_port_num << endl;
    exit(EXIT_FAILURE);
  }

  /****Create Listen Socket and Send Listen Port Number to Master**********/
  char player_name[64];
  gethostname(player_name, sizeof(player_name));
  int player_listen_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (player_listen_fd < 0) {
    cerr << "Can not create player listen socket!" << endl;
    exit(EXIT_FAILURE);
  }
  int flag = 1;
  int setsockopt_status = setsockopt(player_listen_fd, SOL_SOCKET, SO_REUSEADDR,
                                     &flag, sizeof(int));
  if (setsockopt_status != 0) {
    cerr << "Set Sockopt Fails!" << endl;
    exit(EXIT_FAILURE);
  }

  struct sockaddr_in player_sockaddr;
  player_sockaddr.sin_family = AF_INET;
  player_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  player_sockaddr.sin_port = htons(0);
  int bindStatus = bind(player_listen_fd, (struct sockaddr *)&player_sockaddr,
                        sizeof(player_sockaddr));
  if (bindStatus != 0) {
    cerr << "Player can not bind!" << endl;
    exit(EXIT_FAILURE);
  }

  int listenStatus = listen(player_listen_fd, 100);
  if (listenStatus != 0) {
    cerr << "Error: Can not listen on " << player_name << endl;
    exit(EXIT_FAILURE);
  }
  // Get listen socket's port num, for later sending to master
  struct sockaddr_in player_sockaddr_get_port;
  socklen_t player_sockaddr_get_port_len = sizeof(player_sockaddr_get_port);
  int player_listen_port;
  if (getsockname(player_listen_fd,
                  (struct sockaddr *)&player_sockaddr_get_port,
                  &player_sockaddr_get_port_len) == 0) {
    player_listen_port = ntohs(player_sockaddr_get_port.sin_port);
  } else {
    cerr << "Can not getsockname!" << endl;
    exit(EXIT_FAILURE);
  }
  ssize_t sendStatus = send(to_master_fd, (char *)&player_listen_port, 6, 0);
  if (sendStatus == -1) {
    cerr << "Can not send port number msg to master!" << endl;
    exit(EXIT_FAILURE);
  }

  /*****************************Get Setup Info*****************************/
  player player_setup;
  ssize_t recvStatus =
      recv(to_master_fd, &player_setup, sizeof(player_setup), MSG_WAITALL);
  if (recvStatus == -1) {
    cerr << "Can not recv setup info from master" << endl;
    exit(EXIT_FAILURE);
  }

  /*****************************Connect Right******************************/
  struct addrinfo right_hints;
  struct addrinfo *right_player_addrinfo;
  right_hints.ai_family = AF_UNSPEC;
  right_hints.ai_socktype = SOCK_STREAM;

  struct sockaddr_in *right_player_sockaddr =
      (struct sockaddr_in *)(&player_setup.right_addr);
  const char *right_player_name = inet_ntoa(right_player_sockaddr->sin_addr);
  getaddrinfo_status =
      getaddrinfo(right_player_name, player_setup.right_player_listen_port,
                  &right_hints, &right_player_addrinfo);
  if (getaddrinfo_status != 0) {
    cerr << "Can not get addrinfo of right player" << endl;
    exit(EXIT_FAILURE);
  }
  int connect_fd = socket(right_player_addrinfo->ai_family,
                          right_player_addrinfo->ai_socktype,
                          right_player_addrinfo->ai_protocol);
  if (connect_fd < 0) {
    cerr << "Can not create connect fd on " << right_player_name << " : "
         << player_setup.right_player_listen_port << endl;
    exit(EXIT_FAILURE);
  }

  connectStatus = connect(connect_fd, right_player_addrinfo->ai_addr,
                          right_player_addrinfo->ai_addrlen);
  if (connectStatus == -1) {
    cerr << "Can not connect to right player!" << endl;
    exit(EXIT_FAILURE);
  }

  /****************************Accept Left*************************************/
  struct sockaddr_storage left_player_sockaddr;
  socklen_t left_player_sockaddr_len = sizeof(left_player_sockaddr);
  int connected_listen_fd =
      accept(player_listen_fd, (struct sockaddr *)&left_player_sockaddr,
             &left_player_sockaddr_len);
  if (connected_listen_fd < 0) {
    cerr << "Player # " << player_setup.id << " kept waiting for too long!"
         << endl;
    exit(EXIT_FAILURE);
  }

  /**************************Tell Master Ready, Then Claim Connected**********/
  sendStatus = send(to_master_fd, (char *)&player_setup.id, 6, 0);
  if (sendStatus == -1) {
    cerr << "Can not send confirm msg to master!" << endl;
    exit(EXIT_FAILURE);
  }
  cout << "Connected as player " << player_setup.id << " out of "
       << player_setup.num_players << " total players" << endl;

  /****************************Play the Game*********************************/
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
  srand((unsigned)time(NULL));
  int direction;
  int coming_fd;
  potato potato;
  while (1) {
    if (select(max_fd + 1, &game_fd_set, NULL, NULL, NULL) == -1) {
      cerr << "Select() Fail!" << endl;
      exit(EXIT_FAILURE);
    }
    if (FD_ISSET(to_master_fd, &game_fd_set)) {
      coming_fd = to_master_fd;
    } else if (FD_ISSET(connected_listen_fd, &game_fd_set)) {
      coming_fd = connected_listen_fd;
    } else if (FD_ISSET(connect_fd, &game_fd_set)) {
      coming_fd = connect_fd;
    } else {
      cerr << "Did not find any potato came!" << endl;
      exit(EXIT_FAILURE);
    }
    recvStatus = recv(coming_fd, &potato, sizeof(potato), MSG_WAITALL);
    if (recvStatus == -1) {
      cerr << "Can not recv during game!" << endl;
      exit(EXIT_FAILURE);
    }
    if (potato.hops_to_go == -1) {
      break; // Shutdown
    }
    potato.trace[potato.num_hops - potato.hops_to_go] = player_setup.id + '0';
    potato.hops_to_go--;

    if (potato.hops_to_go == 0) {
      cout << "I'm it\n" << endl;

      sendStatus = send(to_master_fd, potato.trace, sizeof(potato.trace),
                        0); // Game End
      if (sendStatus == -1) {
        cerr << "Can not send end msg to master!" << endl;
        exit(EXIT_FAILURE);
      }
    } else {
      direction = rand() % 2;
      if (direction == 1) { // Right
        if (player_setup.id == player_setup.num_players - 1) {
          cout << "Sending potato to " << 0 << endl;
        } else {
          cout << "Sending potato to " << player_setup.id + 1 << endl;
        }

        sendStatus = send(connect_fd, &potato, sizeof(potato), 0);
        if (sendStatus == -1) {
          cerr << "Can not send potato ro right!" << endl;
          exit(EXIT_FAILURE);
        }
      } else {
        if (player_setup.id == 0) {
          cout << "Sending potato to " << player_setup.num_players - 1 << endl;
        } else {
          cout << "Sending potato to " << player_setup.id - 1 << endl;
        }

        sendStatus = send(connected_listen_fd, &potato, sizeof(potato), 0);
        if (sendStatus == -1) {
          cerr << "Can not send potato ro left!" << endl;
          exit(EXIT_FAILURE);
        }
      }
    }
  }
  close(to_master_fd);
  close(connected_listen_fd);
  close(connect_fd);
  exit(EXIT_SUCCESS);
}
