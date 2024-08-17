#ifndef COMMON_H
#define COMMON_H

#define MAX_PEERS 10
#define MAX_FILES 100
#define CHUNK_SIZE 4096
#define PORT 8080
#define DEFAULT_BOOTSTRAP_IP "192.168.1.1"
#define BUFFER_SIZE 1024
#define PEERS_FILE "peers.txt"

typedef int fd_t;

typedef struct {
    char filename[256];
} File;

// Peers are saved in file in the format: "IP:PORT\n" (max size of 32 bytes)
typedef struct {
    char ip[16]; // IP address (e.g., "192.168.1.1")
    int port; // port number (e.g., 8080)
} Peer;

extern Peer peers[MAX_PEERS];
extern int peer_count;

#endif //COMMON_H
