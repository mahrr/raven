/*
 * (debug.c | 12 Mar 19 | Ahmad Maher)
 *
*/

#include <stdio.h>
#include <stdarg.h>

#include "ast.h"
#include "lexer.h"
#include "list.h"

/* string representation of each token type */
char *tok_types_str[] = {
    /* Literals */
    "INT", "FLOAT", "STR",
    "RSTR", "FALSE", "TRUE",
    "NIL",
    
    /* Keywords */
    "FN", "RETURN", "LET", "DO",
    "END", "IF", "ELIF", "ELSE",
    "FOR", "WHILE", "CONTINUE",
    "BREAK", "MATCH", "CASE",
    
    /* Identefier */
    "IDENT",
    
    /* Operators */
    "AND", "OR", "NOT",
    ".", "@", "::",
    
    /* Arthimetik Operators */
    "+", "-", "*", "/", "%",
    
    /* Ordering Operators */
    "<", ">", "==",
    "!=", "<=", ">=",
    
    /* Logic Operators */
    "|", "&", "^",
    "~", ">>", "<<",
    
    /* Delimiters */
    "LPAREN", "RPAREN",
    "LBRACE", "RBRACE",
    "LBRACKET", "RBRACKET",
    "COMMA", "DASH_GT",
    "COLON", "SEMICOLON",
    "EQ", "IN", "NL",
    
    /* Errors and end of file */
    "ERR", "EOF",
};

void print_token(token *t) {
    printf("[%s @line %ld] %.*s (%d) : %s\n",
           t->file,
           t->line,
           t->length,
           t->lexeme == NULL ? "\t" : t->lexeme,
           t->length,
           tok_types_str[t->type]);
}

/* the current identation level for the ast printer */
static int indent_level = -1;
static char *indent = "  ";

static void print_expr(expr*);
static void print_exprs(List_T);
void print_ast(piece*);

static void parenthesize(char *op, int n, ...) {
    va_list ap;
    va_start(ap, n);

    printf("(%s", op);
    expr *e;
    for (int i = 0; i < n; i++) {
        putchar(' ');
        e = va_arg(ap, expr*);
        print_expr(e);
    }
    printf(")");

    va_end(ap);
}

static void paren_stmts(char *op, expr *cond, piece *body) {
    printf("|%s| ", op);
    if (cond != NULL)
        print_expr(cond);
    putchar('\n');
    print_ast(body);
}

static void print_lit_expr(lit_expr *e) {
    switch (e->type) {
    case int_lit_type:
        printf("%ld", e->obj.i_val);
        break;

    case float_lit_type:
        printf("%lf", e->obj.f_val);
        break;

    case str_lit_type:
        printf("'%s'", e->obj.s_val);
        break;

    case rstr_lit_type:
        printf("`%s`", e->obj.s_val);
        break;

    case true_lit_type:
        printf("true");
        break;

    case false_lit_type:
        printf("false");
        break;

    case nil_lit_type:
        printf("nil");
        break;

    default:
        break;
    }
}

static void print_expr(expr *e) {
    switch (e->type) {

    case lit_expr_type:
        print_lit_expr(e->obj.lit_e);
        break;

    case ident_expr_type:
        printf("%s", e->obj.ident);
        break;

    case prefix_expr_type: {
        prefix_expr *pe = e->obj.pre_e;
        parenthesize(tok_types_str[pe->op],
                     1, pe->value);
        break;
    }

    case infix_expr_type: {
        infix_expr *ie = e->obj.inf_e;
        parenthesize(tok_types_str[ie->op],
                     2, ie->left, ie->right);
        break;
    }

    case index_expr_type: {
        index_expr *ie = e->obj.index_e;
        parenthesize("[]", 2, ie->object, ie->index);
        break;
    }

    case group_expr_type: {
        group_expr *ge = e->obj.group_e;
        parenthesize("GR", 1, ge->exp);
        break;
    }

    case call_expr_type: {
        call_expr *ce = e->obj.call_e;
        putchar('(');
        print_expr(ce->func);
        print_exprs(ce->args);
        putchar(')');
        break;
    }

    case if_expr_type: {
        if_expr *ie = e->obj.if_e;
        paren_stmts("if", ie->cond, ie->body);

        if (ie->elifs != NULL) {
            void *obj;
            while ((obj = List_iter(ie->elifs)) != NULL) {
                paren_stmts("elif",
                            ((elif_branch*)obj)->cond,
                            ((elif_branch*)obj)->body);
            }
        }
        
        if (ie->alter != NULL)
            paren_stmts("else", NULL, ie->alter);
        break;
    }

    case for_expr_type: {
        for_expr *fe = e->obj.for_e;
        paren_stmts("for in", fe->iter, fe->body);
        break;
    }

    case while_expr_type: {
        while_expr *we = e->obj.while_e;
        paren_stmts("while", we->cond, we->body);
        break;
    }

    default:
        break;
    }
}

static void print_exprs(List_T exprs) {
    void *obj;
    while ((obj = List_iter(exprs)) != NULL) {
        putchar(' ');
        print_expr(obj);
    }
}

static void print_stmt(stmt *s) {
    switch (s->type) {

    case let_stmt_type: {
        let_stmt *l = s->obj.ls;
        printf("{let %s ", l->name);
        print_expr(l->value);
        printf("}\n");
        break;
    }

    case ret_stmt_type: {
        ret_stmt *r = s->obj.rs;
        printf("{ret ");
        print_expr(r->retval);
        printf("}\n");
        break;
    }

    case expr_stmt_type: {
        expr_stmt *e = s->obj.es;
        printf("{");
        print_expr(e->exp);
        printf("}\n");
        break;
    }

    case fixed_stmt_type:
        printf("{%s}\n", tok_types_str[s->obj.fs]);

    default:
        break;

    }
}

void print_ast(piece *p) {
    List_T t = p->stmts;

    indent_level++;
    void *obj;
    while ((obj = List_iter(t)) != NULL) {
        for (int i = 0; i < indent_level; i++)
            fputs(indent, stdout);
        print_stmt(obj);
    }
    indent_level--;
}
