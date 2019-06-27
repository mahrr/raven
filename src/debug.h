/*
 * (debug.h | 12 Mar 19 | Ahmad Maher)
 *
 * Debug Utilities
 *
*/

#ifndef debug_h
#define debug_h

#include "ast.h"
#include "token.h"

/* print token type, position, length and lexeme */
void print_token(Token *tok);

/* print (roughly) s-expression representation of an ast */
void print_piece(AST_piece piece);

#endif
