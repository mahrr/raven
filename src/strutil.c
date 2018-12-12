/*
 * (strutil.c | 12 Dec 18 | Ahmad Maher)
 *  
*/

#include <stdlib.h>
#include <ctype.h>
#include <limits.h>

#include "strutil.h"
#include "alloc.h"
#include "salloc.h"

/*
 * ::Note::
 *
 * escape function escapes string only if it has no escape
 * errors, if the string has any error, it won't escape any part 
 * it just returns the unescaped string until the error.
 * This could be improved by escaping until the error, but 
 * i don't think it worth the effort atleast for now.
 *
 * escapes returns on a success or on an escape error, a better
 * approach could be that it collects all the the escape errors
 * and return them all. that why the errors flag has only one bit,
 * but it will complicate the implementation, as i need some
 * structure to keep track of the number of errors and their
 * positions. For now we remain with the simpler approach.
 *
*/

extern int escape(char *unescaped, char *escaped, int size) {
    int state = 0;    
    
    int i, j; /* string counters */
    for (i = 0, j = 0; i < size; i++, j++) {
        if (unescaped[i] != '\\')
            escaped[j] = unescaped[i];
        else {
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

                /* terminated before two digits*/
                if (i+2 >= size+1) {
                    state = HEX_MISS;
                    break;
                }
                
                /* the next two digits */
                char digits[2];
                digits[0] = unescaped[i++];
                digits[1] = unescaped[i];
                
                char **e = make(e, R_SECN);
                int conv; 
                conv = strtol(digits, e, 16);

                /* invalid two digits */
                if (*e == digits || *e == digits+1) {
                    state = HEX_INVL;
                    break;
                }

                escaped[j] = (char)conv;
                break;
            }
                
            default: {
                /* \NNN -> three ocatal format */
                if (isdigit(unescaped[i])) {
                    /* terminated before three digits */
                    if (i+3 >= size+1) {
                        state = OCT_MISS;
                        break;
                    }

                    char digits[3];
                    digits[0] = unescaped[i++];
                    digits[1] = unescaped[i++];
                    digits[2] = unescaped[i];

                    char **e = make(e, R_SECN);
                    int conv;
                    conv = strtol(digits, e, 8);

                    /* invalid three digits octal */
                    if (*e == digits ||
                        *e == digits+1 ||
                        *e == digits+2) {
                        state = OCT_INVL;
                        break;
                    }

                    /* the converted number above ASCII limit */
                    if (conv > 255) {
                        state = OCT_OFRG;
                        break;
                    }

                    escaped[j] = (char)conv;
                    break;
                }
                state = INV_ESCP;             
            }    
            }
        }
        
        if (state) {
            escaped[++j] = '\0'; 
            return state;
        }
    }
    escaped[++j] = '\0';
    return state;
}

char *strd(long n) {
    char buf[25];
    /*from the buffer end */
    char *s = buf + sizeof (buf);
    /* works with unsigned because C permits different machines
       to works with signed modulus on negative values differently */
    unsigned long m;

    if (n == LONG_MIN)
        m = (unsigned)LONG_MAX + 1;
    else
        m = (n < 0) ? -n : n;

    do
        *--s = m%10 + '0';
    while ((m/10) != 0);

    if (n < 0) *--s = '-';

    return strn(s, buf + sizeof (buf) - s);
}
