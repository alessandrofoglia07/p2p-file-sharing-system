#include "peers.h"
#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include <string.h>

/**
 * @return 0 if peer was added successfully, -1 if the peer list is full
 */
int add_peer(const char ip[16], const int port) {
    if (peer_count < MAX_PEERS) {
        strncpy(peers[peer_count].ip, ip, sizeof(peers[peer_count].ip));
        peers[peer_count].port = port;
        peer_count++;
        return 0;
    }
    return -1;
}

void load_peers_from_file(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        printf("File not found: %s\n", filename);
        exit(EXIT_FAILURE);
    }

    char line[32]; // enough to hold "xxx.xxx.xxx.xxx:xxxxx"
    while (fgets(line, sizeof(line), file)) {
        const char *ip = strtok(line, ":");
        const int port = atoi(strtok(NULL, ":"));
        // MAX_PEERS reached
        if (add_peer(ip, port) < 0) {
            break;
        }
    }
}
