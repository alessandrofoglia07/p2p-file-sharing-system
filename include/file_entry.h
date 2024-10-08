#ifndef FILE_ENTRY_H
#define FILE_ENTRY_H

#include <stdint.h>

#define MAX_FILENAME 256
#define MAX_FILEPATH 4096

typedef struct FileEntry {
    uint8_t id[20]; // hash key of the file (used for lookup)
    char filename[MAX_FILENAME];
    char filepath[MAX_FILEPATH];
    char owner_ip[16];
    int owner_port;
    struct FileEntry *next;
} FileEntry;

#include "node.h"

extern char outdir[MAX_FILEPATH];

int set_outdir(const char *new_outdir);

int store_file(Node *n, const char *filepath);

void confirm_file_stored(Node *node, const char *filepath);

int internal_store_file(Node *n, const char *filename, const char *filepath, const uint8_t *file_id,
                        const char *uploader_ip, int uploader_port);

FileEntry *find_file(Node *n, const char *filename);

FileEntry *find_uploaded_file(const Node *n, const char *filepath);

int download_file(const Node *n, const FileEntry *file_entry);

int delete_file_entry(FileEntry **pFiles, const uint8_t *id);

int delete_file(Node *n, const char *filename);

#endif //FILE_ENTRY_H
