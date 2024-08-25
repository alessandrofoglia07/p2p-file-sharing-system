#ifndef THREADS_H
#define THREADS_H

#include "node.h"

extern MessageQueue reply_queue;

void *node_thread(void *arg);

void *listener_thread(void *arg);

void handle_user_commands(Node *node);

void handle_exit(int sig);

#endif //THREADS_H
