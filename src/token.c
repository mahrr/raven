/*
 * (token.c | 22 June 19 | Ahmad Maher)
 *
*/

#include <assert.h>
#include <stdlib.h>  /* strtoll */
#include <stdint.h>  /* int64_t */

#include "strutil.h" /* strndup */
#include "token.h"

int64_t int_of_tok(Token *tok) {
    assert(tok->type == TK_INT);
    
    char *endptr;  /* for strtoll function */
    int64_t i;
    
    switch (tok->lexeme[1]) {
    case 'b':
    case 'B':
        i = strtoll(tok->lexeme + 2, &endptr, 2);
        break;
    case 'o':
    case 'O':
        i = strtoll(tok->lexeme + 2, &endptr, 8);
        break;
    default:
        i = strtoll(tok->lexeme, &endptr, 0);
        break;
    }

    assert(endptr == (tok->lexeme + tok->length));
    return i;
}

long double float_of_tok(Token *tok) {
    assert(tok->type == TK_FLOAT);
    
    char *endptr;  /* for strtold function */
    long double f = strtold(tok->lexeme, &endptr);
    
    assert(endptr == (tok->lexeme + tok->length));
    return f;
}

char *str_of_tok(Token *tok) {
    assert(tok->type == TK_STR);
    
    return strndup(tok->lexeme + 1, tok->length - 2);
}

/* return a string representation of TK_IDENT token */
char *ident_of_tok(Token *tok) {
    assert(tok->type == TK_IDENT);

    return strndup(tok->lexeme, tok->length);
}