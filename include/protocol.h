#ifndef PROTOCOL_H
#define PROTOCOL_H

#define MSG_SIZE 512 // size of the message buffer
#define MSG_JOIN "JOIN"
#define MSG_NOTIFY "NOTIFY"
#define MSG_FIND_SUCCESSOR "FIND_SUCCESSOR"
#define MSG_STABILIZE "STABILIZE"
#define MSG_HEARTBEAT "HEARTBEAT"
#define MSG_STORE_FILE "STORE_FILE"
#define MSG_REPLY "REPLY"

#include <stdint.h>

// simple message protocol
typedef struct {
    char type[16]; // message type (e.g. JOIN, NOTIFY, FIND_SUCCESSOR, STABILIZE, REPLY)
    uint8_t id[20]; // ID involved (e.g. the ID to find a successor for)
    char ip[16]; // IP address of the sender
    int port; // port of the sender
    char data[MSG_SIZE - sizeof(char[16]) - sizeof(uint8_t[20]) - sizeof(char[16]) - sizeof(int)];
    // additional data (e.g. filename)
} Message;

#include "node.h"

int send_message(const Node *sender, const char *receiver_ip, int receiver_port, const Message *msg);

int receive_message(const Node *n, Message *msg);

#endif //PROTOCOL_H
