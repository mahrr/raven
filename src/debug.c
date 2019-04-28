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
    "FALSE", "TRUE", "NIL",
    
    /* Keywords */
    "FN", "RETURN", "LET", "DO",
    "END", "IF", "ELIF", "ELSE",
    "FOR", "WHILE", "CONTINUE",
    "BREAK", "MATCH", "CASE",
    
    /* Identefier */
    "IDENT",
    
    /* Operators */
    "AND", "OR", "NOT",
    ".", "@", "|",
    
    /* Arthimetik Operators */
    "+", "-", "*", "/", "%",
    
    /* Ordering Operators */
    "<", ">", "==",
    "!=", "<=", ">=",
    
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

/* buffer for some strings concatenation */
static char buff[128];

#define IDENT()                             \
    for (int i = 0; i < indent_level; i++)  \
        fputs(indent, stdout);

static void print_expr(AST_expr);
static void print_exprs(List_T);
static void print_patt(AST_patt);
static void print_patts(List_T);

void print_piece(AST_piece);

/* print an operator 'op' with arbitrary number 'n' 
   of expressions in s-expression form */
static void paren_op(char *op, int n, ...) {
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

/* print an operator(or keyword) with it's associated block
   in an s-expression like form */
static void paren_block(char *op, AST_expr cond, AST_piece body) {
    printf("|%s| ", op);
    if (cond != NULL)
        print_expr(cond);
    putchar('\n');
    print_piece(body);
}

static void print_lit_expr(AST_lit_expr e) {
    switch (e->type) {

    case FN_LIT: {
        AST_fn_lit fn = e->obj.fn;
        printf("(fn [");
        print_patts(fn->params);
        printf("]\n");
        print_piece(fn->body);
        printf(")");
        break;
    }

    case HASH_LIT: {
        AST_hash_lit hash = e->obj.hash;
        printf("(hash ");
        
        char *name;
        AST_expr value;
        while ((name = List_iter(hash->names)) != NULL &&
               (value = List_iter(hash->values)) != NULL) {
            printf("%s:", name);
            print_expr(value);
            putchar(' ');
        }
        printf(")");
        break;
    }

    case LIST_LIT: {
        AST_list_lit list = e->obj.list;
        printf("(list");
        print_exprs(list->values);
        printf(")");
        break;
    }

    case FALSE_LIT:
        printf("false");
        break;
        
    case FLOAT_LIT:
        printf("%Lf", e->obj.f);
        break;
        
    case INT_LIT:
        printf("%ld", e->obj.i);
        break;

    case NIL_LIT:
        printf("nil");
        break;

    case STR_LIT:
        printf("'%s'", e->obj.s);
        break;

    case TRUE_LIT:
        printf("true");
        break;
    }
}

static void print_patt(AST_patt p) {
    switch (p->type) {
        
    case HASH_PATT: {
        AST_hash_patt hash = p->obj.hash;
        printf("(:hash ");
        
        char *name;
        AST_patt patt;
        while ((name = List_iter(hash->names)) &&
               (patt = List_iter(hash->patts))) {
            printf("%s : ", name);
            print_patt(patt);
            putchar(' ');
        }
        printf(")");
        break;
    }

    case LIST_PATT: {
        AST_list_patt list = p->obj.list;
        printf("(:list");
        print_patts(list->patts);
        printf(")");
        break;
    }

    case PAIR_PATT: {
        AST_pair_patt pair = p->obj.pair;
        printf("(:pair ");
        print_patt(pair->hd);
        printf(" | ");
        print_patt(pair->tl);
        printf(")");
        break;
    }

    case FALSE_CPATT:
        printf("false");
        break;
    
    case FLOAT_CPATT:
        printf("%Lf", p->obj.f);
        break;

    case IDENT_PATT:
        printf("%s", p->obj.ident);
        break;

    case INT_CPATT:
        printf("%ld", p->obj.i);
        break;

    case NIL_CPATT:
        printf("nil");
        break;

    case STR_CPATT:
        printf("%s", p->obj.s);
        break;

    case TRUE_CPATT:
        printf("true");
        break;
    }
}

static void print_patts(List_T patts) {
    void *patt;
    while ((patt = List_iter(patts))) {
        putchar(' ');
        print_patt(patt);
        putchar(' ');
    }
}

static void print_expr(AST_expr e) {
    switch (e->type) {

    case ACCESS_EXPR: {
        AST_access_expr access = e->obj.access;
        sprintf(buff, ".%s", access->field);
        paren_op(buff, 1, access->object);
        break;
    }

    case ASSIGN_EXPR: {
        AST_assign_expr assign = e->obj.assign;
        paren_op("=", 2, assign->lvalue, assign->value);
        break;
    }

    case BINARY_EXPR: {
        AST_binary_expr bin = e->obj.binary;
        paren_op(tok_types_str[bin->op],
                     2, bin->left, bin->right);
        break;
    }

    case CALL_EXPR: {
        AST_call_expr call = e->obj.call;
        putchar('(');
        print_expr(call->func);
        print_exprs(call->args);
        putchar(')');
        break;
    }

    case FOR_EXPR: {
        AST_for_expr for_expr = e->obj.for_expr;
        paren_block("for in", for_expr->iter, for_expr->body);
        break;
    }

    case GROUP_EXPR: {
        AST_group_expr group = e->obj.group;
        paren_op("GR", 1, group->expr);
        break;
    }

    case IF_EXPR: {
        AST_if_expr if_expr = e->obj.if_expr;
        paren_block("if", if_expr->cond, if_expr->then);

        void *obj;
        while ((obj = List_iter(if_expr->elifs)) != NULL) {
            paren_block("elif",
                        ((AST_elif_branch)obj)->cond,
                        ((AST_elif_branch)obj)->body);
        }
        
        if (if_expr->alter != NULL)
            paren_block("else", NULL, if_expr->alter);
        break;
    }

    case IDENT_EXPR:
        printf("%s", e->obj.ident);
        break;

    case INDEX_EXPR: {
        AST_index_expr ie = e->obj.index;
        paren_op("[]", 2, ie->object, ie->index);
        break;
    }

    case LIT_EXPR:
        print_lit_expr(e->obj.lit);
        break;

    case MATCH_EXPR: {
        AST_match_expr match = e->obj.match;
        printf("(match ");
        print_expr(match->value);
        putchar('\n');

        AST_match_branch branch;
        
        while ((branch = (AST_match_branch)List_iter(match->branches))) {
            indent_level++;
            IDENT();
            printf("#");
            print_patt(branch->patt);
            printf(" -> ");

            if (branch->type == EXPR_MATCH_BRANCH)
                print_expr(branch->obj.e);
            else
                print_piece(branch->obj.p);
            printf("\n");
            indent_level--;
        }
        printf(")");
        
        break;
    }

    case UNARY_EXPR: {
        AST_unary_expr unary = e->obj.unary;
        paren_op(tok_types_str[unary->op],
                     1, unary->operand);
        break;
    }

    case WHILE_EXPR: {
        AST_while_expr wh = e->obj.while_expr;
        paren_block("while", wh->cond, wh->body);
        break;
    }
        
    }
}

static void print_exprs(List_T exprs) {
    void *expr;
    while ((expr = List_iter(exprs))) {
        putchar(' ');
        print_expr(expr);
        putchar(' ');
    }
}

static void print_stmt(AST_stmt s) {
    switch (s->type) {
        
    case EXPR_STMT: {
        AST_expr_stmt stmt = s->obj.expr;
        printf("{");
        print_expr(stmt->expr);
        printf("}\n");
        break;
    }
        
    case FN_STMT: {
        AST_fn_stmt fn = s->obj.fn;
        printf("{fn (%s) [", fn->name);
        print_patts(fn->params);
        printf("]\n");
        print_piece(fn->body);
        printf("}\n");
        break;
    }

    case LET_STMT: {
        AST_let_stmt let = s->obj.let;
        printf("{let ");
        print_patt(let->patt);
        putchar(' ');
        print_expr(let->value);
        printf("}\n");
        break;
    }

    case RET_STMT: {
        AST_ret_stmt ret = s->obj.ret;
        printf("{ret ");
        print_expr(ret->value);
        printf("}\n");
        break;
    }

    case FIXED_STMT:
        printf("{%s}\n", tok_types_str[s->obj.fixed]);
    }
}

void print_piece(AST_piece p) {
    List_T t = p->stmts;

    indent_level++;
    AST_stmt stmt;
    while ((stmt = (AST_stmt)List_iter(t)) != NULL) {
        IDENT();
        print_stmt(stmt);
    }
    indent_level--;
}
