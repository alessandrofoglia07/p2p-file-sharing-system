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
#include <threads.h>

// create new chord ring
void create_ring(Node *n) {
    n->predecessor = NULL;
    n->successor = n;
    printf("New ring created.\n");
    fflush(stdout);
}

// join an existing chord ring
void join_ring(Node *n, const char *existing_ip, const int existing_port) {
    // send a JOIN message to the existing node
    Message *msg = create_message(MSG_JOIN, n->id, n->ip, n->port, "");

    send_message(n, existing_ip, existing_port, msg);

    // the existing node will respond with the new node's successor
    const Message *response = pop_message(&reply_queue);
    if (response == NULL) {
        printf("Failed to join the ring at %s:%d\n", existing_ip, existing_port);
        fflush(stdout);
        exit(EXIT_FAILURE);
    }

    // create a new node for the successor
    Node *successor = create_node(response->ip, response->port);

    // set the new node's successor to the one returned by the existing node
    n->predecessor = NULL;
    n->successor = successor;

    printf("Joined the ring at %s:%d\n", n->successor->ip, n->successor->port);
    fflush(stdout);
}

/*
 * Called periodically. Asks the successor for its predecessor,
 * verifies it to be itself. If not, its verified that the new
 * node lies between itself and successor. Therefore, the new
 * node is set as the new successor. The new node is also notified
 * of its new predecessor.
 */
void stabilize(Node *n) {
    // send a STABILIZE message to the successor
    Message *msg = create_message(MSG_STABILIZE, n->id, n->ip, n->port, "");

    send_message(n, n->successor->ip, n->successor->port, msg);

    // receive the successor's predecessor to verify
    const Message *response = pop_message(&reply_queue);

    // x is the successor's predecessor
    Node *x = create_node(response->ip, response->port);

    // if x is in the interval (n, successor), then update the successor
    if (is_in_interval(x->id, n->id, n->successor->id)) {
        n->successor = x;
    }

    // notify the new successor to update its predecessor
    notify(n->successor, n);
}

void notify(Node *n, Node *n_prime) {
    // send a NOTIFY message to n
    Message *msg = create_message(MSG_NOTIFY, n_prime->id, n_prime->ip, n_prime->port, "");

    // notify that n_prime might be its new predecessor
    send_message(n_prime, n->ip, n->port, msg);

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
    Message *msg = create_message(MSG_HEARTBEAT, n->id, n->ip, n->port, "");

    // if the predecessor does not respond, it has failed -> set it to NULL
    if (send_message(n, n->predecessor->ip, n->predecessor->port, msg) < 0) {
        n->predecessor = NULL;
    }
}

// ask node n to find the successor of id
Node *find_successor(Node *n, const uint8_t *id) {
    if (memcmp(id, n->id, HASH_SIZE) > 0 && memcmp(id, n->successor->id, HASH_SIZE) <= 0) {
        return n->successor;
    }

    const Node *current = closest_preceding_node(n, id);
    return find_successor_remote(n, current, id);
}

Node *find_successor_remote(const Node *n, const Node *n0, const uint8_t *id) {
    // Avoid sending message to itself
    if (n0 == n) {
        return n->successor;
    }

    // send a FIND_SUCCESSOR message to the current node
    Message *msg = create_message(MSG_FIND_SUCCESSOR, id, n->ip, n->port, "");

    send_message(n, n0->ip, n0->port, msg);

    // receive the successor's info
    const Message *response = pop_message(&reply_queue);

    Node *successor = n->successor;
    memcpy(successor->id, response->id, HASH_SIZE);
    strcpy(successor->ip, response->ip);
    successor->port = response->port;

    return successor;
}

// search the local finger table for the highest predecessor of id
Node *closest_preceding_node(Node *n, const uint8_t *id) {
    // search the finger table in reverse order to find the closest node
    for (int i = M - 1; i >= 0; i--) {
        if (is_in_interval(n->finger[i]->id, n->id, id)) {
            return n->finger[i];
        }
    }
    return n;
}
