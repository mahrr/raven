/*
 * (error.h | 27 Nov 18 | Ahmad Maher)
 *
 * Error Reporting Utilities
 *
*/

#ifndef error_h
#define error_h

#include <setjmp.h>
#include <stdio.h>

#include "token.h"

typedef enum {
    SYNTAX_ERR,
    RUNTIME_ERR
} Err_Type;

typedef struct Err {
    Err_Type type;
    Token where;
    const char *message;
} Err;

/* error buffer for error recovery */
extern jmp_buf eval_err;

/* reports an internal fatal error */
void fatal_err(int status, const char *msg, ...);

/* log an error to the stream 'out' */
void log_err(Err *errors, FILE *out);

/* log an array of n errors to the stream 'out' */
void log_errs(Err *errors, int n, FILE *out);

#endif
