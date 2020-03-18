/*
 * (hashing.h | 11 May 19 | Ahmad Maher)
 *
 * utilities for various object hashing and key comparsion.
 *
*/

#ifndef hashing_h
#define hashing_h

#include <stdint.h>

/* hash a float */
uint64_t hash_float(const void *key);

/* hash an int */
uint64_t hash_int(const void *key);

/* hash a string */
uint64_t hash_str(const void *key);

/* hash a pointer */
uint64_t hash_ptr(const void *key);

/* return 1 if the result of subtracting *key2 from *key1
   is less than a very small number, 0 otherwise */
int comp_float(const void *key1, const void *key2);

/* return 1 if the two int are equal, 0 otherwise */
int comp_int(const void *key1, const void *key2);

/* return 1 if the two strings are equal, 0 otherwise */
int comp_str(const void *key1, const void *key2);

/* return 1 if the two pointers are equal, 0 otherwise */
int comp_ptr(const void *key1, const void *key2);

/* return the next prime number greater than n */
uint32_t next_prime(uint32_t n);

#endif
