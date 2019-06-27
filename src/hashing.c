/*
 * (hashing.c | 11 May 19 | Ahmad Maher)
 *
*/

#include <assert.h>
#include <stdint.h>
#include <string.h>

#include "hashing.h"

/*
 * The function was adapted from the dragon book
 * 'Compilers: Principles, Techniques, and Tools'
 * (MA: Addison-Wesley), 
 * by Alfred V. Aho, Ravi Sethi, and Jeffrey D. Ullman
*/
uint64_t hash_str(const void *key) {
    uint64_t val = 0;
    const char *ptr = key;
    
    while (*ptr != '\0') {
        uint64_t tmp;
        val = (val << 4) + *ptr;
        if ((tmp = val & 0xf0000000)) {
            val ^= (tmp >> 24);
            val ^= tmp;
        }
        ptr++;
    }

    return val;
}

/*
 * Thomas Wang hash function, 
 * from 'Integer Hash Function' article.
*/
uint64_t hash_ptr(const void *key) {
    uint64_t val = (uint64_t)key;
    
    val = ~val + (val << 21);
    val = val ^ (val >> 24);
    val = (val + (val << 3)) + (val << 8); /* ==> val * 265 */
    val = val ^ (val >> 14);
    val = (val + (val << 2)) + (val << 4); /* ==> val * 21 */
    val = val ^ (val >> 28);
    val = val + (val << 31);
    return val;
}

int comp_str(const void *key1, const void *key2) {
    return !strcmp((char*)key1, (char*)key2);
}

int comp_ptr(const void *key1, const void *key2) {
    return (key1 == key2);
}


/* 
 * simple prime number test. 
 * tested on chen, circular and cuban primes.
*/
static int is_prime(uint32_t n) {
    if (n == 2 || n == 3 ) return 1;
    if (n % 2 == 0 || n % 3 == 0) return 0;

    int x = 2, y = 5;
    while ((x * x) < n) {
        if (n % x == 0) return 0;
        x += y;
        y = 6 - y;
    }

    return 1;
}

/*
 * depend on Bertrand's postulate 
 * (en.wikipedia.org/wiki/Bertrand's_postulate).
*/
uint32_t next_prime(uint32_t n) {
    if (n <= 3)
        return 3;
    
    for (int i = n; i < 2 * n; i++)
        if (is_prime(i)) return i;
    
    assert(0);
}
