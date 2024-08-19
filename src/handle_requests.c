#include "handle_requests.h"
#include "files.h"
#include "socket.h"
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

void print_missing_arguments() {
    printf("Missing or invalid arguments. Type 'HELP' for a list of commands.\n");
}

void *handle_outgoing_requests(void *arg) {
    const fd_t sockfd = *(fd_t *) arg;

    while (1) {
        char input[4096];
        printf("$ ");
        fgets(input, sizeof(input), stdin);

        if (strncmp(input, "ADD_PEER ", 9) == 0) {
            char *addr = strchr(input, ' ') + 1; // skip the space
            if (addr == NULL || *addr == '\0') {
                print_missing_arguments();
                return NULL;
            }
            const char *ip = strtok(addr, ":");
            const char *port = strtok(NULL, ":");
            if (ip == NULL || port == NULL || strlen(ip) > 15 || strlen(port) > 5) {
                print_missing_arguments();
                return NULL;
            }
            if (add_peer(ip, atoi(port)) < 0) {
                printf("Max number of peers reached. Type REMOVE_PEER to remove an existent peer.\n");
                return NULL;
            }
            printf("Peer added successfully.\n");
        } else if (strncmp(input, "REMOVE_PEER ", 12) == 0) {
            char *addr = strchr(input, ' ') + 1; // skip the space
            if (addr == NULL || *addr == '\0') {
                print_missing_arguments();
                return NULL;
            }
            const char *ip = strtok(addr, ":");
            const char *port = strtok(NULL, ":");
            if (ip == NULL || port == NULL || strlen(ip) > 15 || strlen(port) > 5) {
                print_missing_arguments();
                return NULL;
            }
            short peer_found = 0;
            for (int i = 0; i < peer_count; i++) {
                if (strcmp(peers[i].ip, ip) && peers[i].port == atoi(port)) {
                    peer_found = 1;
                    break;
                }
            }
            if (peer_found == 0) {
                printf("Peer not found. Try again or type 'LIST_PEERS' to display a list of peers.\n");
                return NULL;
            }
            remove_peer(ip, atoi(port));
            printf("Peer removed successfully.\n");
        } else if (strcmp(input, "LIST_PEERS") == 0) {
            if (peer_count == 0) {
                printf("No known peers. Type 'ADD_PEER' to add a peer.\n");
                return NULL;
            }
            printf("Known peers:\n");
            for (int i = 0; i < peer_count; i++) {
                printf("- %s:%d\n", peers[i].ip, peers[i].port);
            }
        } else if (strcmp(input, "LIST_FILES") == 0) {
            if (file_count == 0) {
                printf("No files available. Type 'UPLOAD_FILE' to upload a file to the file index.\n");
                return NULL;
            }
            printf("Available files:\n");
            for (int i = 0; i < file_count; i++) {
                printf("- File %s\n  Filepath: %s\n", file_index[i].filename, file_index[i].filepath);
            }
        } else if (strncmp(input, "UPLOAD_FILE ", 12) == 0) {
            const char *filepath = strchr(input, ' ') + 1; // skip the space
            if (filepath == NULL || *filepath == '\0') {
                print_missing_arguments();
                return NULL;
            }
            upload_file(filepath);
        } else if (strncmp(input, "REMOVE_FILE ", 12) == 0) {
            const char *filename = strchr(input, ' ') + 1; // skip the space
            if (filename == NULL || *filename == '\0') {
                print_missing_arguments();
                return NULL;
            }
            remove_file(filename);
        } else if (strncmp(input, "SEARCH_FILE ", 12) == 0) {
            const char *filename = strchr(input, ' ') + 1; // skip the space
            if (filename == NULL || *filename == '\0') {
                print_missing_arguments();
                return NULL;
            }
            search_file(filename);
        } else if (strncmp(input, "DOWNLOAD_FILE ", 14) == 0) {
            char *params = strchr(input, ' ') + 1; // skip the space
            if (params == NULL || *params == '\0') {
                print_missing_arguments();
                return NULL;
            }
            const char *filename = strtok(params, " ");
            const char *ip = strtok(NULL, ":");
            const char *port = strtok(NULL, ":");
            download_file(filename, ip, atoi(port));
        } else if (strncmp(input, "CONNECT ", 8) == 0) {
            char *addr = strchr(input, ' ') + 1; // skip the space
            if (addr == NULL || *addr == '\0') {
                print_missing_arguments();
                return NULL;
            }
            const char *ip = strtok(addr, ":");
            const char *port = strtok(NULL, ":");
            if (ip == NULL || port == NULL || strlen(ip) > 15 || strlen(port) > 5) {
                print_missing_arguments();
                return NULL;
            }
            start_peer_connection(ip, atoi(port));
        } else if (strcmp(input, "EXIT") == 0) {
            close(sockfd);
            handle_exit();
        } else if (strcmp(input, "HELP") == 0) {
            printf(
                "ADD_PEER\nAdd a peer to the network.\nUsage: ADD_PEER 192.168.1.10:8080\nResponse: Acknowledgment or error message.\n\nREMOVE_PEER\nRemove a peer from the network.\nUsage: REMOVE_PEER 192.168.1.10:8080\nResponse: Acknowledgment or error message.\n\nLIST_PEERS\nList all known peers.\nUsage: LIST_PEERS\nResponse: A list of all known peers.\n\nLIST_FILES\nList all files available on the peer.\nUsage: LIST_FILES\nResponse: A list of files available on the peer.\n\nUPLOAD_FILE\nUpload a file to the peer.\nUsage: UPLOAD_FILE filepath\nResponse: Success or error message.\n\nSEARCH_FILE\nSearch for a file in the network.\nUsage: SEARCH_FILE filename\nResponse: A list of peers that have the file.\n\nDOWNLOAD_FILE\nDownload a file from a peer.\nUsage: DOWNLOAD_FILE filename 192.168.1.10:8080\nResponse: Success or error message.\n\nCONNECT\nConnect to a peer.\nUsage: CONNECT 192.168.1.10:8080\nResponse: Acknowledgment or error message.\n\nDISCONNECT\nDisconnect from a peer.\nUsage: DISCONNECT 192.168.1.10:8080\nResponse: Acknowledgment or error message.\n\nEXIT\nSafely exit the network and close the application.\nUsage: EXIT\nResponse: Graceful shutdown message.\n");
        } else {
            printf("Invalid command. Try again or type 'HELP' for a list of commands.\n");
        }
    }
}

void handle_exit() {
    printf("Exiting...\n");
    save_peers_to_file(PEERS_FILE);
    exit(EXIT_SUCCESS);
}
