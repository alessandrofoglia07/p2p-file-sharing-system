#include "protocol.h"

#include <sha1.h>

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
    MessageNode *new_msg = (MessageNode *) malloc(sizeof(MessageNode));
    new_msg->msg = (Message *) malloc(sizeof(Message)); // Allocate memory for the Message
    memcpy(new_msg->msg, msg, sizeof(Message)); // Copy Message data
    new_msg->next = NULL;

    pthread_mutex_lock(&queue->mutex);
    if (queue->tail == NULL) {
        queue->head = queue->tail = new_msg;
    } else {
        queue->tail->next = new_msg;
        queue->tail = new_msg;
    }
    pthread_cond_broadcast(&queue->cond);
    pthread_mutex_unlock(&queue->mutex);
}

Message *pop_message(MessageQueue *queue) {
    pthread_mutex_lock(&queue->mutex);
    while (queue->head == NULL) {
        pthread_cond_wait(&queue->cond, &queue->mutex);
    }
    MessageNode *msg = queue->head; // Use non-const pointer to modify the queue
    queue->head = msg->next;
    if (queue->head == NULL) {
        queue->tail = NULL;
    }
    pthread_mutex_unlock(&queue->mutex);
    Message *message = msg->msg;
    free(msg); // Free the MessageNode
    return message;
}

Message *create_message(const char *type, const uint8_t id[HASH_SIZE], const char *ip, const int port,
                        const char *data) {
    Message *msg = (Message *) malloc(sizeof(Message));
    strcpy(msg->type, type);
    memcpy(msg->id, id, HASH_SIZE);
    strcpy(msg->ip, ip);
    msg->port = port;
    strncpy(msg->data, data, sizeof(msg->data) - 1);
    msg->data[sizeof(msg->data) - 1] = '\0';

    return msg;
}

int send_message(const Node *sender, const char *receiver_ip, const int receiver_port, Message *msg) {
    struct sockaddr_in receiver_addr = {0};
    receiver_addr.sin_family = AF_INET;
    receiver_addr.sin_port = htons(receiver_port);
    receiver_addr.sin_addr.s_addr = inet_addr(receiver_ip);

    char buffer[MSG_SIZE];

    memcpy(buffer, msg, sizeof(Message)); // Copy entire Message structure

    // Use MSG_CONFIRM if the message type is MSG_REPLY
    const int flags = strcmp(msg->type, MSG_REPLY) == 0 ? MSG_CONFIRM : 0;

    const int result = sendto(sender->sockfd, buffer, sizeof(Message), flags,
                              (struct sockaddr *) &receiver_addr, sizeof(receiver_addr));

    free(msg);

    return result;
}

int receive_message(const Node *n, Message *msg) {
    struct sockaddr_in sender_addr = {0};
    socklen_t sender_addr_len = sizeof(sender_addr);

    char buffer[MSG_SIZE];
    const int bytes_read = recvfrom(n->sockfd, buffer, MSG_SIZE, 0,
                                    (struct sockaddr *) &sender_addr, &sender_addr_len);

    if (bytes_read < 0) {
        perror("recvfrom failed");
        return -1;
    }

    if (bytes_read < sizeof(Message)) {
        fprintf(stderr, "Received data is smaller than expected\n");
        return -1;
    }

    memcpy(msg, buffer, sizeof(Message)); // Copy received data into Message

    return bytes_read;
}

