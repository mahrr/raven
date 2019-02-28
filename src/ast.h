/*
 * (ast.h | 28 feb 19 | Amr Anwar)
 * 
 * The abstract syntax trees structs
 * 
*/

#include <stdint.h>
#include "list.h"

typedef struct {
    list *stmts;
} piece;

typedef enum {
    let_stmt_t,
    fn_stmt_t,
    ret_stmt_t,
    expr_stmt_t,
    fixed_stmt_t
} stmt_type;

typedef enum {
    BREAK,
    CONTINUE,
} fixed_stmt;

typedef struct {
    stmt_type type;
    union {
        let_stmt ls;
        fn_stmt fs;
        ret_stmt rs;
        expr_stmt es;
        fixed_stmt fs;
    } obj;
} stmt;

/*
fn_statement ::=
     "fn" name param_list piece "end";
*/

typedef struct {
    char *n_ident;
    param_list param_list;
    piece fn_p;
} fn_stmt;

/*
param_list ::= "(" [pattern {, pattern}] ")";
*/
typedef struct {
    pattern pattern;
    list patts;
} param_list;

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

typedef enum {
    lit_expr_t,
    prefix_expr_t,
    infix_expr_t,
    index_expr_t,
    access_expr_t,
    group_expr_t,
    call_expr_t,
    if_expr_t,
    for_expr_t,
    while_expr_t,
    match_expr_t,
    ident_expr_t,
} expr_type;

typedef struct {
    expr_type type;
    union {
        lit_expr le;
        prefix_expr pe;
        infix_expr ie;
        index_expr index_e;
        access_expr as;
        group_expr ge;
        call_expr ce;
        if_expr if_e;
        for_expr for_e;
        while_expr while_e;
        match_expr match_e;
        char *ident_n;
    } obj;
} expr;

typedef struct {
    expr expression;
    char *n_ident;
} access_expr;

typedef enum {
    dot_iop_t,
    plus_iop_t,
    minus_iop_t,
    asterisk_iop_t,
    slash_iop_t,
    percent_iop_t,
    col_col_iop_t,
    at_iop_t,
    colon_iop_t,
    lt_lt_iop_t,
    gt_gt_iop_t,
    ampersand_iop_t,
    caret_iop_t,
    pipe_iop_t,
    lt_iop_t,
    gt_iop_t,
    lt_eq_iop_t,
    gt_eq_iop_t,
    eq_eq_iop_t,
    bang_eq_iop_t,
    and_iop_t,
    or_iop_t,
} i_op_type;

typedef struct {
    i_op_type op;
    expr left;
    expr right;
} infix_expr;

typedef enum {
    hyphen_pop_t, /* - */ 
    tilde_pop_t, /* ~ */
    not_pop_t,
} p_op_type;

typedef struct {
    p_op_type op;
    expr expression;
} prefix_expr;

/*
index_expression ::=
     expression "[" expression "]";
*/

typedef struct {
    expr outer_e;
    expr inner_e;
} index_expr;

/*
group_expression ::=
     "(" expression ")";
*/
typedef struct {
    expr group_e;
} group_expr;

/*
call_expression ::= 
    expression "(" expr_list ")";
*/
typedef struct {
    expr expr;
    expr_list expression_l;
} call_expr;

typedef struct {
    expr expression;
    list exprs;
} expr_list;

/*
if_expression ::=
  "if" expression "do" piece 
  {"elif" expression "do" piece}
  ["else" piece] "end";
*/
typedef struct {
    expr if_e;
    piece if_p;
    list *elif_e; 
    piece elif_p;
    piece else_p;
} if_expr;

/*
for_expression ::=
     "for" name "in" expression "do" piece "end";
*/

typedef struct {
    char *name_i;
    expr for_e;
    piece for_p;
} for_expr;

/*
while_expression ::=
 "while" expression "do" piece "end";
*/
typedef struct {
    expr while_e;
    piece while_p;
} while_expr;

/*
match_expression ::= 
    "match" expression "do" 
    {
      "case" pattern "->" 
      (expression | "do" piece "end")
    }
   "end";
*/
typedef struct {
    expr match_e;
    list *match_l; 
} match_expr;

typedef enum {
    fn_lit_t,
    list_lit_t,
    record_lit_t,
    int_lit_t,
    float_lit_t,
    str_lit_t,
    rstr_lit_t,
    bool_lit_t,
    nil_lit_t,
} lit_type;

typedef struct {
    lit_type type;
    union {
        fn_lit fn_l;
        list_lit list_l;
        record_lit record_l;
        int_s i_val;
        float_s f_val;
        str_s s_val;
        rstr_s rs_val;
        bool_s b_val; // TRUE | FALSE
        // nil
    } obj;
} lit_expr;


typedef struct {
    int64_t i;
} int_s;

typedef struct {
    float f;
} float_s;

typedef struct {
    char *s;
} str_s;

typedef struct {
    char *s;
    char *inner_s;
} rstr_s;

typedef struct {
    int b;
} bool_s;

/*
fn_literal ::=
     "fn" "(" param_list ")" piece "end";
*/
typedef struct {
    param_list fn_param_l;
    piece fn_p;
} fn_lit;

/*
list_literal ::=
     "[" [expression {, expression}] "]";
*/
typedef struct {
    expr list_e;
    list *list_exprs;
} list_lit;

/*
record_literal ::= 
    "{" [record_field {, record_field}] "}";
record_field ::=
    name ":" expression;
*/
typedef struct {
    record_field record_f;
    list *record_fields;
} record_lit;

typedef struct {
    char *name;
    expr record_e;
} record_field;

/*
match_expression ::= 
    "match" expression "do" 
    {
      "case" pattern "->" 
      (expression | "do" piece "end")
    }
   "end";
*/

typedef struct {
    expr match_e;
    list *match_bs; 
} match_expr;

typedef enum {
    expr_t,
    piece_t,
} match_body_type;

typedef struct {
    match_body_type type;
    pattern patt;
    union {
        expr expr;
        piece piece;
    } obj;
} match_b;

/*
pattern ::= const_pattern
          | ident_pattern
          | list_pattern
          | record_pattern;

const_pattern ::= INTEGER
                | FLOAT
                | STRING
                | RSTRING
                | TRUE
                | FALSE
                | NIL;
ident_pattern ::= name;
*/
typedef enum {
    int_patt_t,
    float_patt_t,
    str_patt_t,
    rstr_patt_t,
    bool_patt_t,
    nil_patt_t,
    ident_patt_t,
    list_patt_t,
    record_patt_t,
} pattern_type;

typedef struct {
    pattern_type type;
    union {
        int_s i_val;
        float_s f_val;
        str_s s_val;
        rstr_s rs_val;
        bool_s b_val; 
        char *ident_n; // ident_pattern
        list_patt *list_pattern;
        record_patt record_pattern;
    } obj;
} pattern;

/*
list_pattern ::= "[" [pattern {, pattern}] "]"
               | pattern "::" pattern;
*/
// TODO check: | pattern "::" pattern;
typedef enum {
    patt_brucket_t,
    patt_scope_t,
} list_pattern_type;

typedef struct {
    list_pattern_type type;
    union {
        list_patt_bruckt p_brucket;
        list_pattern_scope p_scope;
    } obj;
} list_patt;

typedef struct {
    pattern p;
    list *patts;
} list_patt_bruckt;

typedef struct {
    pattern outer_p;
    pattern inner_p;
} list_pattern_scope;
/*
record_pattern ::= 
    "{" [pattern_field {, pattern_field}] "}";
*/
typedef struct {
    patt_field pf;
    list *p_fields;
} record_patt;
/*
pattern_field ::= name ":" pattern;
*/
typedef struct {
    char *name;
    pattern p;
} patt_field;
