#ifndef SOCKET_H
#define SOCKET_H

#include "common.h"

fd_t create_socket(int port);

fd_t connect_to_peer(const char *ip, int port);

int discover_peers(const char *bootstrap_ip, int bootstrap_port);

#endif //SOCKET_H
