/*
 * (hashing.c | 11 May 19 | Ahmad Maher)
 *
*/

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
