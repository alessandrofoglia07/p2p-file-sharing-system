#ifndef COMMON_H
#define COMMON_H

#include <netinet/in.h>

#define MAX_PEERS 10
#define MAX_FILES 100
#define CHUNK_SIZE 4096
#define PORT 8080
#define DEFAULT_BOOTSTRAP_IP "192.168.1.1"

typedef int fd_t;

typedef struct {
    char filename[256];
} File;

typedef struct {
    struct sockaddr_in addr;
    fd_t sockfd;
    File files[MAX_FILES];
    int n_files;
} Peer;

extern Peer peers[MAX_PEERS];
extern int peer_count;

#endif //COMMON_H
