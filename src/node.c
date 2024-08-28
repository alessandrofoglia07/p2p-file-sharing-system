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

        char *buf = (char *) malloc(4096);
        size_t data_size = serialize_file_entries(buf, sizeof(buf), n->files, new_node->id, n->id);

        // send file entries to new node in chunks
        const size_t chunk_size = sizeof(response.data);
        size_t total_chunks = (data_size + chunk_size - 1) / chunk_size;

        for (size_t i = 0; i < total_chunks; i++) {
            strcpy(response.type, MSG_FILE_DATA);
            response.segment_index = (uint16_t) i;
            response.total_segments = (uint16_t) total_chunks;

            // calculate start and end index of chunk
            size_t start_index = i * chunk_size;
            size_t end_index = start_index + chunk_size;
            if (end_index > data_size) {
                end_index = data_size;
            }

            size_t segment_size = end_index - start_index;
            memcpy(response.data, buf + start_index, segment_size);
            response.data_len = (uint32_t) segment_size;

            // send file entries to new node
            send_message(n, msg->ip, msg->port, &response);
        }

        strcpy(response.type, MSG_FILE_END);
        strcpy(response.data, "");
        response.data_len = 0;

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
            response.segment_index = 0;
            response.total_segments = (strlen(file->filepath) + sizeof(response.data) - 1) / sizeof(response.data);

            size_t bytes_sent = 0;

            for (int i = 0; i < response.total_segments; i++) {
                size_t copy_len = strlen(file->filepath) - bytes_sent > sizeof(response.data)
                                      ? sizeof(response.data)
                                      : strlen(file->filepath) - bytes_sent;
                memcpy(response.data, file->filepath + bytes_sent, copy_len);
                response.data[copy_len] = '\0'; // Ensure null-termination
                response.data_len = copy_len;
                send_message(n, msg->ip, msg->port, &response);
                bytes_sent += response.data_len;
                response.segment_index++;
                usleep(1000);
            }
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
        // Ensure the file was uploaded by this node before sending it
        const FileEntry *file_entry = find_uploaded_file(n, filepath);
        if (file_entry != NULL) {
            FILE *file = fopen(file_entry->filepath, "rb");
            if (file == NULL) {
                perror("Failed to open file");
                Message response;
                response.request_id = msg->request_id;
                strcpy(response.type, MSG_REPLY);
                memset(response.id, 0, HASH_SIZE);
                strcpy(response.ip, file_entry->owner_ip);
                response.port = file_entry->owner_port;
                strcpy(response.data, "Failed to open file");
                send_message(n, msg->ip, msg->port, &response);
                return;
            }

            Message response;
            response.request_id = msg->request_id;
            strcpy(response.type, MSG_REPLY);
            memcpy(response.id, file_entry->id, HASH_SIZE);
            strcpy(response.ip, file_entry->owner_ip);
            response.port = file_entry->owner_port;

            strcpy(response.data, "Starting download");

            size_t file_size;
            fseek(file, 0, SEEK_END);
            file_size = ftell(file);
            rewind(file);

            response.total_segments = (file_size + sizeof(response.data) - 1) / sizeof(response.data);
            send_message(n, msg->ip, msg->port, &response);


            size_t bytes_read;
            size_t data_size = sizeof(response.data);
            int status = 0;

            response.segment_index = 0;
            strcpy(response.type, MSG_FILE_DATA);
            while ((bytes_read = fread(response.data, 1, data_size, file)) > 0) {
                response.data_len = bytes_read;
                if (send_message(n, msg->ip, msg->port, &response) < 0) {
                    status = -1;
                    break;
                }
                response.segment_index++;
                usleep(1000);
            }

            strcpy(response.type, MSG_FILE_END);
            strcpy(response.data, status == 0 ? "Transfer complete" : "Transfer interrupted");
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
