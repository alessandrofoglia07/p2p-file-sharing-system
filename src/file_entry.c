#include "file_entry.h"
#include "protocol.h"
#include "stabilization.h"
#include "threads.h"
#include "utils.h"
#include "sha1.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stddef.h>

int store_file(Node *n, const char *filepath) {
    const char *filename = strrchr(filepath, '/');
    if (filename == NULL) {
        filename = (char *) filepath;
    } else {
        filename++;
    }

    uint8_t file_id[HASH_SIZE];
    hash(filename, file_id);

    Node *responsible_node = find_successor(n, file_id);

    if (memcmp(n->id, responsible_node->id, HASH_SIZE) == 0) {
        return internal_store_file(n, filename, filepath, file_id, n->ip, n->port);
    }
    Message *msg = create_message(MSG_STORE_FILE, file_id, n->ip, n->port, filepath);
    const int result = send_message(n, responsible_node->ip, responsible_node->port, msg);
    free(responsible_node);
    return result;
}

int internal_store_file(Node *n, const char *filename, const char *filepath, const uint8_t *file_id,
                        const char *uploader_ip, const int uploader_port) {
    FileEntry *new_entry = (FileEntry *) malloc(sizeof(FileEntry));
    if (new_entry == NULL) {
        perror("malloc");
        return -1;
    }

    memcpy(new_entry->id, file_id, HASH_SIZE);
    strncpy(new_entry->filename, filename, sizeof(new_entry->filename));
    strncpy(new_entry->filepath, filepath, sizeof(new_entry->filepath));
    strcpy(new_entry->owner_ip, uploader_ip);
    new_entry->owner_port = uploader_port;

    new_entry->next = n->files;
    n->files = new_entry;

    return 0;
}

FileEntry *find_file(Node *n, const char *filename) {
    uint8_t file_id[HASH_SIZE];
    hash(filename, file_id);

    Node *responsible_node = find_successor(n, file_id);

    if (memcmp(n->id, responsible_node->id, HASH_SIZE) == 0) {
        FileEntry *current = n->files;
        while (current != NULL) {
            if (memcmp(current->id, file_id, HASH_SIZE) == 0) {
                return current; // file found locally
            }
            current = current->next;
        }
        return NULL; // file not found locally
    }

    Message *msg = create_message(MSG_FIND_FILE, file_id, n->ip, n->port, filename);

    send_message(n, responsible_node->ip, responsible_node->port, msg);

    free(responsible_node);

    const Message *response = pop_message(&reply_queue);

    if (strcmp(response->data, "File not found") == 0) {
        return NULL; // file not found in the network
    }

    FileEntry *file_entry = (FileEntry *) malloc(sizeof(FileEntry));
    if (file_entry == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    memcpy(file_entry->id, response->id, HASH_SIZE);
    strncpy(file_entry->filepath, response->data, sizeof(file_entry->filepath));
    strncpy(file_entry->filename, filename, sizeof(file_entry->filename));
    strcpy(file_entry->owner_ip, response->ip);
    file_entry->owner_port = response->port;

    return file_entry;
}

void download_file(const Node *n, const char ip[16], const int port, const char *filename) {
    Message *msg = create_message(MSG_DOWNLOAD_FILE, n->id, n->ip, n->port, filename);

    send_message(n, ip, port, msg);

    FILE *file = fopen(filename, "wb");
    if (file == NULL) {
        perror("File opening failed");
        exit(EXIT_FAILURE);
    }

    Message *response;

    while ((response = pop_message(&reply_queue))) {
        fwrite(response->data, 1, sizeof(response->data) - offsetof(Message, data), file);
    }

    fclose(file);
}
