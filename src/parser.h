/*
 * (parser.h | 27 Feb 19 | Ahmad Maher, Kareem hamdy)
 *
 * Parser Interface
 *
*/

#ifndef parser_h
#define parser_h

#include <stdio.h>

#include "alloc.h"
#include "list.h"
#include "ast.h"

typedef struct Parser *Parser;

/* initialize a parser state with tokens list */
extern void init_parser(Parser p, List tokens);

/* allocate a new parser on reg region with tokens list */
extern Parser parser_new(List tokens, Region_N reg);

/* predicate for parse error ocurrence */
extern int parser_error(Parser p);

/* print parse errors, if any, to a stream 'out' */
extern void parser_log(Parser p, FILE *out);

/* parse AST_piece node */
extern AST_piece parse_piece(Parser p);

/* parse AST_stmt node */
extern AST_stmt parse_stmt(Parser p);

/* parse AST_expr node */
extern AST_expr parse_expr(Parser p);

/* parse AST_patt node */
extern AST_patt parse_patt(Parser p);

#endif
