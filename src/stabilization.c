#include "stabilization.h"
#include "file_entry.h"
#include "protocol.h"
#include "sha1.h"
#include "utils.h"
#include <unistd.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
