#ifndef NODE_H
#define NODE_H

#define M 160 // number of bits in the hash (SHA-1)

#include <stdint.h>

typedef struct Node {
    uint8_t id[20]; // SHA-1 hash
    char ip[16];
    int port;
    struct Node *successor;
    struct Node *predecessor;
    struct Node *finger[M];
    struct FileEntry *files;
    int sockfd;
} Node;

#include "protocol.h"

Node *create_node(const char *ip, int port);

void node_bind(Node *n);

void cleanup_node(Node *n);

void handle_requests(Node *n, const Message *msg);

#endif //NODE_H
