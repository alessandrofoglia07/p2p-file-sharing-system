#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>

// checks if id is in the interval (a, b)
int is_in_interval(const uint8_t *id, const uint8_t *a, const uint8_t *b);

// generate unique 32 bits id
uint32_t generate_id();

#endif //UTILS_H
