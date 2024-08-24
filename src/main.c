#include "handle_requests.h"
#include "common.h"
#include "socket.h"
#include <sys/socket.h>
#include <stdio.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include "threads.h"
#include "chord.h"

int main(const int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <IP> <PORT>\n", argv[0]);
        return EXIT_FAILURE;
    }

    signal(SIGINT, handle_exit); // gracefully handle ctrl+c

    const char *ip = argv[1];
    const int port = atoi(argv[2]);

    Node *node = create_node(ip, port);

    if (argc == 4) {
        // join an existing ring
        const char *existing_ip = argv[3];
        const int existing_port = atoi(argv[4]);
        Node *n_prime = create_node(existing_ip, existing_port);
        join_ring(node, n_prime);
        free(n_prime);
    } else {
        // create a new ring
        create_ring(node);
    }


    printf("Node running at %s:%d\n", ip, port);

    pthread_t node_tid, listener_tid;
    // start node thread (stabilize, fix fingers, check predecessor)
    if (pthread_create(&node_tid, NULL, node_thread, node) != 0) {
        perror("pthread_create");
        return EXIT_FAILURE;
    }
    // start listener thread (accept incoming connections)
    if (pthread_create(&listener_tid, NULL, listener_thread, node) != 0) {
        perror("pthread_create");
        return EXIT_FAILURE;
    }

    // handle user commands (blocking)
    handle_user_commands(node);

    printf("Exiting...\n");

    pthread_cancel(listener_tid);
    pthread_cancel(node_tid);
    pthread_join(listener_tid, NULL);
    pthread_join(node_tid, NULL);
    cleanup_node(node);

    return 0;
}
