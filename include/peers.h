#ifndef PEERS_H
#define PEERS_H

int add_peer(const char ip[], const int port);

void load_peers_from_file(const char *filename);

#endif //PEERS_H
