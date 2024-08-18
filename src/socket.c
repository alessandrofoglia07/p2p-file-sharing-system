#include "socket.h"
#include "common.h"
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
        printf("Invalid address or address not supported. Address: %s:%d\n", ip, port);
        return -1;
    }

    // connect to peer
    if (connect(sockfd, (struct sockaddr *) &peer_addr, sizeof(peer_addr)) < 0) {
        return -1;
    }

    return sockfd;
}

int discover_peers(const char *bootstrap_ip, const int bootstrap_port) {
    const fd_t sockfd = connect_to_peer(bootstrap_ip, bootstrap_port);
    if (sockfd < 0) {
        return -1;
    }

    const char *req = "GET_PEERS";
    send(sockfd, req, sizeof(req), 0);

    char buf[BUFFER_SIZE];
    const int bytes_received = recv(sockfd, buf, BUFFER_SIZE, 0);
    if (bytes_received < 0) {
        perror("recv");
        close(sockfd);
        return -1;
    }

    load_peers_from_string(buf);

    return 0;
}
