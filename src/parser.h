/*
 * (parser.h | 27 Feb 19 | Ahmad Maher, Kareem hamdy)
 *
 * Parser Interface
 *
*/

#ifndef parser_h
#define parser_h

#include <stdio.h>

#include "array.h"
#include "ast.h"
#include "error.h"
#include "token.h"

typedef struct Parser {
    Token *tokens;      /* array of generated tokens */
    int ident_num;      /* number of AST_expr_ident nodes int the src */
    int been_error;     /* error flag */
    ARRAY(SErr) errors; /* parse error */

    /* state of parser */
    Token *curr;
    Token *prev;
    Token *peek;
} Parser;

/* initialize a parser state with tokens array */
void init_parser(Parser *parser, Token *tokens);

/* parse AST_piece node */
AST_piece parse_piece(Parser *parser);

/* parse AST_stmt node */
AST_stmt parse_stmt(Parser *parser);

/* parse AST_expr node */
AST_expr parse_expr(Parser *parser);

/* parse AST_patt node */
AST_patt parse_patt(Parser *parser);

/* 1 if there is a parsing error, 0 otherwise. */
#define parser_error(parser) ((parser)->been_error)

/* a pointer to the array of the parser (SErr) errors. */
#define parser_errors(parser) ((parser)->errors.elems)

/* number of parsing errors. */
#define parser_errnum(parser) ((parser)->errors.len)

#endif
