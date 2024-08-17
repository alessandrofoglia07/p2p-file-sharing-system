#ifndef PEERS_H
#define PEERS_H
#include <common.h>
#include <stdio.h>

void remove_peer(const Peer peer);

int add_peer(const char ip[16], const int port);

void load_peers_from_stream(FILE *stream);

#endif //PEERS_H
