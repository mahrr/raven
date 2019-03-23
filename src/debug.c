/*
 * (debug.c | 12 Mar 19 | Ahmad Maher)
 *
*/

#include <stdio.h>

#include "lexer.h"

char *tok_types_str[] = {
    /* Literals */
    "TK_INT", "TK_FLOAT", "TK_STR",
    "TK_RSTR", "TK_FALSE", "TK_TRUE",
    "TK_NIL",
    
    /* Keywords */
    "TK_FN", "TK_RETURN", "TK_LET", "TK_DO",
    "TK_END", "TK_IF", "TK_ELIF", "TK_ELSE",
    "TK_FOR", "TK_WHILE", "TK_CONTINUE",
    "TK_BREAK", "TK_MATCH", "TK_CASE",
    
    /* Identefier */
    "TK_IDENT",
    
    /* Operators */
    "TK_AND", "TK_OR", "TK_NOT",
    "TK_DOT", "TK_AT", "TK_COL_COL",
    
    /* Arthimetik Operators */
    "TK_PLUS", "TK_MINUS", "TK_ASTERISK",
    "TK_SLASH", "TK_PERCENT",
    
    /* Ordering Operators */
    "TK_LT", "TK_GT", "TK_EQ_EQ",
    "TK_BANG_EQ", "TK_LT_EQ",
    "TK_GT_EQ",
    
    /* Logic Operators */
    "TK_PIPE", "TK_AMPERSAND", "TK_CARET",
    "TK_TILDE", "TK_GT_GT", "TK_LT_LT",
    
    /* Delimiters */
    "TK_LPAREN", "TK_RPAREN",
    "TK_LBRACE", "TK_RBRACE",
    "TK_LBRACKET", "TK_RBRACKET",
    "TK_COMMA", "TK_DASH_GT",
    "TK_COLON", "TK_SEMICOLON",
    "TK_EQ", "TK_IN",
    "TK_NL", 
    
    /* Errors and end of file */
    "TK_ERR", "TK_EOF",
};

void print_token(token *t) {
    printf("[%s @line %ld] :: %.*s (%d) :: %s\n",
           t->file,
           t->line,
           t->length,
           t->lexeme == NULL ? " " : t->lexeme,
           t->length,
           tok_types_str[t->type]);
}
