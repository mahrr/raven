/*
 * (strutil.c | 12 Dec 18 | Ahmad Maher)
 *  
*/

#include <ctype.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include "strutil.h"

char *strdup(const char *s) {
    int len = strlen(s) + 1;
    char *dup = malloc(len);
    
    memcpy(dup, s, len);
    return dup;
}

char *strndup(const char *s, size_t n) {
    size_t slen = strlen(s);
    size_t ssize = slen > n ? n : slen;
    char *dup = malloc(ssize + 1);
    
    memcpy(dup, s, ssize);
    dup[ssize] = '\0';
    
    return dup;
}

char *strescp(const char *s, size_t size) {
    int count = 0; /* count for escaped characters */

    /* calculate the number of '/' */
    for (size_t i = 0; i < size; i++)
        if (s[i] == '\\') count++;

    size_t new_size = size + count + 1;
    char *escaped = malloc(new_size);

    size_t j = 0;
    for (size_t i = 0; i < size; i++, j++) {
        if (s[i] == '\\') {
            escaped[j++] = '\\';
            escaped[j] = '\\';
        } else
            escaped[j] = s[i];
    }

    escaped[j] = '\0';
    return escaped;
}

int
strunescp(const char *s, char *unescaped, size_t size, char **end) {
    size_t i, j; /* string counters */
    
    for (i = 0, j = 0; i < size; i++, j++) {
        if (s[i] != '\\' || size == 1) {
            unescaped[j] = s[i];
            continue;
        }
        
        i++; /* consume '\' */
        
        switch (s[i]) {
            
        case 'a': unescaped[j] = '\a'; break;
        case 'b': unescaped[j] = '\b'; break;
        case 'f': unescaped[j] = '\f'; break;
        case 'n': unescaped[j] = '\n'; break;
        case 'r': unescaped[j] = '\r'; break;
        case 't': unescaped[j] = '\t'; break;
        case 'v': unescaped[j] = '\v'; break;
        case '\\': unescaped[j] = '\\'; break;
        case '\'': unescaped[j] = '\''; break;
        case '\"': unescaped[j] = '\"'; break;

            /* \xNN -> two hexadecimal format */
        case 'x': {
            i++; /* consumes 'x' */

            /* terminated before digits*/
            if (i+2 >= size+1) {
                if (end) {
                    /* at the 'x' */
                    *end =(char*)(s+i-1);
                }
                return -1;
            }
                
            /* the next two digits */
            char digits[2];
            digits[0] = s[i++];
            digits[1] = s[i];
                
            char *e;
            int conv = strtol(digits, &e, 16);

            /* invalid two digits */
            if (*e != '\0') {
                if (end) {
                    /* at the 'x' */
                    *end = (char*)(s+i-1);
                }
                return -1;
            }

            unescaped[j] = (char)conv;
            break;
        }
                
        default: {
            /* \NNN -> three ocatal format */
            if (isdigit(s[i])) {
                /* terminated before three digits */
                if (i+3 >= size+1) {
                    if (end)
                        *end = (char*)(s+i);
                    return -1;
                }

                char digits[3];
                digits[0] = s[i++];
                digits[1] = s[i++];
                digits[2] = s[i];

                char *e;
                int conv = strtol(digits, &e, 8);

                /* invalid three digits octal */
                if (*e != '\0') {
                    if (end) {
                        /* at first digit */
                        *end = (char*)(s+i-2);
                    }
                    return -1;
                }

                /* the converted number above ASCII limit */
                if (conv > 255) {
                    if (end) {
                        /* at first digit */
                        *end = (char*)(s+i-2);
                    }
                    return -1;
                }

                unescaped[j] = (char)conv;
                break;
            }
            
            if (end)
                *end = (char*)(s+i);
            return -1;
        }
        }
    }

    if (end)
        *end = NULL;
    unescaped[j] = '\0';
    return j;
}
