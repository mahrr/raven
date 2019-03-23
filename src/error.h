/*
 * (error.h | 27 Nov 18 | Ahmad Maher)
 *
 * Error reporting utilities
 *
*/

#ifndef error_h
#define error_h

#include "lexer.h"

/* reports internal fatal errors */
extern void fatal_error(int status, const char *msg, ...);
/* reports errors occurs at lexing phase */
extern void lex_error(char *src, token *tok);

#endif
