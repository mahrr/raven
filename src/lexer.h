/*
 * (lexer.h | 22 Nov 18 | Ahmad Maher)
 *
 * l - lexcical analyzer
*/

#ifndef lexer_h
#define lexer_h

#include <stdbool.h>

#include "list.h"

typedef enum {
    /* Literals */
    INT, FLOAT, STRING,
    R_STRING, FALSE, TRUE, NIL,

    /* Keywords */
    MODULE, FN, TYPE, OF,
    RETURN, LET, FIN, USE,
    DO, END, IF, THEN, ELIF,
    ELSE, FOR, WHILE, CONTINUE,
    BREAK, RAISE, HANDLE,
    MATCH, CASE,

    /* Identefier */
    IDENT,

    /* Types */
    NUM_T, INT_T, FLOAT_T,
    STR_T, BOOL_T, LIST_T,

    /* Operators */
    EQUAL, AND, OR, NOT, IN,
    DOT, DOT_DOT, HASH,
    AT, COLON, EQUAL_GREAT,
    GREAT_PIPE,

    /* Arthimetik Operators */
    PLUS, MINUS, ASTERISK,
    SLASH, PERCENT,

    /* Ordering Operators */
    LESS, GREAT, EQUAL_EQUAL,
    BANG_EQUAL, LESS_EQUAL,
    GREAT_EQUAL,

    /* Logic Operators */
    PIPE, AMPERSAND, CARET,
    TILDE, GREAT_GREAT, LESS_LESS,

    /* Delimiters */
    LPAREN, RPAREN, LBRACE, RBRACE,
    LBRACKET, RBRACKET, COMMA, TK_NL,

    /* Errors and end of file */
    UNRECOG, UNTERMIN_STR,
    INVALID_ESCP, OCT_OUTOFR_ESCP,
    OCT_MISS_ESCP, OCT_INVL_ESCP,
    HEX_MISS_ESCP, HEX_INVL_ESCP,
    ZERO_START, INVALID_SCIEN,
    EOF_TOK
} token_type;

typedef struct {
    token_type type;
    char *lexeme;
    char *file;
    long line;
} token;

typedef struct {
    list *tokens;
    bool been_error;
    list *error_tokens;
} token_list;

typedef struct {
    char *current;    /* the current unconsumed char in the source */
    char *fixed;      /* the start of the current token */
    char *file_name;  /* the source file name */
    long line;        /* the current line number */
} lexer;

/* check if the current token is error_token */
#define is_errtok(t)                                        \
    ((t)->type ==  UNRECOG        ||                        \
     (t)->type == UNTERMIN_STR    ||                        \
     (t)->type == INVALID_ESCP    ||                        \
     (t)->type == OCT_OUTOFR_ESCP ||                        \
     (t)->type == OCT_MISS_ESCP   ||                        \
     (t)->type == OCT_INVL_ESCP   ||                        \
     (t)->type == HEX_MISS_ESCP   ||                        \
     (t)->type == HEX_INVL_ESCP   ||                        \
     (t)->type == INVALID_SCIEN)

/* extract the file content to a buffer */
extern char *scan_file(const char *file);

/* initialize a lexer state */
extern void init_lexer(lexer *l, char *src, const char *file);

/* consumes all tokens in the current source */
extern token_list *cons_tokens(lexer *l);

/* consume token on demand */
extern token cons_token(lexer *l);

/* allocate a copy of token struct t on region reg */
extern token *alloc_token(token t, unsigned reg);

/* print tokens; just for debugging puporse */
extern void print_token(token *t);

#endif
