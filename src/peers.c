#include "peers.h"
#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include <string.h>

void remove_peer(const Peer peer) {
    for (int i = 0; i < peer_count; i++) {
        if (strcmp(peers[i].ip, peer.ip) == 0 && peers[i].port == peer.port) {
            for (int j = i; j < peer_count - 1; j++) {
                peers[j] = peers[j + 1];
            }
            peer_count--;
            break;
        }
    }
}

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

void save_peers_to_file(const char *filename) {
    FILE *file = fopen(filename, "w");
    for (int i = 0; i < peer_count; i++) {
        fprintf(file, "%s:%d\n", peers[i].ip, peers[i].port);
    }
    fclose(file);
}

void load_peers_from_file(const char *filename) {
    FILE *file = fopen(filename, "w"); // create file if it doesn't exist
    fclose(file);
    file = fopen(filename, "r"); // open file for reading
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
    fclose(file);
}

void load_peers_from_string(const char *data) {
    char line[32]; // enough to hold "xxx.xxx.xxx.xxx:xxxxx"
    const char *ptr = data;

    while (*ptr) {
        const char *end = strchr(data, '\n');
        if (end == NULL) {
            end = ptr + strlen(ptr); // if no newline, its the last line
        }

        // calculate length of the current line
        size_t len = end - ptr;
        if (len >= sizeof(line)) {
            len = sizeof(line) - 1; // prevent buffer overflow
        }

        // copy current line into buffer
        strncpy(line, ptr, len);
        line[len] = '\0'; // null-terminate the string

        // move ptr to start of next line
        ptr = end;
        if (*ptr == '\n') {
            ptr++; // skip newline
        }

        // parse IP and port
        const char *ip = strtok(line, ":");
        const int port = atoi(strtok(NULL, ":"));

        // MAX_PEERS reached
        if (add_peer(ip, port) < 0) {
            break;
        }
    }
}
