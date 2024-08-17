#include "handle_requests.h"
#include "common.h"
#include "socket.h"
#include <sys/socket.h>
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

#include "peers.h"

Peer peers[MAX_PEERS];
int peer_count = 0;

int main() {
    fd_t sockfd = create_socket(PORT);

    printf("Loading peers from file...\n");
    load_peers_from_file(PEERS_FILE);
    printf("Peers loaded.\n");

    pthread_t outgoing_requests_thread;
    int pthread_err = pthread_create(&outgoing_requests_thread, NULL, handle_outgoing_requests, &sockfd);
    if (pthread_err) {
        printf("Thread creation failed: %d\n", pthread_err);
        close(sockfd);
        return 1;
    }
    while (1) {
        int client_socket = accept(sockfd, NULL, NULL);
        pthread_t client_thread;
        pthread_err = pthread_create(&client_thread, NULL, handle_client, &client_socket);
        if (pthread_err) {
            printf("Thread creation failed: %d\n", pthread_err);
            close(client_socket);
        }
    }

    return 0;
}
