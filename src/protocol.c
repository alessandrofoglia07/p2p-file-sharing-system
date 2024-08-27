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

Message *pop_message(MessageQueue *queue, uint32_t request_id) {
    pthread_mutex_lock(&queue->mutex);

    MessageNode *prev = NULL;
    MessageNode *msg_node = queue->head;

    // Search for a message with the matching request_id
    while (msg_node != NULL) {
        if (msg_node->msg->request_id == request_id) {
            // If it's the head, move the head pointer
            if (msg_node == queue->head) {
                queue->head = msg_node->next;
                if (queue->head == NULL) {
                    queue->tail = NULL;
                }
            } else if (prev) {
                // Otherwise, unlink it from the list
                prev->next = msg_node->next;
                if (msg_node == queue->tail) {
                    queue->tail = prev;
                }
            }
            pthread_mutex_unlock(&queue->mutex);
            Message *message = msg_node->msg;
            free(msg_node); // Free the MessageNode
            return message;
        }
        prev = msg_node;
        msg_node = msg_node->next;
    }

    // If no matching message is found, wait
    while (1) {
        pthread_cond_wait(&queue->cond, &queue->mutex);
        msg_node = queue->head;
        prev = NULL;

        while (msg_node != NULL) {
            if (msg_node->msg->request_id == request_id) {
                // If it's the head, move the head pointer
                if (msg_node == queue->head) {
                    queue->head = msg_node->next;
                    if (queue->head == NULL) {
                        queue->tail = NULL;
                    }
                } else if (prev) {
                    // Otherwise, unlink it from the list
                    prev->next = msg_node->next;
                    if (msg_node == queue->tail) {
                        queue->tail = prev;
                    }
                }
                pthread_mutex_unlock(&queue->mutex);
                Message *message = msg_node->msg;
                free(msg_node); // Free the MessageNode
                return message;
            }
            prev = msg_node;
            msg_node = msg_node->next;
        }
    }
}


int send_message(const Node *sender, const char *receiver_ip, const int receiver_port, const Message *msg) {
    struct sockaddr_in receiver_addr = {0};
    receiver_addr.sin_family = AF_INET;
    receiver_addr.sin_port = htons(receiver_port);
    receiver_addr.sin_addr.s_addr = inet_addr(receiver_ip);

    char buffer[MSG_SIZE];

    memcpy(buffer, msg, sizeof(Message)); // Copy entire Message structure

    // Use MSG_CONFIRM if the message type is MSG_REPLY
    const int flags = strcmp(msg->type, MSG_REPLY) == 0 ? MSG_CONFIRM : 0;

    return sendto(sender->sockfd, buffer, sizeof(Message), flags,
                  (struct sockaddr *) &receiver_addr, sizeof(receiver_addr));
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

    memcpy(msg, buffer, sizeof(Message)); // Copy received data into Message

    return bytes_read;
}

