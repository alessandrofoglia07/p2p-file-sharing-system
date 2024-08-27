#include "node.h"
#include "file_entry.h"
#include "stabilization.h"
#include "sha1.h"
#include "utils.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <threads.h>
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
    return node;
}

void node_bind(Node *n) {
    if ((n->sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        free(n);
        perror("socket");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(n->sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0) {
        perror("setsockopt");
        close(n->sockfd);
        free(n);
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(n->port);
    addr.sin_addr.s_addr = inet_addr(n->ip);

    if (bind(n->sockfd, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        perror("bind");
        close(n->sockfd);
        free(n);
        exit(EXIT_FAILURE);
    }
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
    if (strcmp(msg->type, MSG_FILE_DATA) == 0 || strcmp(msg->type, MSG_FILE_END) == 0) {
        // FILE_DATA message handling (ignore, just push to download queue for other threads)
        push_message(&download_queue, msg);
    } else if (strcmp(msg->type, MSG_REPLY) == 0) {
        // REPLY message handling (ignore, just push to reply queue for other threads)
        push_message(&reply_queue, msg);
    } else if (strcmp(msg->type, MSG_JOIN) == 0) {
        // JOIN request handling
        Node *new_node = create_node(msg->ip, msg->port);

        // find successor of new node
        const Node *successor = find_successor(n, new_node->id);

        // reply with successor info
        Message response;
        response.request_id = msg->request_id;
        strcpy(response.type, MSG_REPLY);
        memcpy(response.id, successor->id, HASH_SIZE);
        strcpy(response.ip, successor->ip);
        response.port = successor->port;
        strcpy(response.data, "");

        send_message(n, msg->ip, msg->port, &response);

        free(new_node);
    } else if (strcmp(msg->type, MSG_FIND_SUCCESSOR) == 0) {
        // FIND_SUCCESSOR request handling
        const Node *successor = find_successor(n, msg->id);

        // reply with successor info
        Message response;
        response.request_id = msg->request_id;
        strcpy(response.type, MSG_REPLY);
        memcpy(response.id, successor->id, HASH_SIZE);
        strcpy(response.ip, successor->ip);
        response.port = successor->port;
        strcpy(response.data, "");

        send_message(n, msg->ip, msg->port, &response);
    } else if (strcmp(msg->type, MSG_STABILIZE) == 0) {
        // STABILIZE request handling
        if (n->predecessor != NULL) {
            Message response;
            response.request_id = msg->request_id;
            strcpy(response.type, MSG_REPLY);
            memcpy(response.id, n->predecessor->id, HASH_SIZE);
            strcpy(response.ip, n->predecessor->ip);
            response.port = n->predecessor->port;
            strcpy(response.data, "");

            // return predecessor info to get verified
            send_message(n, msg->ip, msg->port, &response);
        }
    } else if (strcmp(msg->type, MSG_NOTIFY) == 0) {
        // NOTIFY request handling
        Node *new_predecessor = create_node(msg->ip, msg->port);

        // update predecessor if necessary
        if (n->predecessor == NULL || is_in_interval(new_predecessor->id, n->predecessor->id, n->id)) {
            n->predecessor = new_predecessor;
        } else {
            free(new_predecessor);
        }
    } else if (strcmp(msg->type, MSG_HEARTBEAT) == 0) {
        // HEARTBEAT request handling
        // do nothing, just to keep the connection alive
    } else if (strcmp(msg->type, MSG_STORE_FILE) == 0) {
        // STORE_FILE request handling
        const char *filepath = msg->data;
        const char *filename = strrchr(filepath, '/') + 1;
        internal_store_file(n, filename, filepath, msg->id, msg->ip, msg->port);
    } else if (strcmp(msg->type, MSG_FIND_FILE) == 0) {
        // FIND_FILE request handling
        const FileEntry *file = find_file(n, msg->data);
        if (file) {
            Message response;
            response.request_id = msg->request_id;
            strcpy(response.type, MSG_REPLY);
            memcpy(response.id, file->id, HASH_SIZE);
            strcpy(response.ip, file->owner_ip);
            response.port = file->owner_port;
            strcpy(response.data, file->filepath);

            send_message(n, msg->ip, msg->port, &response);
        } else {
            Message response;
            response.request_id = msg->request_id;
            strcpy(response.type, MSG_REPLY);
            memset(response.id, 0, HASH_SIZE);
            strcpy(response.ip, "");
            response.port = 0;
            strcpy(response.data, "File not found");
            send_message(n, msg->ip, msg->port, &response);
        }
    } else if (strcmp(msg->type, MSG_DOWNLOAD_FILE) == 0) {
        // DOWNLOAD_FILE request handling
        const char *filepath = msg->data;
        // ensure the file was uploaded by this node before sending it
        const FileEntry *file_entry = find_uploaded_file(n, filepath);
        if (file_entry != NULL) {
            FILE *file = fopen(file_entry->filepath, "rb");
            if (file == NULL) {
                perror("Failed to open file");
                return;
            }
            Message response;
            response.request_id = msg->request_id;
            strcpy(response.type, MSG_REPLY);
            memcpy(response.id, file_entry->id, HASH_SIZE);
            strcpy(response.ip, file_entry->owner_ip);
            response.port = file_entry->owner_port;

            strcpy(response.data, "Starting download");
            send_message(n, msg->ip, msg->port, &response);

            strcpy(response.type, MSG_FILE_DATA);

            size_t bytes_read;

            while ((bytes_read = fread(response.data, 1, sizeof(response.data), file)) > 0) {
                response.data_len = bytes_read;
                send_message(n, msg->ip, msg->port, &response);
                usleep(1000);
            }

            strcpy(response.type, MSG_FILE_END);
            strcpy(response.data, "");
            send_message(n, msg->ip, msg->port, &response);

            fclose(file);
        } else {
            Message response;
            response.request_id = msg->request_id;
            strcpy(response.type, MSG_REPLY);
            memset(response.id, 0, HASH_SIZE);
            strcpy(response.ip, "");
            response.port = 0;
            strcpy(response.data, "File not found");
            send_message(n, msg->ip, msg->port, &response);
        }
    } else if (strcmp(msg->type, MSG_DELETE_FILE) == 0) {
        Message response;
        response.request_id = msg->request_id;
        strcpy(response.type, MSG_REPLY);
        memset(response.id, 0, HASH_SIZE);
        strcpy(response.ip, "");
        response.port = 0;

        if (delete_file_entry(&n->files, msg->id) < 0) {
            strcpy(response.data, "File not found");
        } else {
            strcpy(response.data, "File deleted");
        }
        send_message(n, msg->ip, msg->port, &response);
    } else {
        printf("Unknown message type: %s\n", msg->type);
    }
}
