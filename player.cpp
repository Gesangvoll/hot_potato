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
}
