#ifndef COMMON_H
#define COMMON_H

#define MAX_PEERS 1000
#define MAX_FILES 1000
#define PORT 8080
#define BUFFER_SIZE 1024
#define PEERS_FILE "peers.txt"
#define FINGER_TABLE_SIZE 160 // number of entries in the finger table (160 bits for SHA1)

typedef int fd_t;

typedef struct {
    char filename[256];
    char filepath[512];
} File;

// Peers are saved in file in the format: "IP:PORT\n" (max size of 32 bytes)
typedef struct {
    char ip[16]; // IP address (e.g., "192.168.1.1")
    int port; // port number (e.g., 8080)
    unsigned char id[20]; // SHA1 hash of IP:PORT
} Peer;

typedef struct {
    Peer finger[FINGER_TABLE_SIZE];
} FingerTable;

extern File file_index[MAX_FILES];
extern int file_count;
extern Peer peers[MAX_PEERS];
extern int peer_count;
extern FingerTable finger_table;

#endif //COMMON_H
