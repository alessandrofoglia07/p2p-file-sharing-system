#ifndef PEERS_H
#define PEERS_H
#include <common.h>

void remove_peer(Peer peer);

int add_peer(const char ip[16], int port);

void save_peers_to_file(const char *filename);

void load_peers_from_file(const char *filename);

void load_peers_from_string(const char *data);

#endif //PEERS_H
