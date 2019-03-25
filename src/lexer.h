/*
 * (lexer.h | 22 Nov 18 | Ahmad Maher)
 *
 * lexcical analyzer
*/

#ifndef lexer_h
#define lexer_h

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
    TK_DOT, TK_AT, TK_COL_COL,

    /* Arthimetik Operators */
    TK_PLUS, TK_MINUS, TK_ASTERISK,
    TK_SLASH, TK_PERCENT,

    /* Ordering Operators */
    TK_LT, TK_GT, TK_EQ_EQ,
    TK_BANG_EQ, TK_LT_EQ,
    TK_GT_EQ,

    /* Logic Operators */
    TK_PIPE, TK_AMPERSAND, TK_CARET,
    TK_TILDE, TK_GT_GT, TK_LT_LT,

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
} token_type;

typedef struct {
    token_type type;  /* the type of the token */
    char *lexeme;     /* pointer to the start of the token in the src */
    int length;       /* the length of the token */
    char *file;       /* the name of the source file */
    long line;        /* the line of the token */
    char *err_msg;    /* the associated message if TK_ERR consumed */
} token;

typedef struct {
    List_T tokens;
    int been_error;
    List_T error_tokens;
} token_list;

typedef struct {
    char *current;    /* the current unconsumed char in the source */
    char *fixed;      /* the start of the current token */
    char *file_name;  /* the source file name */
    long line;        /* the current line number */
} lexer;

/* extract the file content to a buffer */
extern char *scan_file(const char *file);

/* initialize a lexer state */
extern void init_lexer(lexer *l, char *src, const char *file);

/* consumes all tokens in the current source */
extern token_list *cons_tokens(lexer *l);

/* consume token on demand */
extern token cons_token(lexer *l);

/* allocate a copy of token struct t on region reg */
extern token *alloc_token(token t, Region_N reg);

#endif
