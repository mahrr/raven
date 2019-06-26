/*
 * (error.h | 27 Nov 18 | Ahmad Maher)
 *
 * Error Reporting Utilities
 *
*/

#ifndef error_h
#define error_h

#include <stdio.h>

#include "token.h"

typedef enum {
    SYNTAX_ERR,
    NAME_ERR,
    RUNTIME_ERR,
} Err_Type;

/* Syntax Error */
typedef struct SErr {
    Err_Type type;
    Token where;
    const char *message;
} Err;

/* reports internal fatal error */
void fatal_err(int status, const char *msg, ...);

/* log an array of n errors to the stream 'out' */
void log_errs(Err *errors, int n, FILE *out);

#endif
