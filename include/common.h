#ifndef COMMON_H
#define COMMON_H

#define MAX_PEERS 1000
#define MAX_FILES 1000
#define PORT 8080
#define BUFFER_SIZE 1024
#define PEERS_FILE "peers.txt"

typedef int fd_t;

typedef struct {
    char filename[256];
    char filepath[512];
} File;

// Peers are saved in file in the format: "IP:PORT\n" (max size of 32 bytes)
typedef struct {
    char ip[16]; // IP address (e.g., "192.168.1.1")
    int port; // port number (e.g., 8080)
} Peer;

extern File file_index[MAX_FILES];
extern int file_count;
extern Peer peers[MAX_PEERS];
extern int peer_count;

#endif //COMMON_H
