#include <handle_requests.h>

#include "socket.h"
#include "common.h"
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

Peer peers[MAX_PEERS];
int peer_count = 0;

int main() {
    fd_t sockfd = create_socket(PORT);

    printf("Enter the IP address of the bootstrap server (default: %s): ", DEFAULT_BOOTSTRAP_IP);
    char bootstrap_ip[16];
    fgets(bootstrap_ip, 16, stdin);
    if (strlen(bootstrap_ip) == 1) {
        // only newline character
        strcpy(bootstrap_ip, DEFAULT_BOOTSTRAP_IP);
    } else {
        bootstrap_ip[strlen(bootstrap_ip) - 1] = '\0';
    }

    discover_peers(bootstrap_ip, PORT);

    pthread_t incoming_requests_thread;
    pthread_t outgoing_requests_thread;

    int pthread_err = pthread_create(&incoming_requests_thread, NULL, handle_incoming_requests, &sockfd);
    if (pthread_err) {
        printf("Thread creation failed: %d\n", pthread_err);
        close(sockfd);
        return 1;
    }
    pthread_err = pthread_create(&outgoing_requests_thread, NULL, handle_outgoing_requests, &sockfd);
    if (pthread_err) {
        printf("Thread creation failed: %d\n", pthread_err);
        close(sockfd);
        return 1;
    }

    return 0;
}
