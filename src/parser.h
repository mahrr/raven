/*
 *
 * (parser.h | 27 Feb 19 | Kareem Hamdy)
*/

#include "lexer.h"
#include "list.h"
#include "ast_test.h"

typedef struct {
    token *tokens;
    token *curr_token;
    token *next_token;
    list *errors;
    const char *file;
} parser;

typedef struct{
    const char *msg;
    long line;
    token *start;
} p_error;

extern void init_parser(parser*, token*);
extern program *parse_program (parser*);