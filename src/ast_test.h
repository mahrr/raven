#include <stdint.h>
#include "list.h"

typedef struct {
    list *stmts;
} program;

typedef enum {
    let_stmt_t,
    ret_stmt_t,
    expr_stmt_t,
    fixed_stmt_t
} stmt_type;

typedef struct {
    stmt_type type;
    union {
        let_stmt * ls;
        ret_stmt *rs;
        expr_stmt *es;
    }obj;
} stmt;

typedef struct {
    char *ident;
    expr *expression;
} let_stmt;

typedef struct {
    expr expression;
} ret_stmt;

typedef struct {
    expr expression;
} expr_stmt;

typedef enum {
    lit_expr_type,
    prefix_expr_type,
    infix_expr_type,
} expr_type;

typedef struct {
    expr_type type;
    union {
        lit_expr *le;
        prefix_expr *pe;
        infix_expr *ie;
    } obj;
} expr;

typedef enum {
    int_lit_t,
    str_lit_t,
    bool_lit_t,
} lit_type;

typedef struct {
    lit_type type;
    union {
        int_lit i_val;
        str_lit s_val;
        bool_lit b_val;
    } obj;
} lit_expr;


typedef struct {
    int64_t i;
} int_lit;

typedef struct {
    char *s;
} str_lit;

typedef struct {
    int b;
} bool_lit;

typedef enum {
    minus_iop_t,
    plust_iop_t,
    at_iop_t,
} i_op_type;

typedef struct {
    i_op_type op;
    expr left;
    expr right;
} infix_expr;

typedef enum {
    minus_pre_type,
    not_pre_type,
    complement_pre_type
} pre_op_type;

typedef struct {
    pre_op_type op;
    expr *expression;
} prefix_expr;