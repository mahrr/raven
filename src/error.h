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

/* Syntax Error */
typedef struct SErr {
    Token where;
    const char *message;
} SErr;

/* reports internal fatal error */
void fatal_err(int status, const char *msg, ...);

/* log an array of n syntax errors to the stream 'out' */
void log_serr(SErr *errors, int n, FILE *out);

#endif
