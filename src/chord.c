#include "chord.h"

#include <sha1.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

// checks if id is in the interval (a, b)
int is_in_interval(const uint8_t *id, const uint8_t *a, const uint8_t *b) {
    if (memcmp(a, b, HASH_SIZE) < 0) {
        return memcmp(id, a, HASH_SIZE) > 0 && memcmp(id, b, HASH_SIZE) < 0;
    }
    return memcmp(id, a, HASH_SIZE) > 0 || memcmp(id, b, HASH_SIZE) < 0;
}

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

// create new chord ring
void create_ring(Node *n) {
    n->predecessor = NULL;
    n->successor = n;
}

// join an existing chord ring
void join_ring(const Node *n, const Node *n_prime) {
    // send JOIN message to n_prime
    Message msg;
    strcpy(msg.type, MSG_JOIN);
    memcpy(msg.id, n->id, HASH_SIZE);
    strcpy(msg.ip, n->ip);
    msg.port = n->port;

    send_message(n, n_prime->ip, n_prime->port, &msg);

    // recieve the successor's info
    Message response;
    if (receive_message(n, &response) < 0) {
        printf("Failed to join the ring at %s:%d\n", n_prime->ip, n_prime->port);
        exit(EXIT_FAILURE);
    }

    memcpy(n->successor->id, response.id, HASH_SIZE);
    strcpy(n->successor->ip, response.ip);
    n->successor->port = response.port;
}

/*
 * Called periodically. n asks the successor about its predecessor,
 * verifies if n's immediate successor is consistent, and tells the
 * successor about n.
 */
void stabilize(Node *n) {
    // send a STABILIZE message to the successor
    Message msg;
    strcpy(msg.type, MSG_STABILIZE);
    memcpy(msg.id, n->id, HASH_SIZE);
    strcpy(msg.ip, n->ip);
    msg.port = n->port;

    send_message(n, n->successor->ip, n->successor->port, &msg);

    Message response;
    receive_message(n, &response);

    Node x;
    memcpy(x.id, response.id, HASH_SIZE);
    strcpy(x.ip, response.ip);
    x.port = response.port;

    // if x is in the interval (n, successor), then update the successor
    if (is_in_interval(x.id, n->id, n->successor->id)) {
        memcpy(n->successor->id, x.id, HASH_SIZE);
        strcpy(n->successor->ip, x.ip);
        n->successor->port = x.port;
    }

    // notify the successor
    notify(n->successor, n);
}

void notify(Node *n, Node *n_prime) {
    // send a NOTIFY message to n
    Message msg;
    strcpy(msg.type, MSG_NOTIFY);
    memcpy(msg.id, n_prime->id, HASH_SIZE);
    strcpy(msg.ip, n_prime->ip);
    msg.port = n_prime->port;

    send_message(n_prime, n->ip, n->port, &msg);

    if (n->predecessor == NULL || is_in_interval(n->id, n->predecessor->id, n->id)) {
        n->predecessor = n_prime;
    }
}

/*
 * Called periodically. Refreshes finger table entries.
 * next is the index of the next finger to fix.
 */
void fix_fingers(Node *n, int *next) {
    *next = (*next + 1) % M;
    Node *finger = find_successor(n, n->finger[*next]->id);
    n->finger[*next] = finger;
}

// called periodically. make sure predecessor is still alive
void check_predecessor(Node *n) {
    if (n->predecessor == NULL) {
        return;
    }

    // send a heartbeat message to the predecessor
    Message msg;
    strcpy(msg.type, MSG_HEARTBEAT);
    memcpy(msg.id, n->id, HASH_SIZE);
    strcpy(msg.ip, n->ip);
    msg.port = n->port;

    if (send_message(n, n->predecessor->ip, n->predecessor->port, &msg) < 0) {
        n->predecessor = NULL;
    }
}

// ask node n to find the successor of id
Node *find_successor(Node *n, const uint8_t *id) {
    if (memcmp(id, n->id, HASH_SIZE) > 0 && memcmp(id, n->successor->id, HASH_SIZE) <= 0) {
        return n->successor;
    }
    // forward the query around the circle
    const Node *current = closest_preceding_node(n, id);

    // send a FIND_SUCCESSOR message to the current node
    Message msg;
    strcpy(msg.type, MSG_FIND_SUCCESSOR);
    memcpy(msg.id, id, HASH_SIZE);
    strcpy(msg.ip, n->ip);
    msg.port = n->port;

    send_message(n, current->ip, current->port, &msg);

    // receive the successor's info
    Message response;
    receive_message(n, &response);

    Node *successor = n->successor;
    memcpy(successor->id, response.id, HASH_SIZE);
    strcpy(successor->ip, response.ip);
    successor->port = response.port;

    return successor;
}

// search the local finger table for the highest predecessor of id
Node *closest_preceding_node(Node *n, const uint8_t *id) {
    for (int i = M - 1; i >= 0; i--) {
        if (is_in_interval(n->finger[i]->id, n->id, id)) {
            return n->finger[i];
        }
    }
    return n;
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

void store_file(Node *n, const char *filename) {
    uint8_t file_id[HASH_SIZE];
    hash(filename, file_id);

    Node *responsible_node = find_successor(n, file_id);

    if (memcmp(n->id, responsible_node->id, HASH_SIZE) == 0) {
        internal_store_file(n, filename, file_id, n->ip, n->port);
        printf("File '%s' stored on node %s:%d\n", filename, n->ip, n->port);
    } else {
        Message msg;
        strcpy(msg.type, MSG_STORE_FILE);
        memcpy(msg.id, file_id, HASH_SIZE);
        strncpy(msg.ip, n->ip, sizeof(msg.ip));
        msg.port = n->port;
        strncpy(msg.data, filename, sizeof(msg.data) - 1);
        msg.data[sizeof(msg.data) - 1] = '\0';

        send_message(n, responsible_node->ip, responsible_node->port, &msg);

        printf("File '%s' forwarded to node %s:%d for storage\n", filename, responsible_node->ip,
               responsible_node->port);
    }
}

void internal_store_file(Node *n, const char *filename, const uint8_t *file_id, const char *uploader_ip,
                         const int uploader_port) {
    FileEntry *new_entry = (FileEntry *) malloc(sizeof(FileEntry));
    if (new_entry == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    memcpy(new_entry->id, file_id, HASH_SIZE);
    strncpy(new_entry->filename, filename, sizeof(new_entry->filename));
    strcpy(new_entry->owner_ip, uploader_ip);
    new_entry->owner_port = uploader_port;

    new_entry->next = n->files;
    n->files = new_entry;
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

FileEntry *find_file(Node *n, const char *filename) {
    uint8_t file_id[HASH_SIZE];
    hash(filename, file_id);

    Node *responsible_node = find_successor(n, file_id);

    if (memcmp(n->id, responsible_node->id, HASH_SIZE) == 0) {
        FileEntry *current = n->files;
        while (current != NULL) {
            if (memcmp(current->id, file_id, HASH_SIZE) == 0) {
                return current; // file found locally
            }
            current = current->next;
        }
        return NULL; // file not found locally
    }

    Message msg;
    strcpy(msg.type, MSG_FIND_SUCCESSOR);
    memcpy(msg.id, file_id, HASH_SIZE);
    strncpy(msg.ip, n->ip, sizeof(msg.ip));
    msg.port = n->port;

    send_message(n, responsible_node->ip, responsible_node->port, &msg);
    strncpy(msg.data, filename, sizeof(msg.data) - 1);
    msg.data[sizeof(msg.data) - 1] = '\0';

    send_message(n, responsible_node->ip, responsible_node->port, &msg);

    free(responsible_node);

    Message response;
    receive_message(n, &response);

    FileEntry *file_entry = (FileEntry *) malloc(sizeof(FileEntry));
    if (file_entry == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    memcpy(file_entry->id, response.id, HASH_SIZE);
    strncpy(file_entry->filename, response.data, sizeof(file_entry->filename));
    strcpy(file_entry->owner_ip, response.ip);
    file_entry->owner_port = response.port;

    return file_entry;
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
