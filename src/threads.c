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
MessageQueue download_queue;

void *node_thread(void *arg) {
    Node *n = (Node *) arg;
    while (1) {
        stabilize(n);
        fix_fingers(n, &(int){0});
        check_predecessor(n);
        sleep(15);
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
        char command[4096];
        fgets(command, sizeof(command), stdin);
        command[strcspn(command, "\n")] = '\0'; // remove trailing newline

        if (strncmp(command, "outdir", 6) == 0) {
            char new_outdir[512];
            sscanf(command, "outdir %511s", new_outdir);
            if (strlen(new_outdir) == 0 || new_outdir[strlen(new_outdir) - 1] != '/') {
                printf("Invalid directory\n");
                continue;
            }
            if (set_outdir(new_outdir) < 0) {
                printf("Failed to set output directory\n");
            } else {
                printf("Output directory set to %s\n", new_outdir);
            }
        } else if (strncmp(command, "store", 5) == 0) {
            char filepath[512];
            sscanf(command, "store %511s", filepath);
            if (strlen(filepath) == 0) {
                printf("Invalid filepath\n");
                continue;
            }
            if (store_file(node, filepath) < 0) {
                printf("Failed to store file\n");
                continue;
            }
            confirm_file_stored(node, filepath);
            printf("File stored successfully\n");
        } else if (strncmp(command, "find", 4) == 0) {
            char filename[256];
            sscanf(command, "find %255s", filename);
            if (strlen(filename) == 0) {
                printf("Invalid filename\n");
                continue;
            }
            FileEntry *file = find_file(node, filename);
            if (file == NULL) {
                printf("File '%s' not found\n", filename);
            } else {
                printf("File '%s' found at %s:%d\n", filename, file->owner_ip, file->owner_port);

                printf("Do you want to download this file? [y/n] ");
                const char c = getchar();
                getchar(); // consume newline

                if (toupper(c) == 'Y') {
                    printf("Starting download...\n");
                    if (download_file(node, file) < 0) {
                        printf("Failed to download file\n");
                    } else {
                        printf("File downloaded successfully\n");
                    }
                } else {
                    printf("Download canceled.\n");
                }
            }
        } else if (strcmp(command, "uploaded") == 0) {
            FileEntry *cur = node->uploaded_files;
            if (cur == NULL) {
                printf("No files uploaded yet.\n");
            } else {
                printf("Files uploaded:\n");
                while (cur != NULL) {
                    printf("  %s\n", cur->filename);
                    cur = cur->next;
                }
            }
        } else if (strncmp(command, "delete", 6) == 0) {
            char filename[256];
            sscanf(command, "delete %255s", filename);
            if (strlen(filename) == 0) {
                printf("Invalid filename\n");
                continue;
            }
            if (delete_file(node, filename) < 0) {
                printf("Failed to delete file\n");
            } else {
                printf("File deleted successfully\n");
            }
        } else if (strcmp(command, "help") == 0 || strcmp(command, "?") == 0) {
            printf("Available commands:\n");
            printf("  outdir <directory> - set the output directory for downloaded files\n");
            printf("  store <filepath> - store a file in the network\n");
            printf("  find <filename> - find a file in the network\n");
            printf("  uploaded - list all files uploaded by the user\n");
            printf("  delete <filename> - delete a file uploaded by the user\n");
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
