/*
 *
 * (lerror.c | 28 Nov 18 | Ahmad Maher)
 *
 * See error.h for full description.
*/

/*
 * ::NOTE::
 *
 * I'm keeping the implementation simple for now.
 * It could be improved by printing the source column
 * where the error occur, and point to the start/end
 * of the error. but i will defer that for later.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "lexer.h"

extern void fatal_error(int status, const char *msg, ...) {
    va_list ap;
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    va_end(ap);
    exit(status);
}

extern void lex_error(char *src, token *tok) {
    fprintf(stderr, "[%s:%ld] syntax error: %s\n",
            tok->file, tok->line, tok->err_msg);

    /* get the beginning of the line */
    char *begin = tok->lexeme;
    int count = 0;
    while (*begin != '\n' && begin != src) {
        begin--;
        count++;
    }

    if (*begin == '\n') {
        begin++;
        count--;
    }
    
    count += tok->length;
    
    fprintf(stderr, "\t%.*s\n\t", count, begin);         
    int len = count - tok->length + 1;
    while (--len) putchar(' ');
    puts("^--");
}
