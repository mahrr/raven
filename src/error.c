/*
 * (error.c | 28 Nov 18 | Ahmad Maher)
 *
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "error.h"
#include "token.h"


char *errors_name[] = {
    "Syntax Error",
    "Name Error",
    "Runtime Error"
};

void fatal_err(int status, const char *msg, ...) {
    va_list ap;
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    va_end(ap);
    exit(status);
}

void log_errs(Err *errors, int n, FILE *out) {
    for (int i = 0; i < n; i++) {
        Token *where = &errors[i].where;
    
        fprintf(out, "[%s | %ld] %s at '%.*s': %s.\n",
                where->file,
                where->line,
                errors_name[errors[i].type],
                where->type == TK_EOF ? 3 : where->length,
                where->type == TK_EOF ? "EOF" : where->lexeme,
                errors[i].message);
    }

    /* 
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
    */
}
