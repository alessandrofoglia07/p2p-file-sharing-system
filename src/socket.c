#include "socket.h"
#include "common.h"
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "peers.h"

fd_t create_socket(const int port) {
    fd_t sockfd;
    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(port),
        .sin_addr.s_addr = INADDR_ANY
    };

    // create socket for incoming connections
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // bind socket to port
    if (bind(sockfd, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        perror("bind");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // start listening for incoming connections
    if (listen(sockfd, MAX_PEERS) < 0) {
        perror("listen");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    return sockfd;
}

fd_t connect_to_peer(const char *ip, const int port) {
    fd_t sockfd;
    struct sockaddr_in peer_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(port)
    };

    // create socket for outgoing connections
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // convert IP address to binary form
    if (inet_pton(AF_INET, ip, &peer_addr.sin_addr) <= 0) {
        printf("Invalid address or address not supported.\n");
        return -1;
    }

    // connect to peer
    if (connect(sockfd, (struct sockaddr *) &peer_addr, sizeof(peer_addr)) < 0) {
        printf("Connection failed.\n");
        return -1;
    }

    return sockfd;
}

/**
 * @return 0 on success, -1 on failure
 */
int discover_peers(const char *bootstrap_ip, const int bootstrap_port) {
    const fd_t sockfd = connect_to_peer(bootstrap_ip, bootstrap_port);

    if (sockfd < 0) {
        return -1;
    }

    char buf[BUFFER_SIZE];

    const char *msg = "GET_PEERS";
    send(sockfd, msg, sizeof(msg), 0);

    const int bytes_received = recv(sockfd, buf, BUFFER_SIZE, 0);
    if (bytes_received < 0) {
        perror("recv");
        close(sockfd);
        return -1;
    }

    load_peers_from_stream(buf);

    return 0;
}
