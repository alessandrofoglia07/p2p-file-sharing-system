#include "chord.h"

#include <sha1.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    return node;
}

// create new chord ring
void create_ring(Node *n) {
    n->predecessor = NULL;
    n->successor = n;
}

// join an existing chord ring
void join_ring(Node *n, Node *n_prime) {
    n->predecessor = NULL;
    n->successor = find_successor(n_prime, n->id);
}

/*
 * Called periodically. n asks the successor about its predecessor,
 * verifies if n's immediate successor is consistent, and tells the
 * successor about n.
 */
void stabilize(Node *n) {
    Node *x = n->successor->predecessor;
    if (is_in_interval(x->id, n->id, n->successor->id)) {
        n->successor = x;
    }
    notify(n->successor, n);
}

// n_prime thinks it might be our predecessor
void notify(Node *n, Node *n_prime) {
    if (n->predecessor == NULL || is_in_interval(n_prime->id, n->predecessor->id, n->id)) {
        n->predecessor = n_prime;
    }
}

/*
 * Called periodically. Refreshes finger table entries.
 * next is the index of the next finger to fix.
 */
void fix_fingers(Node *n, int *next) {
    *next++;
    if (*next > M) {
        *next = 0;
    }
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
    return find_successor(current, id);
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
