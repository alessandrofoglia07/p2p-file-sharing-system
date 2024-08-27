#include "utils.h"
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
