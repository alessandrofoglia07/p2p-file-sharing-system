#include "sha1.h"
#include <string.h>
#include <openssl/sha.h>

void hash(const char *str, uint8_t hash[HASH_SIZE]) {
    SHA1((unsigned char *) str, strlen(str), hash);
}
