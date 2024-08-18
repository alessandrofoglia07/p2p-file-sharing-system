#include "handle_requests.h"
#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include "peers.h"

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
    const fd_t sockfd = *(fd_t *) arg;

    char input[4096];
    printf("$ ");
    fgets(input, sizeof(input), stdin);

    if (strncmp(input, "ADD_PEER ", 9) == 0) {
    } else if (strncmp(input, "REMOVE_PEER ", 12) == 0) {
    } else if (strcmp(input, "LIST_PEERS") == 0) {
    } else if (strcmp(input, "LIST_FILES") == 0) {
    } else if (strncmp(input, "SEARCH_FILE ", 12) == 0) {
    } else if (strncmp(input, "DOWNLOAD_FILE ", 14) == 0) {
    } else if (strncmp(input, "UPLOAD_FILE ", 12) == 0) {
    } else if (strncmp(input, "CONNECT ", 8) == 0) {
    } else if (strncmp(input, "DISCONNECT ", 11) == 0) {
    } else if (strcmp(input, "EXIT") == 0) {
        close(sockfd);
        handle_exit();
    } else if (strcmp(input, "HELP")) {
        printf(
            "ADD_PEER\nAdd a peer to the network.\nUsage: ADD_PEER 192.168.1.10:8080\nResponse: Acknowledgment or error message.\n\nREMOVE_PEER\nRemove a peer from the network.\nUsage: REMOVE_PEER 192.168.1.10:8080\nResponse: Acknowledgment or error message.\n\nLIST_PEERS\nList all known peers.\nUsage: LIST_PEERS\nResponse: A list of all known peers.\n\nLIST_FILES\nList all files available on the peer.\nUsage: LIST_FILES\nResponse: A list of files available on the peer.\n\nSEARCH_FILE\nSearch for a file in the network.\nUsage: SEARCH_FILE filename\nResponse: A list of peers that have the file.\n\nDOWNLOAD_FILE\nDownload a file from a peer.\nUsage: DOWNLOAD_FILE filename 192.168.1.10:8080\nResponse: Success or error message.\n\nUPLOAD_FILE\nUpload a file to the peer.\nUsage: UPLOAD_FILE filepath\nResponse: Success or error message.\n\nCONNECT\nConnect to a peer.\nUsage: CONNECT 192.168.1.10:8080\nResponse: Acknowledgment or error message.\n\nDISCONNECT\nDisconnect from a peer.\nUsage: DISCONNECT 192.168.1.10:8080\nResponse: Acknowledgment or error message.\n\nEXIT\nSafely exit the network and close the application.\nUsage: EXIT\nResponse: Graceful shutdown message.\n");
    } else {
        printf("Invalid command. Try again or type 'HELP' for a list of commands.\n");
    }

    return NULL;
}

void handle_exit() {
    printf("Exiting...\n");
    save_peers_to_file(PEERS_FILE);
    exit(EXIT_SUCCESS);
}
