#ifndef CHORD_H
#define CHORD_H

#include <stddef.h>
#include <stdint.h>

#define M 160 // number of bits in the hash (SHA-1)
#define MSG_SIZE 1024
#define MSG_JOIN "JOIN"
#define MSG_NOTIFY "NOTIFY"
#define MSG_FIND_SUCCESSOR "FIND_SUCCESSOR"
#define MSG_STABILIZE "STABILIZE"
#define MSG_REPLY "REPLY"
#define MSG_HEARTBEAT "HEARTBEAT"
#define MSG_STORE_FILE "MSG_STORE_FILE"

// simple message protocol
typedef struct {
    char type[16]; // message type (e.g. JOIN, NOTIFY, FIND_SUCCESSOR, STABILIZE, REPLY)
    uint8_t id[20]; // ID involved (e.g. the ID to find a successor for)
    char ip[16]; // IP address of the sender
    int port; // port of the sender
    char data[MSG_SIZE - 36]; // additional data (e.g. filename)
} Message;

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

void create_ring(Node *n);

void join_ring(Node *n, const Node *n_prime);

void stabilize(Node *n);

void notify(const Node *n, const Node *n_prime);

void fix_fingers(Node *n, int *next);

void check_predecessor(Node *n);

Node *find_successor(Node *n, const uint8_t *id);

Node *closest_preceding_node(Node *n, const uint8_t *id);

int send_message(const Node *sender, const Node *receiver, const char *msg, size_t msg_len);

int receive_message(const Node *n, char *buffer, size_t buffer_size);

void store_file(Node *n, const char *filename);

void internal_store_file(Node *n, const char *filename, const uint8_t *file_id, const char *uploader_ip,
                         int uploader_port);

FileEntry *find_file(Node *n, const char *filename);

#endif //CHORD_H
