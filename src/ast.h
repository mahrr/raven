/*
 * (ast.h | 8 Mar 19 | Amr Anwar)
 * 
 * the AST interface.
 * 
*/

#ifndef ast_h
#define ast_h

#include <stdint.h>

#include "list.h"
#include "lexer.h"

/** nodes value types **/

/* main nodes value types */
typedef enum {
    EXPR_STMT,
    FIXED_STMT,
    FN_STMT,
    LET_STMT,
    RET_STMT
} Stmt_VT;

typedef enum {
    ACCESS_EXPR,
    CALL_EXPR,
    FOR_EXPR,
    GROUP_EXPR,
    IDENT_EXPR,
    IF_EXPR,
    INDEX_EXPR,
    INFIX_EXPR,
    LIT_EXPR,
    MATCH_EXPR,
    PREFIX_EXPR,
    WHILE_EXPR
} Expr_VT;

typedef enum {
    FALSE_CPATT,
    FLOAT_CPATT,
    IDENT_PATT,
    INT_CPATT,
    LIST_PATT,
    NIL_CPATT,
    PAIR_PATT,
    RECORD_PATT,
    RSTR_CPATT,
    STR_CPATT,
    TRUE_CPATT
} Patt_VT;

/* sub nodes value types*/
typedef enum {
    FALSE_LIT,
    FLOAT_LIT,
    FN_LIT,
    INT_LIT,
    LIST_LIT,
    NIL_LIT,
    RECORD_LIT,
    RSTR_LIT,
    STR_LIT,
    TRUE_LIT
} Lit_expr_VT;

typedef enum {
    EXPR_BRANCH_VT,
    PIECE_BRANCH_VT
} Match_branch_VT;


/** nodes declaration **/

/* main nodes */
typedef struct AST_piece *AST_piece;
typedef struct AST_stmt  *AST_stmt;
typedef struct AST_expr  *AST_expr;
typedef struct AST_patt  *AST_patt;

/* statements sub nodes */
typedef struct AST_expr_stmt *AST_expr_stmt;
typedef struct AST_fn_stmt   *AST_fn_stmt;
typedef struct AST_let_stmt  *AST_let_stmt;
typedef struct AST_ret_stmt  *AST_ret_stmt;

/* expressions sub nodes */
typedef struct AST_access_expr *AST_access_expr;
typedef struct AST_call_expr   *AST_call_expr;
typedef struct AST_for_expr    *AST_for_expr;
typedef struct AST_group_expr  *AST_group_expr;
typedef struct AST_if_expr     *AST_if_expr;
typedef struct AST_index_expr  *AST_index_expr;
typedef struct AST_infix_expr  *AST_infix_expr;
typedef struct AST_lit_expr    *AST_lit_expr;
typedef struct AST_match_expr  *AST_match_expr;
typedef struct AST_prefix_expr *AST_prefix_expr;
typedef struct AST_while_expr  *AST_while_expr;

/* branches sub nodes */
typedef struct AST_elif_branch  *AST_elif_branch;
typedef struct AST_match_branch *AST_match_branch;

/* literal expressions sub nodes */
typedef struct AST_fn_lit     *AST_fn_lit;
typedef struct AST_list_lit   *AST_list_lit;
typedef struct AST_record_lit *AST_record_lit;

/* patterns sub nodes */
typedef struct AST_const_patt  *AST_const_patt;
typedef struct AST_pair_patt   *AST_pair_patt;
typedef struct AST_list_patt   *AST_list_patt;
typedef struct AST_record_patt *AST_record_patt;


/** nodes definition **/

/* main nodes (stmt, expr, patt) */
struct AST_piece {
    List_T stmts;
};

struct AST_stmt {
    Stmt_VT type;
    union {
        AST_expr_stmt expr;
        AST_fn_stmt fn;
        AST_let_stmt let;
        AST_ret_stmt ret;
        token_type fixed;  /* fixed statements */
    } obj;
};

struct AST_expr {
    Expr_VT type;
    union {
        AST_access_expr access;
        AST_call_expr call;
        AST_for_expr for_exp;
        AST_group_expr group;
        AST_if_expr if_exp;
        AST_index_expr index;
        AST_infix_expr infix;
        AST_lit_expr lit;
        AST_match_expr match;
        AST_prefix_expr prefix;
        AST_while_expr while_exp;
        char *ident;  /* identifier expression */
    } obj;
};

struct AST_patt {
    Patt_VT type;
    union {
        AST_list_patt list;
        AST_pair_patt pair;
        AST_record_patt record;
        char *id;          /* identifier(variable) pattern */
        int64_t *i;        /* literal int pattern */
        long double *f;    /* literal float pattern */
        char *s;           /* literal string pattern */
    } obj;
};

/* statements sub nodes */
struct AST_expr_stmt {
    AST_expr expr;
};

struct AST_let_stmt {
    char *name;
    AST_expr value;
};

struct AST_ret_stmt {
    AST_expr value;
};

struct AST_fn_stmt {
    char *name;
    List_T params;
    AST_piece body;
};

/* expressions sub nodes */
struct AST_access_expr {
    AST_expr object;
    char *field;
};

struct AST_call_expr {
    AST_expr func;
    List_T args;
};

struct AST_for_expr {
    char *name;
    AST_expr iter;
    AST_piece body;
};

struct AST_group_expr {
    AST_expr exp;
};

struct AST_elif_branch {
    AST_expr cond;
    AST_piece body;
};

struct AST_if_expr {
    AST_expr cond;
    AST_piece body;
    List_T elifs;    /* of AST_elif_branch */
    AST_piece alter;
};

struct AST_index_expr {
    AST_expr object;
    AST_expr index;
};

struct AST_infix_expr {
    token_type op;
    AST_expr left;
    AST_expr right;
};

struct AST_lit_expr {
    Lit_expr_VT type;
    union {
        AST_fn_lit fn;
        AST_list_lit list;
        AST_record_lit record;
        long double f; /* float literal */
        int64_t i;     /* integer literal */
        char *s;       /* string literal */
    } obj;
};

struct AST_match_branch {
    Match_branch_VT type;
    AST_patt patt;
    union {
        AST_expr e;
        AST_piece p;
    } obj;
};

struct AST_match_expr {
    AST_expr value;
    List_T branches; /* of AST_match_branch */
};

struct AST_prefix_expr {
    token_type op;
    AST_expr value;
};

struct AST_while_expr {
    AST_expr cond;
    AST_piece body;
};

/* literal expressions sub nodes */
struct AST_fn_lit {
    List_T params;
    AST_piece body;
};

struct AST_list_lit {
    List_T exps;   /* of AST_expr */
};

struct AST_record_lit {
    List_T names;  /* of (char*) */
    List_T values; /* of AST_expr */
};

struct AST_list_patt {
    List_T patts;  /* of AST_patt */
};

/* patterns sub nodes */
struct AST_pair_patt {
    AST_patt hd;
    AST_patt tl;
};

struct AST_record_patt {
    List_T names;  /* of (char*) */
    List_T patts;  /* pf AST_patt */
};

#endif
