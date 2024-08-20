#ifndef DHT_H
#define DHT_H
#include <common.h>

void hash_file(const char *filename, unsigned char *hash);

void join_network(Peer *new_peer, Peer *bootstrap_peer);

Peer find_successor(unsigned char *id);

Peer find_file_peer(const char *filename);

int is_local_peer(Peer peer);

void add_file_to_local_storage(const char *filename);

void send_file_to_peer(Peer *peer, const char *filename, const char *filepath);

void store_file(const char *filename, const char *filepath);

#endif //DHT_H
