/*
 * (salloc.h | 18 Nov 2018 | Ahmad Maher)
 *
 * Strings allocations.
 *
 * These function unlike string.h functions saves exactly
 * one copy of each distinct string, so two returned strings
 * by these functions can be compared by their address not
 * every individual character. This simplifies the comperasion
 * and saves space.
 *
 * The main function is strn, and the rest just a wrappers.
 * It maintains a set of distinct strings by saving them in a string
 * table. It saves exactly one copy of each distinct string, and
 * it never removes it (for optimization sake, but could be changed).
 * The string table is an array of 1024 hash buckets, and every
 * bucket heads a list of strings that share the same hash value and
 * each entry includes a len field because strings could contain null
 * characters.
 *
*/

#ifndef salloc_h
#define salloc_h

/* size of the string hash table, using a power of two makes
   the hashing faster, as masking is usually faster than module */
#define SHASH_SIZE 1024

typedef struct string {
    char *s;
    int len;
    struct string *link;
} string;

/* makes a copy from a null terminated string s */
extern char *str(const char *s);
/* makes a copy from string s with length len and adds it to the string 
   table unless it's already there and return its address*/ 
extern char *strn(const char *s, unsigned long len);
/* convert n to a decimal representation string and returns it */
extern char *strd(long n);

#endif
