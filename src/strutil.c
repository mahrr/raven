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

int
strescp(const char *unescaped, char *escaped, size_t size, char **end) {
    unsigned i, j; /* string counters */
    
    for (i = 0, j = 0; i < size; i++, j++) {
        if (unescaped[i] != '\\') {
            escaped[j] = unescaped[i];
            continue;
        }
        
        i++; /* consume '\' */
        
        switch (unescaped[i]) {
            
        case 'a': escaped[j] = '\a'; break;
        case 'b': escaped[j] = '\b'; break;
        case 'f': escaped[j] = '\f'; break;
        case 'n': escaped[j] = '\n'; break;
        case 'r': escaped[j] = '\r'; break;
        case 't': escaped[j] = '\t'; break;
        case 'v': escaped[j] = '\v'; break;
        case '\\': escaped[j] = '\\'; break;
        case '\'': escaped[j] = '\''; break;
        case '\"': escaped[j] = '\"'; break;

            /* \xNN -> two hexadecimal format */
        case 'x': {
            i++; /* consumes 'x' */

            /* terminated before digits*/
            if (i+2 >= size+1) {
                if (end) {
                    /* at the 'x' */
                    *end =(char*)(unescaped+i-1);
                }
                return -1;
            }
                
            /* the next two digits */
            char digits[2];
            digits[0] = unescaped[i++];
            digits[1] = unescaped[i];
                
            char *e;
            int conv = strtol(digits, &e, 16);

            /* invalid two digits */
            if (*e != '\0') {
                if (end) {
                    /* at the 'x' */
                    *end = (char*)(unescaped+i-1);
                }
                return -1;
            }

            escaped[j] = (char)conv;
            break;
        }
                
        default: {
            /* \NNN -> three ocatal format */
            if (isdigit(unescaped[i])) {
                /* terminated before three digits */
                if (i+3 >= size+1) {
                    if (end)
                        *end = (char*)(unescaped+i);
                    return -1;
                }

                char digits[3];
                digits[0] = unescaped[i++];
                digits[1] = unescaped[i++];
                digits[2] = unescaped[i];

                char *e;
                int conv = strtol(digits, &e, 8);

                /* invalid three digits octal */
                if (*e != '\0') {
                    if (end) {
                        /* at first digit */
                        *end = (char*)(unescaped+i-2);
                    }
                    return -1;
                }

                /* the converted number above ASCII limit */
                if (conv > 255) {
                    if (end) {
                        /* at first digit */
                        *end = (char*)(unescaped+i-2);
                    }
                    return -1;
                }

                escaped[j] = (char)conv;
                break;
            }
            
            if (end)
                *end = (char*)(unescaped+i);
            return -1;
        }
        }
    }

    if (end)
        *end = NULL;
    escaped[j] = '\0';
    return j;
}
