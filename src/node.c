#include "node.h"
#include "file_entry.h"
#include "stabilization.h"
#include "sha1.h"
#include "utils.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

Node *create_node(const char *ip, const int port) {
    Node *node = (Node *) malloc(sizeof(Node));
    if (node == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    char id_str[256];
    snprintf(id_str, sizeof(id_str), "%s:%d", ip, port);
    hash(id_str, node->id);
    strncpy(node->ip, ip, sizeof(node->ip));
    node->port = port;
    node->successor = node;
    node->predecessor = NULL;
    node->files = NULL;
    for (int i = 0; i < M; i++) {
        node->finger[i] = node; // initially, all fingers point to the node itself
    }
    if ((node->sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        free(node);
        perror("socket");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(node->sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0) {
        perror("setsockopt");
        close(node->sockfd);
        free(node);
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &addr.sin_addr);

    if (bind(node->sockfd, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        perror("bind");
        close(node->sockfd);
        free(node);
        exit(EXIT_FAILURE);
    }

    return node;
}

void cleanup_node(Node *n) {
    // free files
    FileEntry *file = n->files;
    while (file) {
        FileEntry *next = file->next;
        free(file);
        file = next;
    }
    close(n->sockfd);
    free(n);
}

void handle_requests(Node *n, const Message *msg) {
    if (strcmp(msg->type, MSG_JOIN) == 0) {
        // JOIN request handling
        Node *new_node = (Node *) malloc(sizeof(Node));
        if (new_node == NULL) {
            perror("malloc");
            exit(EXIT_FAILURE);
        }
        memcpy(new_node->id, msg->id, HASH_SIZE);
        strcpy(new_node->ip, msg->ip);
        new_node->port = msg->port;

        // find successor of new node
        const Node *successor = find_successor(n, new_node->id);

        // reply with successor info
        Message response;
        strcpy(response.type, MSG_REPLY);
        memcpy(response.id, successor->id, HASH_SIZE);
        strcpy(response.ip, successor->ip);
        response.port = successor->port;

        send_message(n, msg->ip, msg->port, &response);
        free(new_node);
    } else if (strcmp(msg->type, MSG_FIND_SUCCESSOR) == 0) {
        // FIND_SUCCESSOR request handling
        const Node *successor = find_successor(n, msg->id);

        // reply with successor info
        Message response;
        strcpy(response.type, MSG_REPLY);
        memcpy(response.id, successor->id, HASH_SIZE);
        strcpy(response.ip, successor->ip);
        response.port = successor->port;

        send_message(n, msg->ip, msg->port, &response);
    } else if (strcmp(msg->type, MSG_NOTIFY) == 0) {
        // NOTIFY request handling
        Node *new_predecessor = (Node *) malloc(sizeof(Node));
        if (new_predecessor == NULL) {
            perror("malloc");
            exit(EXIT_FAILURE);
        }
        memcpy(new_predecessor->id, msg->id, HASH_SIZE);
        strcpy(new_predecessor->ip, msg->ip);
        new_predecessor->port = msg->port;

        if (n->predecessor == NULL || is_in_interval(new_predecessor->id, n->predecessor->id, n->id)) {
            n->predecessor = new_predecessor;
        } else {
            free(new_predecessor);
        }
    } else if (strcmp(msg->type, MSG_STORE_FILE) == 0) {
        // STORE_FILE request handling
        internal_store_file(n, msg->data, msg->id, msg->ip, msg->port);
    } else if (strcmp(msg->type, MSG_HEARTBEAT) == 0) {
        // HEARTBEAT request handling
        Message response;
        strcpy(response.type, MSG_REPLY);
        memcpy(response.id, n->id, HASH_SIZE);
        strcpy(response.ip, n->ip);
        response.port = n->port;

        send_message(n, msg->ip, msg->port, &response);
    } else if (strcmp(msg->type, MSG_STABILIZE) == 0) {
        // STABILIZE request handling
        if (n->predecessor != NULL) {
            Message response;
            strcpy(response.type, MSG_REPLY);
            memcpy(response.id, n->predecessor->id, HASH_SIZE);
            strcpy(response.ip, n->predecessor->ip);
            response.port = n->predecessor->port;

            send_message(n, msg->ip, msg->port, &response);
        }
    }
}
