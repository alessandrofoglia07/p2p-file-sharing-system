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
#include <dirent.h>
#include <stdbool.h>
#include <stddef.h>

char outdir[MAX_FILEPATH] = "./";

int set_outdir(const char *new_outdir) {
    // check if directory exists
    DIR *dir = opendir(new_outdir);
    if (strlen(new_outdir) > sizeof(outdir) || dir == NULL) {
        return -1;
    }
    closedir(dir);
    strcpy(outdir, new_outdir);
    return 0;
}

int store_file(Node *n, const char *filepath) {
    // check if file exists
    if (access(filepath, F_OK) < 0) {
        return -1;
    }

    // get filename
    const char *filename = strrchr(filepath, '/');
    if (filename == NULL) {
        filename = (char *) filepath;
    } else {
        filename++;
    }

    uint8_t file_id[HASH_SIZE];
    hash(filename, file_id);

    const Node *responsible_node = find_successor(n, file_id);

    // if the responsible node is the current node, store the file locally
    if (memcmp(n->id, responsible_node->id, HASH_SIZE) == 0) {
        return internal_store_file(n, filename, filepath, file_id, n->ip, n->port);
    }

    // send a message to the responsible node to store the file
    Message msg;
    msg.request_id = generate_id();
    strcpy(msg.type, MSG_STORE_FILE);
    memcpy(msg.id, file_id, HASH_SIZE);
    strcpy(msg.ip, n->ip);
    msg.port = n->port;
    strcpy(msg.data, filepath);

    return send_message(n, responsible_node->ip, responsible_node->port, &msg);
}

// run if store function is successful to locally confirm that the file was actually uploaded
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

    // add file to uploaded files list
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

    // add file to files list
    new_entry->next = n->files;
    n->files = new_entry;

    return 0;
}

FileEntry *find_file(Node *n, const char *filename) {
    uint8_t file_id[HASH_SIZE];
    hash(filename, file_id);

    const Node *responsible_node = find_successor(n, file_id);

    // if the responsible node is the current node, search for the file locally
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

    // send a message to the responsible node to find the file
    Message msg;
    msg.request_id = generate_id();
    strcpy(msg.type, MSG_FIND_FILE);
    memcpy(msg.id, file_id, HASH_SIZE);
    strcpy(msg.ip, n->ip);
    msg.port = n->port;
    strcpy(msg.data, filename);

    send_message(n, responsible_node->ip, responsible_node->port, &msg);

    FileEntry *file_entry = (FileEntry *) malloc(sizeof(FileEntry));
    if (file_entry == NULL) {
        perror("malloc");
        return NULL;
    }

    const Message *response = pop_message(&reply_queue, msg.request_id);

    if (response == NULL || strcmp(response->data, "File not found") == 0) {
        free(file_entry);
        return NULL; // file not found in the network or timeout occurred
    }

    memcpy(file_entry->id, response->id, HASH_SIZE);
    strncpy(file_entry->filename, filename, sizeof(file_entry->filename) - 1);
    file_entry->filename[sizeof(file_entry->filename) - 1] = '\0';
    strcpy(file_entry->owner_ip, response->ip);
    file_entry->owner_port = response->port;
    strncpy(file_entry->filepath, response->data, response->data_len);
    file_entry->filepath[response->data_len] = '\0';

    if (response->total_segments > 1) {
        for (int i = 1; i < response->total_segments; i++) {
            response = pop_message(&reply_queue, msg.request_id);
            if (response == NULL) {
                // Handle error, possibly free memory or return an error code
                free(file_entry);
                return NULL;
            }
            strncat(file_entry->filepath, response->data, response->data_len);
        }
    }

    return file_entry;
}

FileEntry *find_uploaded_file(const Node *n, const char *filepath) {
    FileEntry *current = n->uploaded_files;
    // search for the file in the uploaded files list
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

    // create the output file
    char outpath[strlen(outdir) + strlen(file_entry->filename) + 1];
    strcpy(outpath, outdir);
    strncat(outpath, file_entry->filename, MAX_FILEPATH);

    FILE *file = fopen(outpath, "wb");
    if (file == NULL) {
        perror("File opening failed");
        return -1;
    }

    const Message *response = pop_message(&reply_queue, msg.request_id);
    if (response == NULL) {
        fclose(file);
        return -1;
    }

    if (strcmp(response->data, "Starting download") != 0) {
        fclose(file);
        return -1;
    }

    bool *received_segments = calloc(response->total_segments, sizeof(bool));
    if (received_segments == NULL) {
        perror("calloc");
        fclose(file);
        return -1;
    }

    size_t total_received = 0;
    int retries = 0;
    const int max_retries = 10;

    while (total_received < response->total_segments + 1 && retries < max_retries) {
        response = pop_message(&download_queue, msg.request_id);
        if (response == NULL) {
            retries++;
            usleep(1000);
            continue;
        }
        retries = 0; // reset retries after successful receive

        if (received_segments[response->segment_index]) {
            continue;
        }

        if (strcmp(response->type, MSG_FILE_END) == 0) {
            break;
        }

        fseek(file, response->segment_index * MSG_SIZE, SEEK_SET);
        fwrite(response->data, 1, response->data_len, file);
        received_segments[response->segment_index] = true;
        total_received++;
    }

    free(received_segments);
    fclose(file);

    if (retries == max_retries) {
        fprintf(stderr, "Timeout waiting for segments.\n");
        return -1;
    }

    if (response && total_received != response->total_segments || strcmp(response->data, "Transfer complete") != 0) {
        fprintf(stderr, "Download incomplete. Missing segments.\n");
        return -1;
    }

    return 0;
}

int delete_file_entry(FileEntry **pFiles, const uint8_t *id) {
    FileEntry *temp = *pFiles;
    FileEntry *prev = NULL;
    // If the head node itself holds the key
    if (temp != NULL && memcmp(temp->id, id, HASH_SIZE) == 0) {
        *pFiles = temp->next;
        free(temp);
        return 0;
    }
    // Search for the key and keep track of the previous node
    while (temp != NULL && memcmp(temp->id, id, HASH_SIZE) != 0) {
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

int delete_file(Node *n, const char *filename) {
    const FileEntry *cur = n->uploaded_files;
    if (cur == NULL) {
        return -1;
    }
    int found = 0;
    // search for the file in the uploaded files list
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
    // find the responsible node for the file
    const Node *responsible_node = find_successor(n, cur->id);

    // if the responsible node is the current node, delete the file locally
    if (memcmp(n->id, responsible_node->id, HASH_SIZE) == 0) {
        const int result1 = delete_file_entry(&n->files, cur->id);
        const int result2 = delete_file_entry(&n->uploaded_files, cur->id);
        return result1 == 0 && result2 == 0 ? 0 : -1;
    }

    // send a message to the responsible node to delete the file
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

    // delete the file from the uploaded files list
    return delete_file_entry(&n->uploaded_files, cur->id);
}
