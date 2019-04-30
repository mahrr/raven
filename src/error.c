/*
 * (error.c | 28 Nov 18 | Ahmad Maher)
 *
 * See error.h for full description.
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

extern void lex_error(char *src, Token tok) {
    fprintf(stderr, "[%s:%ld] syntax error: %s\n",
            tok->file, tok->line, tok->err_msg);

    /* get the beginning of the line */
    const char *begin = tok->lexeme;
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
