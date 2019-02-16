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
    cerr << "Wrong Format!\n" << endl;
    exit(EXIT_FAILURE);
  }

  int port_num = atoi(argv[1]);
  if (port_num <= 1024 || port_num >= 65535) {
    cerr << "Unavailable Port Number!\n" << endl;
    exit(EXIT_FAILURE);
  }

  int num_players = atoi(argv[2]);
  if (num_players <= 1) {
    cerr << "Unavailable Number of Players!\n" << endl;
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
}
