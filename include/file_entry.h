#ifndef FILE_ENTRY_H
#define FILE_ENTRY_H

#include <stdint.h>

typedef struct FileEntry {
    uint8_t id[20]; // hash key of the file (used for lookup)
    char filename[256];
    char owner_ip[16];
    int owner_port;
    struct FileEntry *next;
} FileEntry;

#include "node.h"

void store_file(Node *n, const char *filename);

void internal_store_file(Node *n, const char *filename, const uint8_t *file_id, const char *uploader_ip,
                         int uploader_port);

FileEntry *find_file(Node *n, const char *filename);

#endif //FILE_ENTRY_H
