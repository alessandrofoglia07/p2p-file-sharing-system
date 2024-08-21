#include "sha1.h"
#include <string.h>
#include <openssl/sha.h>

void hash(const char *str, uint8_t hash[20]) {
    SHA1((unsigned char *) str, strlen(str), hash);
}
