#include <common.h>
#include <socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>

void remove_file(const char *filename) {
    short found = 0;
    for (int i = 0; i < file_count; i++) {
        if (strcmp(file_index[i].filename, filename) == 0) {
            for (int j = i; j < file_count - 1; j++) {
                file_index[j] = file_index[j + 1];
            }
            file_count--;
            found = 1;
            break;
        }
    }
    if (found) {
        printf("File %s removed successfully from file index.\n", filename);
    } else {
        printf("File %s not found in file index.\n", filename);
    }
}

void upload_file(const char *filepath) {
    if (access(filepath, F_OK) == -1) {
        printf("No file found at location %s.\n", filepath);
        return;
    }
    File file;
    strcpy(file.filepath, filepath);
    strcpy(file.filename, strrchr(filepath, '/') + 1);
    file_index[file_count++] = file;
    printf("File %s uploaded successfully to file index.\n", file.filename);
}

// TODO: maybe optimize this
void search_file(const char *filename) {
    for (int i = 0; i < peer_count; i++) {
        const fd_t sockfd = connect_to_peer(peers[i].ip, peers[i].port);
        if (sockfd < 0) {
            continue;
        }
        const char req[1024];
        sprintf(req, "SEARCH %s\n", filename);
        send(sockfd, req, strlen(req), 0);
        char res[1024];
        recv(sockfd, res, 1024, 0);
        close(sockfd);
        if (strcmp(res, "FOUND") == 0) {
            printf("File %s found at %s:%d\n", filename, peers[i].ip, peers[i].port);
            return;
        }
    }
}

void download_file(const char *filename, const char *peer_ip, const int peer_port) {
}
