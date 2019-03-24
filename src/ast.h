/*
 * (ast.h | 8 Mar 19 | Amr Anwar)
 * 
 * The abstract syntax trees structs
 * 
*/

#ifndef ast_h
#define ast_h

#include <stdint.h>

#include "list.h"
#include "lexer.h"

typedef struct expr expr;
typedef struct patt patt;

typedef struct {
    list *stmts;
} piece;

typedef enum {
    let_stmt_type,
    fn_stmt_type,
    ret_stmt_type,
    expr_stmt_type,
    fixed_stmt_type
} stmt_type;

typedef struct {
    char *name;
    list *params;
    piece *body;
} fn_stmt;

typedef struct {
    char *name;
    expr *value;
} let_stmt;

typedef struct {
    expr *retval;
} ret_stmt;

typedef struct {
    expr *exp;
} expr_stmt;

typedef struct {
    stmt_type type;
    union {
        let_stmt *ls;
        fn_stmt *fns;
        ret_stmt *rs;
        expr_stmt *es;
        token_type fs;
    } obj;
} stmt;

typedef enum {
    lit_expr_type,
    prefix_expr_type,
    infix_expr_type,
    index_expr_type,
    access_expr_type,
    group_expr_type,
    call_expr_type,
    if_expr_type,
    for_expr_type,
    while_expr_type,
    match_expr_type,
    ident_expr_type,
} expr_type;

typedef struct {
    expr *object;
    char *field;
} access_expr;

typedef struct {
    token_type op;
    expr *left;
    expr *right;
} infix_expr;

typedef struct {
    token_type op;
    expr *value;
} prefix_expr;

typedef struct {
    expr *object;
    expr *index;
} index_expr;

typedef struct {
    expr *exp;
} group_expr;

typedef struct {
    expr *func;
    list *args;
} call_expr;

typedef struct {
    expr *cond;
    piece *body;
} elif_branch;

typedef struct {
    expr *cond;
    piece *body;
    list *elifs;
    piece *alter;
} if_expr;

typedef struct {
    char *name;
    expr *iter;
    piece *body;
} for_expr;

typedef struct {
    expr *cond;
    piece *body;
} while_expr;

typedef enum {
    expr_branch_type,
    piece_branch_type,
} match_branch_type;

typedef struct {
    match_branch_type type;
    char* patt;
    union {
        expr *e;
        piece *p;
    } obj;
} match_body;

typedef struct {
    expr *value;
    list *branches;
} match_expr;

typedef enum {
    fn_lit_type,
    list_lit_type,
    record_lit_type,
    int_lit_type,
    float_lit_type,
    str_lit_type,
    rstr_lit_type,
    true_lit_type,
    false_lit_type,
    nil_lit_type,
} lit_type;

typedef struct {
    list *params;
    piece *body;
} fn_lit;

typedef struct {
    list *exps;
} list_lit;

typedef struct {
    list *names;
    list *values;
} record_lit;

typedef struct {
    lit_type type;
    union {
        fn_lit *fn_l;
        list_lit *list_l;
        record_lit *record_l;
        char *s_val;
        int64_t i_val;
        double f_val;
    } obj;
} lit_expr;

struct expr {
    expr_type type;
    union {
        lit_expr *lit_e;
        prefix_expr* pre_e;
        infix_expr *inf_e;
        index_expr *index_e;
        access_expr* access_e;
        group_expr *group_e;
        call_expr *call_e;
        if_expr *if_e;
        for_expr *for_e;
        while_expr *while_e;
        match_expr *match_e;
        char *ident;
    } obj;
};

typedef enum {
    int_patt_type,
    float_patt_type,
    str_patt_type,
    rstr_patt_type,
    true_patt_type,
    false_patt_type,
    nil_patt_type,
    ident_patt_type,
    list_patt_type,
    record_patt_type,
} patt_type;

typedef struct {
    patt *hd;
    patt *tl;
} pair_patt;

typedef struct {
    list *patts;
} list_patt;

typedef struct {
    list *names;
    list *patts;
} record_patt;

struct patt {
    patt_type type;
    union {
        char *name;
        int64_t *i_const;
        double *f_const;
        char *s_const;
        list_patt *l_patt;
        record_patt *r_patt;
    } obj;
};

#endif
