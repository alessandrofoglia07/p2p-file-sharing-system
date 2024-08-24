#include "protocol.h"
#include "node.h"
#include <arpa/inet.h>
#include <sys/socket.h>
#include <string.h>
#include <stdio.h>

int send_message(const Node *sender, const char *receiver_ip, const int receiver_port, const Message *msg) {
    struct sockaddr_in receiver_addr = {0};
    receiver_addr.sin_family = AF_INET;
    receiver_addr.sin_port = htons(receiver_port);

    if (inet_pton(AF_INET, receiver_ip, &receiver_addr.sin_addr) <= 0) {
        perror("inet_pton failed");
        return -1;
    }

    char buffer[MSG_SIZE];
    memcpy(buffer, msg, MSG_SIZE);

    return sendto(sender->sockfd, buffer, MSG_SIZE, 0, (struct sockaddr *) &receiver_addr, sizeof(receiver_addr));
}

int receive_message(const Node *n, Message *msg) {
    struct sockaddr_in sender_addr;
    socklen_t sender_addr_len = sizeof(sender_addr);

    char buffer[MSG_SIZE];
    const int bytes_read = recvfrom(n->sockfd, buffer, MSG_SIZE, 0, (struct sockaddr *) &sender_addr, &sender_addr_len);

    if (bytes_read < 0) {
        perror("recvfrom");
        return -1;
    }

    if (bytes_read > 0) {
        memcpy(msg, buffer, MSG_SIZE);
    }

    return bytes_read;
}
