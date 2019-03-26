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

static void print_expr(AST_expr);
static void print_exprs(List_T);
void print_ast(AST_piece);

static void parenthesize(char *op, int n, ...) {
    va_list ap;
    va_start(ap, n);

    printf("(%s", op);
    AST_expr e;
    for (int i = 0; i < n; i++) {
        putchar(' ');
        e = va_arg(ap, AST_expr);
        print_expr(e);
    }
    printf(")");

    va_end(ap);
}

static void paren_stmts(char *op, AST_expr cond, AST_piece body) {
    printf("|%s| ", op);
    if (cond != NULL)
        print_expr(cond);
    putchar('\n');
    print_ast(body);
}

static void print_lit_expr(AST_lit_expr e) {
    switch (e->type) {
    case INT_LIT:
        printf("%ld", e->obj.i);
        break;

    case FLOAT_LIT:
        printf("%Lf", e->obj.f);
        break;

    case STR_LIT:
        printf("'%s'", e->obj.s);
        break;

    case RSTR_LIT:
        printf("`%s`", e->obj.s);
        break;

    case TRUE_LIT:
        printf("true");
        break;

    case FALSE_LIT:
        printf("false");
        break;

    case NIL_LIT:
        printf("nil");
        break;

    default:
        break;
    }
}

static void print_expr(AST_expr e) {
    switch (e->type) {

    case LIT_EXPR:
        print_lit_expr(e->obj.lit);
        break;

    case IDENT_EXPR:
        printf("%s", e->obj.ident);
        break;

    case PREFIX_EXPR: {
        AST_prefix_expr pe = e->obj.prefix;
        parenthesize(tok_types_str[pe->op],
                     1, pe->value);
        break;
    }

    case INFIX_EXPR: {
        AST_infix_expr ie = e->obj.infix;
        parenthesize(tok_types_str[ie->op],
                     2, ie->left, ie->right);
        break;
    }

    case INDEX_EXPR: {
        AST_index_expr ie = e->obj.index;
        parenthesize("[]", 2, ie->object, ie->index);
        break;
    }

    case GROUP_EXPR: {
        AST_group_expr ge = e->obj.group;
        parenthesize("GR", 1, ge->exp);
        break;
    }

    case CALL_EXPR: {
        AST_call_expr ce = e->obj.call;
        putchar('(');
        print_expr(ce->func);
        print_exprs(ce->args);
        putchar(')');
        break;
    }

    case IF_EXPR: {
        AST_if_expr ie = e->obj.if_exp;
        paren_stmts("if", ie->cond, ie->body);

        if (ie->elifs != NULL) {
            void *obj;
            while ((obj = List_iter(ie->elifs)) != NULL) {
                paren_stmts("elif",
                            ((AST_elif_branch)obj)->cond,
                            ((AST_elif_branch)obj)->body);
            }
        }
        
        if (ie->alter != NULL)
            paren_stmts("else", NULL, ie->alter);
        break;
    }

    case FOR_EXPR: {
        AST_for_expr fe = e->obj.for_exp;
        paren_stmts("for in", fe->iter, fe->body);
        break;
    }

    case WHILE_EXPR: {
        AST_while_expr we = e->obj.while_exp;
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

static void print_stmt(AST_stmt s) {
    switch (s->type) {

    case LET_STMT: {
        AST_let_stmt l = s->obj.let;
        printf("{let %s ", l->name);
        print_expr(l->value);
        printf("}\n");
        break;
    }

    case RET_STMT: {
        AST_ret_stmt r = s->obj.ret;
        printf("{ret ");
        print_expr(r->value);
        printf("}\n");
        break;
    }

    case EXPR_STMT: {
        AST_expr_stmt e = s->obj.expr;
        printf("{");
        print_expr(e->expr);
        printf("}\n");
        break;
    }

    case FIXED_STMT:
        printf("{%s}\n", tok_types_str[s->obj.fixed]);

    default:
        break;

    }
}

void print_ast(AST_piece p) {
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
