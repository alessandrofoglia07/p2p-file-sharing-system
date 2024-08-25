#include "protocol.h"
#include "node.h"
#include <arpa/inet.h>
#include <sys/socket.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

void init_queue(MessageQueue *queue) {
    queue->head = queue->tail = NULL;
    pthread_mutex_init(&queue->mutex, NULL);
    pthread_cond_init(&queue->cond, NULL);
}

void push_message(MessageQueue *queue, const Message *msg) {
    Message *new_msg = (Message *) malloc(sizeof(Message));
    memcpy(new_msg, msg, sizeof(Message));
    new_msg->next = NULL;

    pthread_mutex_lock(&queue->mutex);
    if (queue->tail == NULL) {
        queue->head = queue->tail = new_msg;
    } else {
        queue->tail->next = (struct Message *) new_msg;
        queue->tail = new_msg;
    }
    pthread_cond_signal(&queue->cond);
    pthread_mutex_unlock(&queue->mutex);
}

Message *pop_message(MessageQueue *queue) {
    pthread_mutex_lock(&queue->mutex);
    while (queue->head == NULL) {
        pthread_cond_wait(&queue->cond, &queue->mutex);
    }
    Message *msg = queue->head;
    queue->head = (Message *) msg->next;
    if (queue->head == NULL) {
        queue->tail = NULL;
    }
    pthread_mutex_unlock(&queue->mutex);
    return msg;
}

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
