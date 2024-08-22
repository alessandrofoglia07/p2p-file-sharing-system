#include "chord.h"

#include <sha1.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

// TODO: Implement error handling for socket api

// checks if id is in the interval (a, b)
int is_in_interval(const uint8_t *id, const uint8_t *a, const uint8_t *b) {
    return memcmp(id, a, 20) > 0 && memcmp(id, b, 20) < 0;
}

Node *create_node(const char *ip, const int port) {
    Node *node = (Node *) malloc(sizeof(Node));
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
    node->sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &addr.sin_addr);

    bind(node->sockfd, (struct sockaddr *) &addr, sizeof(addr));

    return node;
}

// create new chord ring
void create_ring(Node *n) {
    n->predecessor = NULL;
    n->successor = n;
}

// join an existing chord ring
void join_ring(Node *n, Node *n_prime) {
    // send JOIN message to n_prime
    Message msg;
    strcpy(msg.type, MSG_JOIN);
    memcpy(msg.id, n->id, 20);
    strcpy(msg.ip, n->ip);
    msg.port = n->port;

    send_message(n, n_prime, (char *) &msg);

    // recieve the successor's info
    char buf[MSG_SIZE];
    receive_message(n, buf, MSG_SIZE);

    const Message *response = (Message *) buf;
    Node *successor = create_node(response->ip, response->port);
    memcpy(successor->id, response->id, 20);
    n->successor = successor;
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
    memcpy(msg.id, n->id, 20);
    strcpy(msg.ip, n->ip);
    msg.port = n->port;

    send_message(n, n->successor, (char *) &msg);

    char buf[MSG_SIZE];
    receive_message(n, buf, MSG_SIZE);

    const Message *response = (Message *) buf;
    Node *x = create_node(response->ip, response->port);
    memcpy(x->id, response->id, 20);

    // if x is in the interval (n, successor), then update the successor
    if (is_in_interval(x->id, n->id, n->successor->id)) {
        n->successor = x;
    }

    // notify the successor
    notify(n->successor, n);
}

// n_prime thinks it might be our predecessor
void notify(Node *n, Node *n_prime) {
    // send a NOTIFY message to n
    Message msg;
    strcpy(msg.type, MSG_NOTIFY);
    memcpy(msg.id, n_prime->id, 20);
    strcpy(msg.ip, n_prime->ip);
    msg.port = n_prime->port;

    send_message(n_prime, n, (char *) &msg);
}

/*
 * Called periodically. Refreshes finger table entries.
 * next is the index of the next finger to fix.
 */
void fix_fingers(Node *n, int *next) {
    *next = (*next + 1) % M;
    n->finger[*next] = find_successor(n, n->finger[*next]->id);
}

// called periodically. make sure predecessor is still alive
void check_predecessor(Node *n) {
}

// ask node n to find the successor of id
Node *find_successor(Node *n, const uint8_t *id) {
    if (memcmp(id, n->id, 20) > 0 && memcmp(id, n->successor->id, 20) <= 0) {
        return n->successor;
    }
    // forward the query around the circle
    Node *current = closest_preceding_node(n, id);

    // send a FIND_SUCCESSOR message to the current node
    Message msg;
    strcpy(msg.type, MSG_FIND_SUCCESSOR);
    memcpy(msg.id, id, 20);
    strcpy(msg.ip, n->ip);
    msg.port = n->port;

    send_message(n, current, (char *) &msg);

    // receive the successor's info
    char buf[MSG_SIZE];
    receive_message(n, buf, MSG_SIZE);

    const Message *response = (Message *) buf;
    Node *successor = create_node(response->ip, response->port);
    memcpy(successor->id, response->id, 20);
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

void store_file(Node *n, const char *filename) {
}

FileEntry *find_file(Node *n, const char *filename) {
}
