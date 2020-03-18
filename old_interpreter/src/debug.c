/*
 * (debug.c | 12 Mar 19 | Ahmad Maher, Kareem Hamdy)
 *
*/

#include <assert.h>
#include <stdio.h>
#include <stdarg.h>

#include "ast.h"
#include "list.h"
#include "object.h"
#include "token.h"

/** INTERNALS **/

/* string representation of each token type */
char *tok_types_str[] = {
    /* Literals */
    "INT", "FLOAT", "STR", "RSTR",
    "FALSE", "TRUE", "NIL",
    
    /* Keywords */
    "FN", "RETURN", "LET", "TYPE",
    "DO", "END", "IF", "ELIF", "ELSE",
    "FOR", "WHILE", "CONTINUE",
    "BREAK", "COND", "MATCH",
    
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
    
    /* Delimiters */
    "LPAREN", "RPAREN",
    "LBRACE", "RBRACE",
    "LBRACKET", "RBRACKET",
    "COMMA", "DASH_GT",
    "COLON", "SEMICOLON",
    "EQ", "IN", "PIPE",
    "NL",
    
    /* Errors and end of file */
    "ERR", "EOF",
};

/* the current identation level for the ast printer */
static int indent_level = -1;
static char *indent = "  ";

#define INDENT()                             \
    for (int i = 0; i < indent_level; i++)   \
        fputs(indent, stdout);

static void print_expr(AST_expr);
static void print_exprs(AST_expr*);
static void print_patt(AST_patt);
static void print_patts(AST_patt*);

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

    case FN_LIT:
        printf("(fn [");
        print_patts(e->fn->params);
        printf("]\n");
        print_piece(e->fn->body);
        printf(")");
        return;

    case HASH_LIT: {
        AST_hash_lit hash = e->hash;
        printf("(hash ");
        
        AST_expr *keys = hash->keys;
        AST_expr *values = hash->values;
        for (int i = 0; keys[i]; i++) {
            print_expr(keys[i]);
            print_expr(values[i]);
            putchar(' ');
        }
        printf(")");
        return;
    }

    case LIST_LIT:
        printf("(list");
        print_exprs(e->list->values);
        printf(")");
        return;

    case FALSE_LIT:
        printf("false");
        return;
        
    case FLOAT_LIT:
        printf("%f", e->f);
        return;
        
    case INT_LIT:
        printf("%ld", e->i);
        return;

    case NIL_LIT:
        printf("nil");
        return;

    case STR_LIT:
        printf("'%s'", e->s);
        return;

    case TRUE_LIT:
        printf("true");
        return;
    }

    fprintf(stderr, "[INTERNAL] invalid literal type (%d)", e->type);
    assert(0);
}

static void print_patt(AST_patt p) {
    switch (p->type) {
        
    case BOOL_CPATT:
        printf(p->b ? "true" : "false");
        return;
        
    case CONS_PATT:
        printf("(:cons ");
        print_expr(p->cons->tag);
        print_patts(p->cons->patts);
        putchar(')');
        return;

    case HASH_PATT: {
        AST_hash_patt hash = p->hash;
        printf("(:hash ");
        
        AST_expr *keys = hash->keys;
        AST_patt *patts = hash->patts;
        for (int i = 0; keys[i]; i++) {
            print_expr(keys[i]);
            print_patt(patts[i]);
            putchar(' ');
        }
        
        printf(")");
        return;
    }

    case LIST_PATT:
        printf("(:list ");
        print_patts(p->list->patts);
        printf(")");
        return;

    case PAIR_PATT:
        printf("(:pair ");
        print_patt(p->pair->hd);
        printf(" | ");
        print_patt(p->pair->tl);
        printf(")");
        return;
    
    case FLOAT_CPATT:
        printf("%f", p->f);
        return;

    case IDENT_PATT:
        printf("%s", p->ident);
        return;

    case INT_CPATT:
        printf("%ld", p->i);
        return;

    case NIL_CPATT:
        printf("nil");
        return;

    case STR_CPATT:
        printf("'%s'", p->s);
        return;
    }

    fprintf(stderr, "[INTERNAL] invalid pattern type (%d)\n", p->type);
    assert(0);
}

static void print_patts(AST_patt *patts) {
    for (int i = 0; patts[i]; i++) {
        print_patt(patts[i]);
        putchar(' ');
    }
}

static void print_expr(AST_expr e) {
    switch (e->type) {

    case ASSIGN_EXPR: {
        AST_assign_expr assign = e->assign;
        paren_op("=", 2, assign->lvalue, assign->value);
        return;
    }

    case BINARY_EXPR: {
        AST_binary_expr bin = e->binary;
        paren_op(tok_types_str[bin->op],
                     2, bin->left, bin->right);
        return;
    }

    case CALL_EXPR:
        putchar('(');
        print_expr(e->call->func);
        print_exprs(e->call->args);
        putchar(')');
        return;

    case COND_EXPR: {
        printf("|cond| ");
        putchar('\n');

        AST_expr *exprs = e->cond->exprs;
        AST_arm *arms = e->cond->arms;
        
        for (int i = 0; arms[i]; i++) {
            indent_level++;
            INDENT();
            print_expr(exprs[i]);
            printf(" -> ");

            if (arms[i]->type == EXPR_ARM) {
                print_expr(arms[i]->e);
                putchar('\n');
            } else {
                putchar('\n');
                print_piece(arms[i]->p);
            }
           
            indent_level--;
        }
        return;
    }

    case FOR_EXPR:
        printf("|for| ");
        print_patt(e->for_expr->patt);
        printf(" in ");
        print_expr(e->for_expr->iter);
        putchar('\n');
        print_piece(e->for_expr->body);
        return;

    case GROUP_EXPR:
        paren_op("GR", 1, e->group->expr);
        return;

    case IF_EXPR: {
        AST_if_expr if_expr = e->if_expr;
        paren_block("if", if_expr->cond, if_expr->then);

        AST_elif *elifs = if_expr->elifs;
        for (int i = 0; elifs[i]; i++)
            paren_block("elif", elifs[i]->cond, elifs[i]->then);
        
        if (if_expr->alter != NULL)
            paren_block("else", NULL, if_expr->alter);
        return;
    }

    case IDENT_EXPR:
        printf("%s", e->ident->name);
        return;

    case INDEX_EXPR:
        paren_op("[]", 2, e->index->object, e->index->index);
        return;

    case LIT_EXPR:
        print_lit_expr(e->lit);
        return;

    case MATCH_EXPR: {
        AST_match_expr match = e->match;
        printf("|match| ");
        print_expr(match->value);
        putchar('\n');

        AST_patt *patts = match->patts;
        AST_arm *arms = match->arms;
        
        for (int i = 0; arms[i]; i++) {
            indent_level++;
            INDENT();
            print_patt(patts[i]);
            printf(" -> ");

            if (arms[i]->type == EXPR_ARM) {
                print_expr(arms[i]->e);
                putchar('\n');
            } else {
                putchar('\n');
                print_piece(arms[i]->p);
            }
            indent_level--;
        }
                
        return;
    }

    case UNARY_EXPR:
        paren_op(tok_types_str[e->unary->op],
                     1, e->unary->operand);
        return;

    case WHILE_EXPR: {
        AST_while_expr wh = e->while_expr;
        paren_block("|while|", wh->cond, wh->body);
        return;
    }
        
    }

    fprintf(stderr, "[INTERNAL] invalid expression type (%d)\n", e->type);
    assert(0);
}

static void print_exprs(AST_expr *exprs) {
    for (int i = 0; exprs[i]; i++) {
        putchar(' ');
        print_expr(exprs[i]);
        putchar(' ');
    }
}

static void print_variants(AST_variant *variants) {
    for (int i = 0; variants[i]; i++) {
        printf("  (%s:", variants[i]->name);
        for (int j = 0; j < variants[i]->count; j++)
            printf(" %s", variants[i]->params[j]);
        printf(")\n");
    }
}

static void print_stmt(AST_stmt s) {
    switch (s->type) {
        
    case EXPR_STMT:
        printf("{");
        print_expr(s->expr->expr);
        printf("}\n");
        return;
        
    case FN_STMT:
        printf("{fn (%s) [", s->fn->name);
        print_patts(s->fn->params);
        printf("]\n");
        print_piece(s->fn->body);
        printf("}\n");
        return;

    case LET_STMT:
        printf("{let ");
        print_patt(s->let->patt);
        putchar(' ');
        print_expr(s->let->value);
        printf("}\n");
        return;

    case RET_STMT:
        printf("{ret ");
        print_expr(s->ret->value);
        printf("}\n");
        return;

    case TYPE_STMT:
        printf("{type %s\n", s->type_stmt->name);
        print_variants(s->type_stmt->variants);
        printf("}\n");
        return;

    case FIXED_STMT:
        printf("{%s}\n", tok_types_str[s->fixed]);
        return;
    }

    fprintf(stderr, "[INTERNAL] invalid statement type (%d)\n", s->type);
    assert(0);
}

/** INTEFACE **/

void print_token(Token *t) {
    printf("[%s @line %ld] %.*s (%d) : %s\n",
           t->file,
           t->line,
           t->length,
           t->lexeme,
           t->length,
           tok_types_str[t->type]);
}

void print_piece(AST_piece p) {
    indent_level++;
    for (int i = 0; p->stmts[i]; i++) {
        INDENT();
        print_stmt(p->stmts[i]);
    }
    indent_level--;
}
