/*
 * (parser.h | 27 Feb 19 | Kareem hamdy, Ahmad Maher)
 *
 * the parser interface.
 *
*/

#ifndef parser_h
#define parser_h

#include "alloc.h"
#include "list.h"
#include "ast.h"

typedef struct Parser_T *Parser_T;

/* allocate a new parser on reg region with tokens list */
extern Parser_T Parser_new(List_T tokens, Region_N reg);

/* parse AST_piece node */
extern AST_piece Parser_parse(Parser_T p);

/* parse AST_stmt node */
extern AST_stmt Parser_parse_stmt(Parser_T p);

/* parse AST_expr node */
extern AST_expr Parser_parse_expr(Parser_T p);

#endif
