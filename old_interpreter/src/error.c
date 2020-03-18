/*
 * (error.c | 28 Nov 18 | Ahmad Maher)
 *
*/

#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "error.h"
#include "token.h"

jmp_buf eval_err;

char *errors_name[] = {
    "SyntaxError",
    "EvalError"
};

/*
static void print_line(Token *tok, const char *src) {
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
*/

void fatal_err(int status, const char *msg, ...) {
    va_list ap;
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    va_end(ap);
    exit(status);
}

void log_err(Err *error, FILE *out) {
    Token *where = &(error->where);
        
    fprintf(out, "[%s | %ld] %s",
            where->file,
            where->line,
            errors_name[error->type]);

    if (error->type == SYNTAX_ERR) {
        fprintf(out, " near '%.*s'",
                where->type == TK_EOF ? 3 : where->length,
                where->type == TK_EOF ? "EOF" : where->lexeme);
    }
    
    fprintf(out, ": %s.\n", error->message);
}

void log_errs(Err *errors, int n, FILE *out) {
    for (int i = 0; i < n; i++)
        log_err(errors+i, out);    
}
