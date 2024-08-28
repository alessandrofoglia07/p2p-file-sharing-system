#ifndef UTILS_H
#define UTILS_H

#include <file_entry.h>
#include <stdint.h>

// checks if id is in the interval (a, b)
int is_in_interval(const uint8_t *id, const uint8_t *a, const uint8_t *b);

// generate unique 32 bits id
uint32_t generate_id();

// checks if file_id falls in the interval (new_node_id, current_node_id]
int should_transfer_file(const uint8_t *file_id, const uint8_t *new_node_id, const uint8_t *current_node_id);

size_t serialize_file_entries(char *buf, size_t buf_size, FileEntry *files, const uint8_t *new_node_id,
                              const uint8_t *current_node_id);

void deserialize_file_entries(Node *n, const char *buf, size_t buf_size);

void delete_transferred_files(FileEntry **pFiles, const uint8_t *new_node_id, const uint8_t *current_node_id);

#endif //UTILS_H
