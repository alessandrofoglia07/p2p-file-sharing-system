#ifndef SHA1_H
#define SHA1_H

#define HASH_SIZE 20 // size of the hash in bytes

#include <stdint.h>

void hash(const char *str, uint8_t hash[20]);

#endif //SHA1_H
