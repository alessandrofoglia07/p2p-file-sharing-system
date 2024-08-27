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
    // check if file exists
    if (access(filepath, F_OK) < 0) {
        return -1;
    }

    const char *filename = strrchr(filepath, '/');
    if (filename == NULL) {
        filename = (char *) filepath;
    } else {
        filename++;
    }

    uint8_t file_id[HASH_SIZE];
    hash(filename, file_id);

    const Node *responsible_node = find_successor(n, file_id);

    if (memcmp(n->id, responsible_node->id, HASH_SIZE) == 0) {
        return internal_store_file(n, filename, filepath, file_id, n->ip, n->port);
    }
    Message msg;
    msg.request_id = generate_id();
    strcpy(msg.type, MSG_STORE_FILE);
    memcpy(msg.id, file_id, HASH_SIZE);
    strcpy(msg.ip, n->ip);
    msg.port = n->port;
    strcpy(msg.data, filepath);

    return send_message(n, responsible_node->ip, responsible_node->port, &msg);
}

// run if store function is successful to confirm that the file was actually uploaded
void confirm_file_stored(Node *node, const char *filepath) {
    FileEntry *file_entry = (FileEntry *) malloc(sizeof(FileEntry));
    if (file_entry == NULL) {
        perror("malloc");
        return;
    }

    strcpy(file_entry->filepath, filepath);
    const char *filename = strrchr(filepath, '/');
    if (filename == NULL) {
        filename = (char *) filepath;
    } else {
        filename++;
    }
    hash(filename, file_entry->id);
    strncpy(file_entry->filename, filename, sizeof(file_entry->filename));
    strcpy(file_entry->owner_ip, node->ip);
    file_entry->owner_port = node->port;

    file_entry->next = node->uploaded_files;
    node->uploaded_files = file_entry;
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

    const Node *responsible_node = find_successor(n, file_id);

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

    Message msg;
    msg.request_id = generate_id();
    strcpy(msg.type, MSG_FIND_FILE);
    memcpy(msg.id, file_id, HASH_SIZE);
    strcpy(msg.ip, n->ip);
    msg.port = n->port;
    strcpy(msg.data, filename);

    send_message(n, responsible_node->ip, responsible_node->port, &msg);

    const Message *response = pop_message(&reply_queue, msg.request_id);

    if (response == NULL || strcmp(response->data, "File not found") == 0) {
        return NULL; // file not found in the network or timeout occurred
    }

    FileEntry *file_entry = (FileEntry *) malloc(sizeof(FileEntry));
    if (file_entry == NULL) {
        perror("malloc");
        return NULL;
    }

    memcpy(file_entry->id, response->id, HASH_SIZE);
    strncpy(file_entry->filepath, response->data, sizeof(file_entry->filepath));
    strncpy(file_entry->filename, filename, sizeof(file_entry->filename));
    strcpy(file_entry->owner_ip, response->ip);
    file_entry->owner_port = response->port;

    return file_entry;
}

FileEntry *find_uploaded_file(const Node *n, const char *filepath) {
    FileEntry *current = n->uploaded_files;
    while (current != NULL) {
        if (strcmp(current->filepath, filepath) == 0) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

int download_file(const Node *n, const FileEntry *file_entry) {
    Message msg;
    msg.request_id = generate_id();
    strcpy(msg.type, MSG_DOWNLOAD_FILE);
    memcpy(msg.id, n->id, HASH_SIZE);
    strcpy(msg.ip, n->ip);
    msg.port = n->port;
    strcpy(msg.data, file_entry->filepath);

    if (send_message(n, file_entry->owner_ip, file_entry->owner_port, &msg) < 0) {
        return -1;
    }

    FILE *file = fopen(file_entry->filename, "wb");
    if (file == NULL) {
        perror("File opening failed");
        return -1;
    }

    Message *response;

    while ((response = pop_message(&reply_queue, msg.request_id))) {
        fwrite(response->data, 1, sizeof(response->data) - offsetof(Message, data), file);
    }

    fclose(file);

    return 0;
}

int delete_file(Node *n, const char *filename) {
    const FileEntry *cur = n->uploaded_files;
    if (cur == NULL) {
        return -1;
    }
    int found = 0;
    while (cur != NULL) {
        if (strcmp(filename, cur->filename) == 0) {
            found = 1;
            break;
        }
        cur = cur->next;
    }
    if (!found) {
        return -1;
    }
    const Node *responsible_node = find_successor(n, cur->id);
    Message msg;
    msg.request_id = generate_id();
    strcpy(msg.type, MSG_DELETE_FILE);
    memcpy(msg.id, cur->id, HASH_SIZE);
    strcpy(msg.ip, n->ip);
    msg.port = n->port;

    if (send_message(n, responsible_node->ip, responsible_node->port, &msg) < 0) {
        return -1;
    }

    const Message *response = pop_message(&reply_queue, msg.request_id);
    if (response == NULL || strcmp(response->data, "File not found") == 0) {
        return -1;
    }

    FileEntry *temp = n->uploaded_files;
    FileEntry *prev = NULL;
    // If the head node itself holds the key
    if (temp != NULL && memcmp(temp->id, cur->id, HASH_SIZE) == 0) {
        n->uploaded_files = temp->next; // Change head
        free(temp); // Free old head
        return 0;
    }

    // Search for the key and keep track of the previous node
    while (temp != NULL && memcmp(temp->id, cur->id, HASH_SIZE) != 0) {
        prev = temp;
        temp = temp->next;
    }

    // If key was not present in the list
    if (temp == NULL) {
        return -1;
    }

    // Unlink the node from the linked list
    if (prev) {
        prev->next = temp->next;
    }

    free(temp);

    return 0;
}
