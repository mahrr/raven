/*
 *
 * (parser.c | 27 Feb 19 | Kareem Hamdy)
*/
#include <stdlib.h>
#include <string.h>

#include "alloc.h"
#include "ast.h"
#include "lexer.h"
#include "list.h"
#include "parser.h"
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
    LOWEST_PREC,
    OR_PREC,
    AND_PREC,
    EQUALITY_PREC,
    COMPARE_PREC,
    BOR_PREC,  //B for Bitwise
    BXOR_PREC,
    BAND_PREC,
    SHIFT_PREC,
    LCONS_PREC,
    CONCATENATION_PREC,
    ADD_PREC,  /* add  muins */
    MULT_PREC, /*mult  dived mode */
    PREFIX_PREC,
    GROUP_PREC /* group and index and access prec*/
} precedence;

precedence precedenc_of(token_type type) {
    switch (type) {
        case TK_OR:
            return OR_PREC;
        case TK_AND:
            return AND_PREC;
        case TK_EQ_EQ:
        case TK_BANG_EQ:
            return EQUALITY_PREC;
        case TK_LT:
        case TK_GT:
        case TK_GT_EQ:
        case TK_LT_EQ:
            return COMPARE_PREC;
        case TK_PIPE:
            return BOR_PREC;
        case TK_CARET:
            return BXOR_PREC;
        case TK_AMPERSAND:
            return BAND_PREC;
        case TK_GT_GT:
        case TK_LT_LT:
            return SHIFT_PREC;
        case TK_COL_COL:
            return LCONS_PREC;
        case TK_AT:
            return CONCATENATION_PREC;
        case TK_PLUS:
        case TK_MINUS:
            return ADD_PREC; /* add  muins */
        case TK_ASTERISK:
        case TK_SLASH:
        case TK_PERCENT:
            return MULT_PREC; /*mult  dived mode */
        case TK_NOT:
        case TK_TILDE:
            return PREFIX_PREC;
        case TK_LPAREN:
        case TK_LBRACKET:
        case TK_DOT:
            return GROUP_PREC; /* group and index and access prec*/

        default:
            LOWEST_PREC;
    }
}

expr *parse_expr(parser *, precedence);

typedef expr *(*prefix_type)(parser *);
typedef expr *(*infix_type)(parser *, expr *);
/*
expr * assign_exp (expr * the_exp, expr_type t){
    expr * exp= make(exp,R_SECN);
    exp->type= t;
    exp->obj.pe= the_exp;
    return exp;
}*/

expr *parse_prefix_expr(parser *p) {
    /* current token = -  "-X" */
    prefix_expr *pre_exp = make(pre_exp, R_SECN);
    pre_exp->op = p->curr_token->type;
    next_token(p);
    pre_exp->expression = parse_expr(p, PREFIX_PREC);
    /* assgin to big exp */
    expr *exp = make(exp, R_SECN);
    exp->type = infix_expr_type;
    exp->obj.pe = pre_exp;
    return exp;
}

expr *parse_ident_expr(parser *p) {
    /*current token "x"  "x+y"*/
    expr *ident_exp = make(ident_exp, R_SECN);
    ident_exp->type = ident_expr_type;
    ident_exp->obj.ident_n = strn(p->curr_token->lexeme, p->curr_token->length);
    return ident_exp;
}

expr *parse_int_lit(parser *p) {
    int_lit *int_exp = make(int_exp, R_SECN);
    char *str_number = strn(p->curr_token->lexeme, p->curr_token->length);

    char *end = make(end, R_SECN);

    if ((*str_number) == '0') {
        switch (str_number[1]) {
            case 'x':
            case 'X':
                /*hex number */
                int_exp->i = strtol(str_number, end, 0);
                break;
            case 'o':
            case 'O':
                int_exp->i = strtol(str_number + 2, end, 0);
                break;
            case 'b':
            case 'B':
                int_exp->i = strtol(str_number + 2, end, 2);
                break;
            default:
                int_exp->i = strtol(str_number, end, 10);
        }
    } else {
        int_exp->i = strtol(str_number, end, 10);
    }

    if (*end != NULL) {
        /*Error in function "strn" */
    }

    lit_expr *li_exp = make(li_exp, R_SECN);
    li_exp->type = int_lit_type;
    li_exp->obj.i_val = int_exp;

    expr *exp = make(exp, R_SECN);
    exp->type = lit_expr_type;
    exp->obj.le = li_exp;
    return exp;
}

expr *parse_float_lit(parser *p) {
    float_lit *float_exp = make(float_exp, R_SECN);
    char *end = make(end, R_SECN);
    char *str_number = strn(p->curr_token->lexeme, p->curr_token->length);

    float_exp->f = strtod(str_number, end);

    if (*end != NULL) {
        /*Error in function strn */
    }

    lit_expr *li_exp = make(li_exp, R_SECN);
    li_exp->type = float_lit_type;
    li_exp->obj.f_val = float_exp;

    expr *exp = make(exp, R_SECN);
    exp->type = lit_expr_type;
    exp->obj.le = li_exp;
    return exp;
}

expr *parse_str_lit(parser *p) {
    str_lit *str_exp = make(str_exp, R_SECN);
    str_exp->s = strn(p->curr_token->lexeme + 1, p->curr_token->length - 1);

    lit_expr *li_exp = make(li_exp, R_SECN);
    li_exp->type = str_lit_type;
    li_exp->obj.s_val = str_exp;

    expr *exp = make(exp, R_SECN);
    exp->type = lit_expr_type;
    exp->obj.le = li_exp;
    return exp;
}

expr *parse_rstr_lit(parser *p) {
    rstr_lit *rstr_exp = make(rstr_exp, R_SECN);
    rstr_exp->s = strn(p->curr_token->lexeme + 1, p->curr_token->length - 1);

    lit_expr *li_exp = make(li_exp, R_SECN);
    li_exp->type = str_lit_type;
    li_exp->obj.rs_val = rstr_exp;

    expr *exp = make(exp, R_SECN);
    exp->type = lit_expr_type;
    exp->obj.le = li_exp;
    return exp;
}

expr *parse_true_lit(parser *p) {
    lit_expr *li_exp = make(li_exp, R_SECN);
    li_exp->type = true_type;

    expr *exp = make(exp, R_SECN);
    exp->type = lit_expr_type;
    exp->obj.le = li_exp;
    return exp;
}

expr *parse_false_lit(parser *p) {
    lit_expr *li_exp = make(li_exp, R_SECN);
    li_exp->type = false_type;

    expr *exp = make(exp, R_SECN);
    exp->type = lit_expr_type;
    exp->obj.le = li_exp;
    return exp;
}

expr *parse_nil_lit(parser *p) {
    lit_expr *li_exp = make(li_exp, R_SECN);
    li_exp->type = nil_lit_type;

    expr *exp = make(exp, R_SECN);
    exp->type = lit_expr_type;
    exp->obj.le = li_exp;
    return exp;
}

param_list *parse_param_list(parser *p) {
    if (!expect_token(p, TK_LPAREN)) {
        //add Error
        return NULL;
    }
    param_list *paraml_exp = make(paraml_exp, R_SECN);
    if (expect_token(p, TK_RPAREN)) {
        return paraml_exp;
    }
    paraml_exp->ident = strn(p->curr_token->lexeme, p->curr_token->length);
    paraml_exp->patts = make(paraml_exp->patts, R_SECN);
    append_list(paraml_exp->patts, paraml_exp->ident);

    while (expect_token(p, TK_COMMA)) {
        if (!expect_token(p, TK_IDENT))
            break;
        char *curr_id = strn(p->curr_token->lexeme, p->curr_token->length);
        //pattern * curr_p = parse_pattern(p);
        append_list(paraml_exp->patts, curr_id);
    }
    if (!expect_token(p, TK_RPAREN)) {
        /* error */
        return NULL;
    }
    return paraml_exp;
}

expr *parse_function_lit(parser *p) {
    fn_lit *fun_exp = make(fun_exp, R_SECN);
    fun_exp->fn_param_l = parse_param_list(p);
    next_token(p);

    fun_exp->fn_p = parse_piece(p);
    if (!expect_token(p, TK_END)) {
        /*add error */
        return NULL;
    }

    lit_expr *li_exp = make(li_exp, R_SECN);
    li_exp->type = fn_lit_type;
    li_exp->obj.fn_l = fun_exp;

    expr *exp = make(exp, R_SECN);
    exp->type = lit_expr_type;
    exp->obj.le = li_exp;
    return exp;
}

expr *parse_list_lit(parser *p) {
    /* pass "[" token */
    next_token(p);
    list_lit *list_exp = make(list_exp, R_SECN);
    list_exp->list_e = parse_expr(p, LOWEST_PREC);
    if (list_exp->list_e == NULL) {
        if (expect_token(p, TK_RBRACKET)) {
            return list_exp;
        } else {
            /*error */
            return NULL;
        }
    }
    list_exp->list_exprs = make(list_exp->list_exprs, R_SECN);
    append_list(list_exp->list_exprs, list_exp->list_e);

    while (expect_token(p, TK_COMMA)) {
        /* to pass Comma Token */
        next_token(p);
        expr *curr_exp = parse_expr(p, LOWEST_PREC);
        if (curr_exp == NULL) {
            /*Error */
            return NULL;
        }
        append_list(list_exp->list_exprs, curr_exp);
    }
    if (!expect_token(p, TK_RBRACKET)) {
        /*error */
        return NULL;
    }

    lit_expr *li_exp = make(li_exp, R_SECN);
    li_exp->type = list_lit_type;
    li_exp->obj.list_l = list_exp;

    expr *exp = make(exp, R_SECN);
    exp->type = lit_expr_type;
    exp->obj.le = li_exp;
    return exp;
}

record_field *parse_record(parser *p) {
    record_field *rec_f = make(rec_f, R_SECN);
    if (!expect_token(p, TK_IDENT)) {
        /*error */
        return NULL;
    }
    rec_f->name = strn(p->curr_token->lexeme, p->curr_token->length);
    if (!expect_token(p, TK_COLON)) {
        /*error */
        return NULL;
    }
    /* to pass : token */
    next_token(p);
    rec_f->experssion = parse_expr(p, LOWEST_PREC);
    if (rec_f->experssion == NULL) {
        /* Error */
        return NULL;
    }
    return rec_f;
}

expr *parse_record_lit(parser *p) {
    record_lit *record_l = make(record_l, R_SECN);
    record_l->record_f = parse_record(p);
    if (record_l->record_f == NULL) {
        /*error */
        return NULL;
    }

    record_l->record_fields = make(record_l->record_fields, R_SECN);
    append_list(record_l->record_fields, record_l->record_f);
    while (expect_token(p, TK_COMMA)) {
        /* no need to pass comma_token cause I use expect_token in parse_record() function */
        record_field *curr_record = parse_record(p);
        //Erorr
        if (curr_record == NULL)
            return NULL;
        append_list(record_l->record_fields, curr_record);
    }
    if (!expect_token(p, TK_RBRACE)) {
        /*error */
        return NULL;
    }

    lit_expr *li_exp = make(li_exp, R_SECN);
    li_exp->type = record_lit_type;
    li_exp->obj.record_l = record_l;

    expr *exp = make(exp, R_SECN);
    exp->type = lit_expr_type;
    exp->obj.le = li_exp;
    return exp;
}

expr *parse_group_exp(parser *p) {
    group_expr *group_exp = make(group_exp, R_SECN);
    /* to pass "(" token */
    next_token(p);
    group_exp->group_e = parse_expr(p, GROUP_PREC);

    if (group_exp->group_e == NULL) { /*Error */
        return NULL;
    }
    if (!expect_token(p, TK_RPAREN)) {
        /* error */
        return NULL;
    }

    expr *exp = make(exp, R_SECN);
    exp->type = group_expr_type;
    exp->obj.ge = group_exp;
    return exp;
}

expr *parse_if_exp(parser *p) {
    if_expr *if_exp = make(if_exp, R_SECN);
    /* to pass  "IF" key word token */
    next_token(p);
    if_exp->if_e = parse_expr(p, LOWEST_PREC);
    if (if_exp->if_e == NULL) {
        /*Error */
        return NULL;
    }
    if (!expect_token(p, TK_DO)) {
        /*error*/
        return NULL;
    }
    /* to pass  "do" key word token */
    next_token(p);
    if_exp->if_p = parse_piece(p);

    /*elif part */
    while (expect_token(p, TK_ELIF)) {
        /* to pass  "ELIF" key word token */
        next_token(p);
        if_exp->elif_e = parse_expr(p, LOWEST_PREC);

        if (if_exp->elif_e == NULL) {
            /*Error */
            return NULL;
        }
        if (!expect_token(p, TK_DO)) {
            /*error*/
            return NULL;
        }
        /* to pass  "do" key word token */
        next_token(p);
        if_exp->elif_p = parse_piece(p);
    }
    /* else part */
    if (expect_token(p, TK_ELSE)) {
        if_exp->else_p = parse_piece(p);
    }
    if (!expect_token(p, TK_END)) {
        /* error */
        return NULL;
    }
    expr *exp = make(exp, R_SECN);
    exp->type = if_expr_type;
    exp->obj.if_e = if_exp;
    return exp;
}

expr *parse_while_exp(parser *p) {
    while_expr *while_exp = make(while_exp, R_SECN);
    /* to pass while token */
    next_token(p);
    while_exp->while_e = parse_expr(p, LOWEST_PREC);
    if (while_exp->while_e == NULL) {
        /*Error */
        return NULL;
    }
    if (!expect_token(p, TK_DO)) {
        /*error*/
        return NULL;
    }
    /* to pass  "do" key word token */
    next_token(p);
    while_exp->while_p = parse_piece(p);
    if (!expect_token(p, TK_END)) {
        /* error */
        return NULL;
    }
    expr *exp = make(exp, R_SECN);
    exp->type = while_expr_type;
    exp->obj.while_e = while_exp;
    return exp;
}

expr *parse_for_exp(parser *p) {
    for_expr *for_exp = make(for_exp, R_SECN);
    if (!expect_token(p, TK_IDENT)) {
        /*error*/
        return NULL;
    }
    for_exp->for_e = parse_expr(p, LOWEST_PREC);
    if (for_exp->for_e == NULL) {
        /*Error */
        return NULL;
    }
    if (!expect_token(p, TK_DO)) {
        /*error*/
        return NULL;
    }
    /* to pass  "do" key word token */
    next_token(p);
    for_exp->for_p = parse_piece(p);

    if (!expect_token(p, TK_END)) {
        /* error */
        return NULL;
    }
    expr *exp = make(exp, R_SECN);
    exp->type = for_expr_type;
    exp->obj.for_e = for_exp;
    return exp;
}

match_body *parse_match_body(parser *p) {
    match_body *m_body = make(m_body, R_SECN);
    if (!expect_token(p, TK_IDENT)) {
        /*Error */
        return NULL;
    }
    /*parse pattern */
    m_body->patt = strn(p->curr_token->lexeme, p->curr_token->length);
    if (!expect_token(p, TK_DASH_GT)) {
        /* Error */
        return NULL;
    }
    if (expect_token(p, TK_DO)) {
        next_token(p);
        m_body->type = piece_body_type;
        m_body->obj.piece = parse_piece(p);
        if (m_body->obj.piece == NULL) {
            /* Error */
            return NULL;
        }
        if (!expect_token(p, TK_END)) {
            /*error */
            return NULL;
        }

    } else {
        next_token(p);
        m_body->type = expr_body_type;
        m_body->obj.expr = parse_expr(p, LOWEST_PREC);
        if (m_body->obj.expr == NULL) {
            /* Error */
            return NULL;
        }
    }
}

expr *parse_match_exp(parser *p) {
    match_expr *match_exp = make(match_exp, R_SECN);
    next_token(p);
    match_exp->match_e = parse_expr(p, LOWEST_PREC);
    if (match_exp->match_e == NULL) {
        /* error */
        return NULL;
    }
    if (!expect_token(p, TK_DO)) {
        /*error*/
        return NULL;
    }
    /* to pass do token and New line Token  */

    next_token(p);
    while (p->curr_token->type == TK_NL) {
        next_token(p);
    }
    match_exp->match_bs = make(match_exp->match_bs, R_SECN);

    while (expect_token(p, TK_CASE)) {
        match_body *curr = parse_match_body(p);
        if (curr == NULL) {
            /*error */
            return NULL;
        }
        append_list(match_exp->match_bs, curr);
    }
    if (!expect_token(p, TK_END)) {
        /*Error */
        return NULL;
    }
    expr *exp = make(exp, R_SECN);
    exp->type = match_expr_type;
    exp->obj.match_e = match_exp;
    return exp;
}

prefix_type prefix_of(token_type type) {
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
            return parse_nil_lit;
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
        case TK_DOT:
            return parse_access_exp;
        case TK_LPAREN:
            return parse_call_exp;
        case TK_LBRACKET:
            return parse_index_exp;

        default:
            return NULL;
    }
}

expr *parse_expr(parser *p, precedence prec) {
    /* 15 + 1 */

    /* current is "15" */
    prefix_type pre = prefix_of(p->curr_token->type);
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
    /* let x = 15+1 ; */

    let_stmt *let_s = make(let_s, R_SECN);
    /* current let next is "x" */
    if (!expect_token(p, TK_IDENT)) {
        //TODO add error
        return NULL;
    }
    /* current "X" next is "=" */
    let_s->ident = strn(p->curr_token->lexeme, p->curr_token->length);

    if (!expect_token(p, TK_EQ)) {
        //TODO add error
        return NULL;
    }
    /*current "=" next is "15" */
    next_token(p);

    /*current "15" next is "+" */
    let_s->expression = parse_expr(p, LOWEST_PREC);
    if (let_s->expression == NULL) {
        //TODO add error
        return NULL;
    }
    if (!expect_token(p, TK_SEMICOLON) || !expect_token(p, TK_NL)) {
        //TODO add error
        return NULL;
    }
    stmt *s = make(s, R_SECN);
    s->type = let_stmt_type;
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

piece *parse_piece(parser *p) {
    piece *prog = make(prog, R_SECN);
    prog->stmts = make(prog->stmts, R_SECN);

    token *tok = p->curr_token;
    /* to skip  empty lines in the begin and set current token to real token*/
    while (p->curr_token->type == TK_NL)
        next_token(p);

    while (tok != NULL && tok->type != TK_EOF) {
        stmt *s = parse_stmt(p);
        if (s != NULL) {
            prog->stmts = append_list(prog->stmts, s);
        }
    }

    return prog;
}
