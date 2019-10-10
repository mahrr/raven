/*
 * (token.h | 22 June 19 | Ahmad Maher)
 *
 * Raven Token Representation
 *
*/

#ifndef token_h
#define token_h

#include <stdint.h>  /* int64_t */

typedef enum {
    /* Literals */
    TK_INT, TK_FLOAT, TK_STR,
    TK_RSTR, TK_FALSE, TK_TRUE,
    TK_NIL,

    /* Keywords */
    TK_FN, TK_RETURN, TK_LET, TK_TYPE,
    TK_DO, TK_END, TK_IF, TK_ELIF, TK_ELSE,
    TK_FOR, TK_WHILE, TK_CONTINUE,
    TK_BREAK, TK_COND, TK_MATCH, TK_CASE,

    /* Identefier */
    TK_IDENT,
    
    /* Operators */
    TK_AND, TK_OR, TK_NOT,
    TK_DOT, TK_AT, TK_PIPE,

    /* Arthimetik Operators */
    TK_PLUS, TK_MINUS, TK_ASTERISK,
    TK_SLASH, TK_PERCENT,

    /* Ordering Operators */
    TK_LT, TK_GT, TK_EQ_EQ,
    TK_BANG_EQ, TK_LT_EQ,
    TK_GT_EQ,

    /* Delimiters */
    TK_LPAREN, TK_RPAREN,
    TK_LBRACE, TK_RBRACE,
    TK_LBRACKET, TK_RBRACKET,
    TK_COMMA, TK_DASH_GT,
    TK_COLON, TK_SEMICOLON,
    TK_EQ, TK_IN,
    TK_NL, 

    /* Error and end of file */
    TK_ERR, TK_EOF,
} TK_type;

typedef struct Token {
    TK_type type;        /* the type of the token */
    const char *file;    /* the name of the source file */
    const char *lexeme;  /* pointer to the start of the token in src */
    int length;          /* the length of the token lexeme */
    long line;           /* the line of the token */
} Token;

/* return the int value of TK_INT token. */
int64_t int_of_tok(Token *tok);

/* return the float value of TK_FLOAT token. */
double float_of_tok(Token *tok);

/* 
 * return a duplicate string (you need to free it)
 * value of TK_STR token.
*/
char *str_of_tok(Token *tok);

/* 
 * return a duplicate string (you need to free it)
 * representation of TK_IDENT token.
 */
char *ident_of_tok(Token *tok);

#endif
