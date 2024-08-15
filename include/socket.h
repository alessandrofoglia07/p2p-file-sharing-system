#ifndef SOCKET_H
#define SOCKET_H

#include "common.h"

fd_t create_socket(const int port);

fd_t connect_to_peer(const char *ip, const int port);

void discover_peers(const char *bootstrap_ip, const int bootstrap_port);

#endif //SOCKET_H
