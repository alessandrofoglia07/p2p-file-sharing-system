#include "DHT.h"
#include <openssl/sha.h>
#include <string.h>

void hash_file(const char *filename, unsigned char *hash) {
    SHA1((unsigned char *) filename, strlen(filename), hash);
}

void join_network(Peer *new_peer, Peer *bootstrap_peer) {
    // contact existing peer to initialize finger table and discover its position
    // example: RPC to get the successor of the new_peer's ID from the existing_peer
    // Peer successor = find_successor(new_peer->id);
    // initialize_finger_table(new_peer, bootstrap_peer);
    // update_other_peers(new_peer);
}

Peer find_successor(unsigned char *id) {
    // find the successor for the given ID
    // maybe traverse the finger table
}

// find successor peer responsible for given file hash
Peer find_file_peer(const char *filename) {
    unsigned char file_id[20];
    hash_file(filename, file_id);
    return find_successor(file_id);
}

int is_local_peer(Peer peer) {
    // check if the peer is the local peer
}

// add file to local storage
void add_file_to_local_storage(const char *filename) {
    // add file to local storage
}

// send file to peer
void send_file_to_peer(Peer *peer, const char *filename, const char *filepath) {
    // send file to peer
}

// store file in the network
void store_file(const char *filename, const char *filepath) {
    Peer responsible_peer = find_file_peer(filename);
    if (is_local_peer(responsible_peer)) {
        add_file_to_local_storage(filename);
    } else {
        send_file_to_peer(&responsible_peer, filename, filepath);
    }
}
