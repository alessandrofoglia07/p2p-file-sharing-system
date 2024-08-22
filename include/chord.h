#ifndef CHORD_H
#define CHORD_H

#include <stdint.h>

#define M 160 // number of bits in the hash (SHA-1)

typedef struct Node {
    uint8_t id[20]; // SHA-1 hash
    char ip[16];
    int port;
    struct Node *successor;
    struct Node *predecessor;
    struct Node *finger[M];
    struct FileEntry *files;
} Node;

typedef struct FileEntry {
    uint8_t id[20]; // hash key of the file (used for lookup)
    char filename[256];
    char owner_ip[16];
    int owner_port;
    struct FileEntry *next;
} FileEntry;

// checks if id is in the interval (a, b)
int is_in_interval(const uint8_t *id, const uint8_t *a, const uint8_t *b);

Node *create_node(const char *ip, int port);

void create_ring(Node *n)

void join_ring(Node *n, Node *n_prime);

void stabilize(Node *n);

void notify(Node *n, Node *n_prime);

void fix_fingers(Node *n, int *next);

void check_predecessor(Node *n);

Node *find_successor(Node *n, const uint8_t *id);

Node *closest_preceding_node(Node *n, const uint8_t *id);

void store_file(Node *n, const char *filename);

FileEntry *find_file(Node *n, const char *filename);

#endif //CHORD_H
