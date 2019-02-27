/*
 *
 * (parser.c | 27 Feb 19 | Kareem Hamdy)
*/

#include "parser.h"
#include "alloc.h"
#include "ast_test.h"
#include "lexer.h"
#include "list.h"
#include "salloc.h"

void init_parser(parser *p, token *toks) {
    assert(toks != NULL);
    assert(p != NULL);
    p->tokens = toks;
    p->curr_token = toks;
    p->next_token = toks + 1;
    p->file = toks->file;
}

#define next_token(p) p->curr_token = p->next_token++
#define expect_token(p, t) p->next_token->type == t ? (next_token(p), 1) : 0

// for experssions parsing
typedef enum {
    LOWEST,
    OR,
    AND,
    EQUALITY,
    COMPARE,
    BOR,  //B for Bitwise
    BXOR,
    BAND,
    SHIFT,
    L_CONS,
    CONCATENATION,
    ADDSUB,
    MULT_DIV_MOD,
    NOT_NEGATE_COMPLEMENT,
    GROUP_INDEX_ACCESS
} precedence;
typedef expr *(*prefix_t)(parser *);
typedef expr *(*infix_type)(parser *, expr *);

prefix_t prefix_of(token_type type) {
    switch (type) {
        case TK_MINUS:
        case TK_NOT:
        case TK_TILDE:
            return parse_prefix_expr;
        case TK_IDENT:
            return parse_ident_expr;
        case TK_INT:
            return parse_int_lit;
        case TK_FLOAT:
            return parse_float_lit;
        case TK_STR:
            return parse_str_lit;
        case TK_RSTR:
            return parse_rstr_lit;
        case TK_TRUE:
            return parse_true_lit;
        case TK_FALSE:
            return parse_false_lit;
        case TK_NIL:
            return parse_literal_expr;
        case TK_FN:
            return parse_function_lit;
        case TK_LBRACKET:
            return parse_list_lit;
        case TK_LBRACE:
            return parse_record_lit;
        case TK_LPAREN:
            return parse_group_exp;
        case TK_IF:
            return parse_if_exp;
        case TK_WHILE:
            return parse_while_exp;
        case TK_FOR:
            return parse_for_exp;
        case TK_MATCH:
            return parse_match_exp;

        default:
            return NULL;
    }
}
infix_type infix_of(token_type type) {
    switch (type) {
        case TK_PLUS:
        case TK_MINUS:
        case TK_ASTERISK:
        case TK_SLASH:
        case TK_PERCENT:
        case TK_AT:
        case TK_COL_COL:
        case TK_GT_GT:
        case TK_LT_LT:
        case TK_AMPERSAND:
        case TK_PIPE:
        case TK_LT:
        case TK_GT:
        case TK_GT_EQ:
        case TK_LT_EQ:
        case TK_EQ_EQ:
        case TK_BANG_EQ:
        case TK_AND:
        case TK_OR:
            return parse_infix;
        case TK_DOT :
            return parse_access_exp;
        case TK_LPAREN: 
        return  parse_call_exp;
        case TK_LBRACKET:
        return parse_index_exp;

        default:
            return NULL;
    }
}

expr *parse_expr(parser *p, precedence prec) {
    prefix_t pre = prefix_of(p->curr_token->type);
    if (pre == NULL) {
        //TODO add error
        return NULL;
    }
    expr *left_expr = pre(p);
    while (!expect_token(p, TK_NL) &&
           !expect_token(p, TK_SEMICOLON) &&
           prec < precedenc_of(p->next_token)) {
        infix_type inf = infex_of(p->next_token->type);
        if (inf == NULL) {
            return left_expr;
        }
        next_token(p);
        left_expr = inf(p, left_expr);
    }
    return left_expr;
}

stmt *parse_let_stmt(parser *p) {
    let_stmt *let_s = make(let_s, R_SECN);
    if (!expect_token(p, TK_IDENT)) {
        //TODO add error
        return NULL;
    }
    let_s->ident = str(p->curr_token->lexeme);
    if (!expect_token(p, TK_EQ)) {
        //TODO add error
        return NULL;
    }
    let_s->expression = parse_expr(p, LOWEST);
    if (let_s->expression == NULL) {
        //TODO add error
        return NULL;
    }
    stmt *s = make(s, R_SECN);
    s->type = let_stmt_t;
    s->obj.ls = let_s;
    return s;
}

stmt *parse_stmt(parser *p) {
    switch (p->curr_token->type) {
        case TK_LET:
            return parse_let_stmt(p);

        default:
            break;
    }
}

program *parse_program(parser *p) {
    program *prog = make(prog, R_SECN);
    prog->stmts = make(prog->stmts, R_SECN);

    token *tok = p->curr_token;
    while (tok != NULL && tok->type != TK_EOF) {
        stmt *s = parse_stmt(p);
        if (s != NULL) {
            prog->stmts = append_list(prog->stmts, s);
        }
    }

    return prog;
}
