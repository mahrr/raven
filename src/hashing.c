/*
 * (hashing.c | 11 May 19 | Ahmad Maher)
 *
*/

#include <assert.h>
#include <stdint.h>
#include <stdint.h>  /* int64_t */
#include <string.h>
#include <math.h>    /* fabs */

#include "hashing.h"


uint64_t hash_float(const void *key) {
    double f = *(double*)key;
    uint64_t ui;
    memcpy(&ui, &f, sizeof (uint64_t));
    return ui & 0xfffffffffffff000;
}

uint64_t hash_int(const void *key) {
    uint64_t x = *(uint64_t*)key;
    x = (x ^ (x >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
    x = (x ^ (x >> 27)) * UINT64_C(0x94d049bb133111eb);
    x = x ^ (x >> 31);
    return x;
}

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

int comp_float(const void *key1, const void *key2) {
    return fabs(*(double*)key1) - (*(double*)key2) < 0.000000001;
}

int comp_int(const void *key1, const void *key2) {
    return (*(int64_t*)key1 == *(int64_t*)key2);
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
static int is_prime(int n) {
    if (n == 2 || n == 3 ) return 1;
    if (n % 2 == 0 || n % 3 == 0) return 0;

    int x = 2, y = 5;
    while ((x * x) <= n) {
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
    if (n < 3)
        return 3;

    int i = n + 1;
    for ( ; i < 2 * (int)n; i++)
        if (is_prime(i)) break;
    
    return i;
}
