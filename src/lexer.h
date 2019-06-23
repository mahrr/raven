/*
 * (lexer.h | 22 Nov 18 | Ahmad Maher)
 *
 * Lexer Interface
 * 
*/

#ifndef lexer_h
#define lexer_h

#include <stdio.h>

#include "array.h"
#include "error.h"
#include "token.h"

typedef struct Lexer {
    const char *current;  /* the current unconsumed char in the source */
    const char *fixed;    /* the start of the current token */
    const char *file;     /* the source file name */
    long line;            /* the current line number */
    int been_error;       /* lexing error flag */
    ARRAY(Token) tokens;  /* array of the consumed tokens */
    ARRAY(SErr) errors;   /* array of the lexing (syntax) errors */
} Lexer;

/* 
 * extract the file content to a newly allocated
 * buffer and return pointer to that buffer.
*/
char *scan_file(const char *file);

/* initialize a lexer state. */
void init_lexer(Lexer *lexer, const char *src, const char *file);

/* 
 * free the lexer internal resources (tokens and errors
 * arrays). the lexer cannot be used after call to this
 * function, init_lexer is needed to be called first.
*/
void free_lexer(Lexer *lexer);

/* 
 * consume a token on demand add the token to the lexer
 * token array and return pointer to the consumed token. 
*/
Token *cons_token(Lexer *lexer);

/*
 * consume tokens until EOF is encountered. the consumed
 * tokens are added to the lexer token array, and any
 * lexing (syntax) errors are added to the lexer errors
 * array. it returns a pointer to the tokens array.
*/
Token *cons_tokens(Lexer *lexer);


/* number of consumed tokens */
#define lexer_toknum(lexer) ((lexer)->tokens.len)

/* 1 if there is a lexing error, 0 otherwise. */
#define lexer_error(lexer) ((lexer)->been_error)

/* a pointer to the array of the lexer (SErr) errors. */
#define lexer_errors(lexer) ((lexer)->errors.elems)

/* number of lexing errors. */
#define lexer_errnum(lexer) ((lexer)->errors.len)

#endif
