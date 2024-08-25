#include "threads.h"
#include "node.h"
#include "stabilization.h"
#include "file_entry.h"
#include <ctype.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>

MessageQueue reply_queue;

void *node_thread(void *arg) {
    Node *n = (Node *) arg;
    while (1) {
        stabilize(n);
        fix_fingers(n, &(int){0});
        check_predecessor(n);
        sleep(5);
    }
}

void *listener_thread(void *arg) {
    Node *node = (Node *) arg;
    Message msg;

    while (1) {
        if (receive_message(node, &msg) > 0) {
            handle_requests(node, &msg);
        }
    }
}

// main thread (blocking)
void handle_user_commands(Node *node) {
    while (1) {
        char command[256];
        fgets(command, sizeof(command), stdin);
        command[strcspn(command, "\n")] = '\0'; // remove trailing newline

        if (strncmp(command, "store", 5) == 0) {
            char filepath[512];
            sscanf(command, "store %s", filepath);
            store_file(node, filepath);
        } else if (strncmp(command, "find", 4) == 0) {
            char filename[256];
            sscanf(command, "find %s", filename);
            FileEntry *file = find_file(node, filename);
            if (file) {
                printf("File '%s' found at %s:%d\n", filename, file->owner_ip, file->owner_port);
            } else {
                printf("File '%s' not found\n", filename);
            }
        } else if (strncmp(command, "download", 8) == 0) {
            char ip[16];
            int port;
            char filename[256];
            sscanf(command, "download %s:%d %s", ip, &port, filename);
            download_file(node, ip, port, filename);
        } else if (strcmp(command, "help") == 0 || strcmp(command, "?") == 0) {
            printf("Available commands:\n");
            printf("  store <filepath> - store a file in the network\n");
            printf("  find <filename> - find a file in the network\n");
            printf("  download <ip:port> <filename> - download a file from the network\n");
            printf("  help/? - show this help message\n");
            printf("  exit - exit the program\n");
        } else if (strcmp(command, "exit") == 0) {
            printf("Do you really want to quit? [y/n] ");
            const char c = getchar();
            if (toupper(c) == 'Y') {
                break;
            }
            getchar();
        } else {
            printf("Invalid command.\n");
        }
    }
}

void handle_exit(const int sig) {
    signal(sig, SIG_IGN);
    printf("\nDo you really want to quit? [y/n] ");
    const char c = getchar();
    if (toupper(c) == 'Y') {
        printf("Exiting...\n");
        exit(0);
    }
    signal(SIGINT, handle_exit);
    getchar();
}
