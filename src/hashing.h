/*
 * (hashing.h | 11 May 19 | Ahmad Maher)
 *
 * utilities for various object hashing and key comparsion.
 *
*/

#ifndef hashing_h
#define hashing_h

#include <stdint.h>

/* hash a string */
uint64_t hash_str(const void *key);

/* hash a pointer */
uint64_t hash_ptr(const void *key);

/* return 0 if the two strings are equal, 1 otherwise */
int comp_str(const void *key1, const void *key2);

/* return 0 if the two pointers are equal, 1 otherwise */
int comp_ptr(const void *key1, const void *key2);

/* return the next prime number greater than n */
uint32_t next_prime(uint32_t n);

#endif
