/*
 * (lexer.h | 22 Nov 18 | Ahmad Maher)
 *
 * Lexcical Analyzer Interface
 * 
*/

#ifndef lexer_h
#define lexer_h

#include <stdio.h>

#include "alloc.h"
#include "list.h"

typedef enum {
    /* Literals */
    TK_INT, TK_FLOAT, TK_STR,
    TK_RSTR, TK_FALSE, TK_TRUE,
    TK_NIL,

    /* Keywords */
    TK_FN, TK_RETURN, TK_LET, TK_DO,
    TK_END, TK_IF, TK_ELIF, TK_ELSE,
    TK_FOR, TK_WHILE, TK_CONTINUE,
    TK_BREAK, TK_MATCH, TK_CASE,

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
    const char *lexeme;  /* pointer to the start of the token in the src */
    int length;          /* the length of the token */
    const char *file;    /* the name of the source file */
    long line;           /* the line of the token */
    const char *err_msg; /* the associated message if TK_ERR consumed */
} *Token;

typedef struct Lexer *Lexer;

/* extract the file content to a newly allocated
   buffer and return pointer to that buffer.*/
extern char *scan_file(const char *file);

/* allocate and initialize a new lexer state. */
extern Lexer lexer_new(const char *src, const char *file, Region_N reg);

/* initialize a lexer state. */
extern void init_lexer(Lexer l, const char *src, const char *file);

/* return a consumed token on demand including
   error tokens and add this token to the Lexer
   token lists. */
extern Token cons_token(Lexer);

/* return a list of consumed valid tokens. */
extern List cons_tokens(Lexer);

/* check if there is a lexing error. */
extern int lexer_been_error(Lexer);

/* return a list of the lexer tokens error and NULL
   if there is no errors. */
extern List lexer_errors(Lexer);

#endif
