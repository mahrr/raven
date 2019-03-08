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
typedef struct pattern pattern;

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

typedef enum {
    BREAK,
    CONTINUE,
} fixed_stmt;

typedef struct {
    char* ident;
    list* patts;
} param_list;

typedef struct {
    char *n_ident;
    list *param_list;
    piece *fn_p;
} fn_stmt;

typedef struct {
    char *ident;
    expr *expression;
} let_stmt;

typedef struct {
    expr *expression;
} ret_stmt;

typedef struct {
    expr *expression;
} expr_stmt;

typedef struct {
    stmt_type type;
    union {
        let_stmt * ls;
        fn_stmt *fns;
        ret_stmt *rs;
        expr_stmt *es;
        fixed_stmt fs;
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
    expr *expression;
    char *n_ident;
} access_expr;

typedef struct {
    token_type op;
    expr *left;
    expr *right;
} infix_expr;

typedef struct {
    token_type op;
    expr *expression;
} prefix_expr;

typedef struct {
    expr *outer_e;
    expr *inner_e;
} index_expr;

typedef struct {
    expr *group_e;
} group_expr;

typedef struct {
    expr *expression;
    list *exprs;
} expr_list;

typedef struct {
    expr *expr;
    list *expression_l;
} call_expr;

typedef struct {
    expr *if_e;
    piece *if_p;
    expr *elif_e; 
    piece *elif_p;
    piece *else_p;
} if_expr;

typedef struct {
    char *name_i;
    expr *for_e;
    piece *for_p;
} for_expr;

typedef struct {
    expr *while_e;
    piece *while_p;
} while_expr;

typedef struct {
    expr *match_e;
    list *match_bs; 
} match_expr;

typedef enum {
    fn_lit_type,
    list_lit_type,
    record_lit_type,
    int_lit_type,
    float_lit_type,
    str_lit_type,
    rstr_lit_type,
    true_type,
    false_type,
    nil_lit_type,
} lit_type;

typedef struct {
    int64_t i;
} int_lit;

typedef struct {
    double f;
} float_lit;

typedef struct {
    char *s;
} str_lit;

typedef struct {
    char *s;
} rstr_lit;

typedef struct {
    list *fn_param_l;
    piece *fn_p;
} fn_lit;

typedef struct {
    expr *list_e;
    list *list_exprs;
} list_lit;

typedef struct {
    char *name;
    expr *experssion;
} record_field;

typedef struct {
    record_field *record_f;
    list *record_fields;
} record_lit;

typedef struct {
    lit_type type;
    union {
        fn_lit *fn_l;
        list *list_l;
        record_lit *record_l;
        int_lit *i_val;
        float_lit *f_val;
        str_lit *s_val;
        rstr_lit *rs_val;
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
        char *ident_n;
    } obj;
};

typedef enum {
    expr_body_type,
    piece_body_type,
} match_body_type;

typedef struct {
    match_body_type type;
    char* patt;
    union {
        expr *expr;
        piece *piece;
    } obj;
} match_body;

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
} pattern_type;

typedef struct {
    pattern *outer_p;
    pattern *inner_p;
} list_pattern_scope;

typedef struct {
    pattern *p;
    list *patts;
} list_patt_bruckt;

typedef enum {
    patt_brucket_type,
    patt_scope_type,
} list_pattern_type;

typedef struct {
    list_pattern_type type;
    union {
        list_patt_bruckt *p_brucket;
        list_pattern_scope *p_scope;
    } obj;
} list_patt;

typedef struct {
    char *name;
    pattern *p;
} patt_field;

typedef struct {
    patt_field *pf;
    list *p_fields;
} record_patt;


struct pattern {
    pattern_type type;
    union {
        int_lit *i_val;
        float_lit *f_val;
        str_lit *s_val;
        rstr_lit *rs_val;
        char *ident_n;
        list *list_pattern;
        record_patt *record_pattern;
    } obj;
};

#endif
