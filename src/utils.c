#include "utils.h"

#include <stdio.h>
#include <stdlib.h>

#include "sha1.h"
#include <string.h>
#include <arpa/inet.h>
#include <uuid/uuid.h>

// checks if id is in the interval (a, b)
int is_in_interval(const uint8_t *id, const uint8_t *a, const uint8_t *b) {
    if (memcmp(a, b, HASH_SIZE) < 0) {
        return memcmp(id, a, HASH_SIZE) > 0 && memcmp(id, b, HASH_SIZE) < 0;
    }
    return memcmp(id, a, HASH_SIZE) > 0 || memcmp(id, b, HASH_SIZE) < 0;
}

// generate unique 32 bits id
uint32_t generate_id() {
    uuid_t uuid;
    uuid_generate(uuid);
    return *(uint32_t *) uuid; // return the first 32 bits
}

// checks if file_id falls in the interval (new_node_id, current_node_id]
int should_transfer_file(const uint8_t *file_id, const uint8_t *new_node_id, const uint8_t *current_node_id) {
    return memcmp(file_id, new_node_id, HASH_SIZE) > 0 && memcmp(file_id, current_node_id, HASH_SIZE) <= 0;
}

size_t serialize_file_entries(char **buf, const size_t buf_size, const FileEntry *files, const uint8_t *new_node_id,
                              const uint8_t *current_node_id) {
    size_t offset = 0;
    const FileEntry *entry = files;

    while (entry != NULL && offset < buf_size) {
        if (should_transfer_file(entry->id, new_node_id, current_node_id)) {
            const size_t entry_size = sizeof(FileEntry);
            if (offset + entry_size > buf_size) {
                *buf = realloc(*buf, buf_size * 2);
            }

            memcpy(*buf + offset, entry, entry_size);
            offset += entry_size;
        }
        entry = entry->next;
    }

    return offset;
}

size_t serialize_all_file_entries(char **buf, const size_t buf_size, const FileEntry *files) {
    size_t offset = 0;
    const FileEntry *entry = files;

    while (entry != NULL && offset < buf_size) {
        const size_t entry_size = sizeof(FileEntry);
        if (offset + entry_size > buf_size) {
            *buf = realloc(*buf, buf_size * 2);
            if (*buf == NULL) {
                perror("realloc");
                exit(EXIT_FAILURE);
            }
        }

        memcpy(*buf + offset, entry, entry_size);
        offset += entry_size;

        entry = entry->next;
    }

    return offset;
}

void deserialize_file_entries(Node *n, const char *buf, const size_t buf_size) {
    size_t offset = 0;

    while (offset < buf_size) {
        // allocate memory for new file entry
        FileEntry *entry = (FileEntry *) malloc(sizeof(FileEntry));
        if (entry == NULL) {
            perror("malloc");
            exit(EXIT_FAILURE);
        }

        memcpy(entry, buf + offset, sizeof(FileEntry));
        entry->next = NULL; // make sure the next pointer is NULL

        entry->next = n->files;
        n->files = entry;

        offset += sizeof(FileEntry);
    }
}

void delete_transferred_files(FileEntry **pFiles, const uint8_t *new_node_id, const uint8_t *current_node_id) {
    FileEntry *prev = NULL;
    FileEntry *entry = *pFiles;

    while (entry != NULL) {
        if (should_transfer_file(entry->id, new_node_id, current_node_id)) {
            if (prev == NULL) {
                // removing the head of the file
                *pFiles = entry->next;
            } else {
                prev->next = entry->next;
            }

            FileEntry *to_delete = entry;
            entry = entry->next;
            free(to_delete); // free the memory
        } else {
            prev = entry;
            entry = entry->next;
        }
    }
}
