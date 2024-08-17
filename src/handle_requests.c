#include "handle_requests.h"
#include "common.h"
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>


void *handle_client(void *arg) {
    const fd_t sockfd = *(fd_t *) arg;

    char buf[BUFFER_SIZE];
    recv(sockfd, buf, BUFFER_SIZE, 0);

    if (strncmp(buf, "GET_PEERS", 9) == 0) {
        // TODO: send peers
    }
    // TODO: handle other requests

    return NULL;
}

void *handle_outgoing_requests(void *arg) {
    fd_t *sockfd = (fd_t *) arg;

    return NULL;
}
