#include "handle_requests.h"
#include "common.h"
#include "socket.h"
#include <sys/socket.h>
#include <stdio.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

#include "peers.h"

File file_index[MAX_FILES];
Peer peers[MAX_PEERS];
int peer_count = 0;

int main() {
    signal(SIGINT, handle_exit); // gracefully handle ctrl+c
    // initialize server socket
    fd_t sockfd = create_socket(PORT);

    // load known peers from file
    printf("Loading known peers from file...\n");
    load_peers_from_file(PEERS_FILE);
    printf("Peers loaded.\n");

    // contact known peers to discover more peers
    for (int i = 0; i < peer_count; i++) {
        discover_peers(peers[i].ip, peers[i].port);
    }

    // start thread to handle user i/o and outgoing requests
    pthread_t outgoing_requests_thread;
    int pthread_err = pthread_create(&outgoing_requests_thread, NULL, handle_outgoing_requests, &sockfd);
    if (pthread_err) {
        printf("Thread creation failed: %d\n", pthread_err);
        close(sockfd);
        return 1;
    }

    // at the same time, start listening for incoming requests
    while (1) {
        fd_t client_socket = accept(sockfd, NULL, NULL);
        pthread_t client_thread;
        pthread_err = pthread_create(&client_thread, NULL, handle_client, &client_socket);
        if (pthread_err) {
            printf("Thread creation failed: %d\n", pthread_err);
            close(client_socket);
        }
    }
}
