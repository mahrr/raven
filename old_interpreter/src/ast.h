/*
 * (ast.h | 8 Mar 19 | Amr Anwar)
 * 
 * the AST interface.
 * 
*/

#ifndef ast_h
#define ast_h

#include <stdint.h>

#include "token.h"

/** nodes value types **/

/* main nodes value types */
typedef enum {
    EXPR_STMT,
    FIXED_STMT,
    FN_STMT,
    LET_STMT,
    RET_STMT,
    TYPE_STMT
} Stmt_VT;

typedef enum {
    ASSIGN_EXPR,
    BINARY_EXPR,
    CALL_EXPR,
    COND_EXPR,
    FOR_EXPR,
    GROUP_EXPR,
    IDENT_EXPR,
    IF_EXPR,
    INDEX_EXPR,
    LIT_EXPR,
    MATCH_EXPR,
    UNARY_EXPR,
    WHILE_EXPR
} Expr_VT;

typedef enum {
    BOOL_CPATT,
    CONS_PATT,
    FLOAT_CPATT,
    HASH_PATT,
    IDENT_PATT,
    INT_CPATT,
    LIST_PATT,
    NIL_CPATT,
    PAIR_PATT,
    STR_CPATT,
} Patt_VT;

/* sub nodes value types*/
typedef enum {
    FALSE_LIT,
    FLOAT_LIT,
    FN_LIT,
    HASH_LIT,
    INT_LIT,
    LIST_LIT,
    NIL_LIT,
    STR_LIT,
    TRUE_LIT
} Lit_Expr_VT;

typedef enum {
    EXPR_ARM,
    PIECE_ARM
} Arm_VT;


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
typedef struct AST_type_stmt *AST_type_stmt;

/* constructor variant for type statements */
typedef struct AST_variant *AST_variant;

/* expressions sub nodes */
typedef struct AST_assign_expr *AST_assign_expr;
typedef struct AST_binary_expr *AST_binary_expr;
typedef struct AST_call_expr   *AST_call_expr;
typedef struct AST_cond_expr   *AST_cond_expr;
typedef struct AST_for_expr    *AST_for_expr;
typedef struct AST_group_expr  *AST_group_expr;
typedef struct AST_ident_expr  *AST_ident_expr;
typedef struct AST_if_expr     *AST_if_expr;
typedef struct AST_index_expr  *AST_index_expr;
typedef struct AST_lit_expr    *AST_lit_expr;
typedef struct AST_match_expr  *AST_match_expr;
typedef struct AST_unary_expr  *AST_unary_expr;
typedef struct AST_while_expr  *AST_while_expr;

/* branches/fields sub nodes */
typedef struct AST_elif *AST_elif;
typedef struct AST_key  *AST_key;
typedef struct AST_arm  *AST_arm;

/* literal expressions sub nodes */
typedef struct AST_fn_lit   *AST_fn_lit;
typedef struct AST_hash_lit *AST_hash_lit;
typedef struct AST_list_lit *AST_list_lit;

/* patterns sub nodes */
typedef struct AST_cons_patt *AST_cons_patt;
typedef struct AST_hash_patt *AST_hash_patt;
typedef struct AST_pair_patt *AST_pair_patt;
typedef struct AST_list_patt *AST_list_patt;


/** nodes definition **/

/*
 * note: 
 * all the arrays fields in AST nodes struct
 * are null terminated arrays.
*/

/* main nodes (stmt, expr, patt) */
struct AST_piece {
    AST_stmt *stmts;  /* array */
};

struct AST_stmt {
    Stmt_VT type;
    Token *where;
    union {
        AST_expr_stmt expr;
        AST_fn_stmt fn;
        AST_let_stmt let;
        AST_ret_stmt ret;
        AST_type_stmt type_stmt;
        TK_type fixed;
    };
};

struct AST_expr {
    Expr_VT type;
    Token *where;
    union {
        AST_assign_expr assign;
        AST_binary_expr binary;
        AST_call_expr call;
        AST_cond_expr cond;
        AST_for_expr for_expr;
        AST_group_expr group;
        AST_ident_expr ident;
        AST_if_expr if_expr;
        AST_index_expr index;
        AST_lit_expr lit;
        AST_match_expr match;
        AST_unary_expr unary;
        AST_while_expr while_expr;
    };
};

struct AST_patt {
    Patt_VT type;
    Token *where;
    union {
        AST_cons_patt cons;
        AST_hash_patt hash;
        AST_list_patt list;
        AST_pair_patt pair;
        char *ident;       /* identifier(variable) pattern */
        int64_t i;         /* literal int pattern */
        double f;          /* literal float pattern */
        char *s;           /* literal string pattern */
        int b;             /* literal boolean value */
    };
};

/* statements sub nodes */
struct AST_expr_stmt {
    AST_expr expr;
};

struct AST_fn_stmt {
    char *name;
    AST_patt *params;  /* array */
    int count;         /* number of parameters */
    AST_piece body;
};

struct AST_let_stmt {
    AST_patt patt;
    AST_expr value;
};

struct AST_ret_stmt {
    AST_expr value;
};

struct AST_variant {
    char *name;     /* the variant name */
    char **params;  /* array */
    int count;      /* number of parameters */
};

struct AST_type_stmt {
    char *name;   /* the type name */
    AST_variant *variants; /* array */
};

/* expressions sub nodes */
struct AST_assign_expr {
    AST_expr lvalue;
    AST_expr value;
};

struct AST_binary_expr {
    TK_type op;
    AST_expr left;
    AST_expr right;
};

struct AST_call_expr {
    AST_expr func;
    AST_expr *args;  /* array */
    int count;       /* number of arguments */
};

struct AST_cond_expr {
    AST_expr *exprs; /* array */
    AST_arm *arms;   /* array */
};

struct AST_for_expr {
    AST_patt patt;
    AST_expr iter;
    AST_piece body;
};

struct AST_group_expr {
    AST_expr expr;
};

struct AST_ident_expr {
    char *name;
    
    /* 
     * index of the variable in the environment array.
     * -1 is an indication of a global variable.
    */
    int index;
    
    /* 
     * number of scopes (distance) from the current scope 
     * to the scope in which the variable is defined.
    */
    int depth;
};

struct AST_elif {
    AST_expr cond;
    AST_piece then;
};

struct AST_if_expr {
    AST_expr cond;
    AST_piece then;
    AST_elif *elifs;    /* array */
    AST_piece alter;
};

struct AST_index_expr {
    AST_expr object;
    AST_expr index;
};

struct AST_lit_expr {
    Lit_Expr_VT type;
    union {
        AST_fn_lit fn;
        AST_hash_lit hash;
        AST_list_lit list;
        double f;      /* float literal */
        int64_t i;     /* integer literal */
        char *s;       /* string literal */
    };
};

struct AST_arm {
    Arm_VT type;
    union {
        AST_expr e;
        AST_piece p;
    };
};

struct AST_match_expr {
    AST_expr value;
    AST_patt *patts; /* array */
    AST_arm *arms;   /* array */
};

struct AST_unary_expr {
    TK_type op;
    AST_expr operand;
};

struct AST_while_expr {
    AST_expr cond;
    AST_piece body;
};

/* literal expressions sub nodes */
struct AST_fn_lit {
    AST_patt *params;   /* array */
    int32_t count;      /* number of parameters */
    AST_piece body;
};

struct AST_hash_lit {
    AST_expr *keys;     /* array */
    AST_expr *values;   /* array */
};

struct AST_list_lit {
    AST_expr *values;   /* array */
};

/* patterns sub nodes */
struct AST_cons_patt {
    AST_expr tag;       /* always ident_expr */
    AST_patt *patts;    /* array */
    int count;          /* number of variants */
};

struct AST_hash_patt {
    AST_expr *keys;     /* array */
    AST_patt *patts;    /* array */
};

struct AST_list_patt {
    AST_patt *patts;    /* array */
    int count;          /* number of patterns */
};

struct AST_pair_patt {
    AST_patt hd;
    AST_patt tl;
};

#endif
