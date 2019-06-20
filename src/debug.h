/*
 * (debug.h | 12 Mar 19 | Ahmad Maher)
 *
 * Debug utilities
 *
*/

#include "lexer.h"
#include "ast.h"

/* print token type, position, length and lexeme */
extern void print_token(Token);

/* print (roughly) s-expression representation of an ast */
extern void print_piece(AST_piece);