#ifndef STABILIZATION_H
#define STABILIZATION_H

#include "node.h"

void create_ring(Node *n);

void join_ring(Node *n, const char *existing_ip, int existing_port);

void stabilize(Node *n);

void notify(Node *n, Node *n_prime);

void fix_fingers(Node *n, int *next);

void check_predecessor(Node *n);

Node *find_successor(Node *n, const uint8_t *id);

Node *closest_preceding_node(Node *n, const uint8_t *id);

#endif //STABILIZATION_H
