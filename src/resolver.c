/*
 * (resolver.c | 12 May 19 | Ahmad Maher)
 *
 * Resolver Implementation
 *
*/

#include <assert.h>
#include <stdio.h>   /* FILE */

#include "array.h"
#include "ast.h"
#include "env.h"
#include "error.h"
#include "eval.h"
#include "hashing.h"
#include "resolver.h"
#include "table.h"
#include "token.h"


/** TYPES **/

/* resolver state */
enum {
    IN_FUNCTION = 0x01,
    IN_LOOP     = 0x02,
};

/* variable state */
enum {
    DEFINED,
    USED,
};


typedef struct Var {
    int slot;        /* the slot number of the enviroment array */
    unsigned state;  /* the state of a variable (e.g. define, used) */
} Var;

/** INTERNALS **/

static void resolve_piece(Resolver *r, AST_piece piece);
static void resolve_stmts(Resolver *r, AST_stmt *stmts);
static void resolve_exprs(Resolver *r, AST_expr *exprs);
static void resolve_patts(Resolver *r, AST_patt *patts);
static void resolve_expr(Resolver *r, AST_expr expr);
static void resolve_patt(Resolver *r, AST_patt patt);


/* push a new scope the resolver scopes stack */
void push_scope(Resolver *r) {
    Table *scope = malloc(sizeof(Table));
    init_table(scope, 191, hash_str, free, comp_str);

    ARR_ADD(&r->scopes, scope);
}

void pop_scope(Resolver *r) {
    int i = r->scopes.len-1;
    Table *scope = r->scopes.elems[i];
    
    /* free the table internals */
    free_table(scope);

    /* free the table strucutre */
    free(scope);
    
    r->scopes.len--;
}

/* register an error in the resolver error array */
static void reg_error(Resolver *r, Token *where, const char *msg) {
    r->been_error = 1;
    SErr error = {*where, msg};
    ARR_ADD(&r->errors, error);
}

/* add a variable in the innermost scope of the resolver */
static void define(Resolver *r, Token *where, const char *name) {
    /* peek the scope on the top of stack */
    int depth = r->scopes.len - 1;
    Table *scope = r->scopes.elems[depth];

    /* already defined variable */
    if (table_lookup(scope, name)) {
        reg_error(r, where, "already defined varaible");
        return;
    }
    
    Var *var = malloc(sizeof(Var));
    var->state = DEFINED;
    var->slot = table_elems(scope);
    
    /* add the variable to the scope */
    table_put(scope, name, var);
}


/* register variable 'expr' and its environment steps and slot 
   to the local table of the evaluator if it's found,
   or register an error for not defined variable usage */
static void resolve_local(Resolver *r, AST_expr expr) {
    int inner = r->scopes.len -1;
    char *name = expr->obj.ident;
    
    for (int i = inner; i >= 0; i--) {
        /* current outer most scope */
        Table *scope = r->scopes.elems[i];
        Var *var = table_get(scope, name);

        /* check if the scope contains the variable */
        if (var) {
            /* the evaluator variables lookup table */
            Table *eval_vars = r->evaluator->vars;

            /* the index of the variable in the 
               scope where it's defined */
            int slot = var->slot;

            /* array of two ints, contains the current used 
               variable index and the distance from the current 
               scope to the scope in which it's defined */
            int *loc = malloc(sizeof(int) * 2);
            loc[0] = slot;
            loc[1] = inner - i;

            /* put the location array in the evaluator
               variable lookup table */
            table_put(eval_vars, expr, loc);
            return;
        }
    }

    /* the variable is not found */
    reg_error(r, expr->where, "undefined variable usage");
}

/* resolve function body */
static void resolve_fn(Resolver *r, AST_patt *params, AST_piece body) {
    /* the resolver enters the function body */
    unsigned prev_state = r->state;
    r->state |= IN_FUNCTION;
    
    /* push a new scope for the function body */
    push_scope(r);
    
    /* define the parameters(patterns) in the function scope */
    resolve_patts(r, params);
    resolve_stmts(r, body->stmts);
    
    /* pop the function scope */
    pop_scope(r);
    
    /* the resolver exits the function body */
    r->state = prev_state;
}

static void resolve_lit(Resolver *r, AST_lit_expr lit) {
    switch (lit->type) {
        
    case FN_LIT: {
        AST_fn_lit fn = lit->obj.fn;
        resolve_fn(r, fn->params, fn->body);
        break;
    }
        
    case HASH_LIT:
        resolve_exprs(r, lit->obj.hash->values);
        break;
        
    case LIST_LIT:
        resolve_exprs(r, lit->obj.list->values);
        break;
        
    default:
        break;
    }
}

static void resolve_match(Resolver *r, AST_match_expr match) {
    resolve_expr(r, match->value);

    AST_patt *patts = match->patts;
    AST_arm *arms = match->arms;
    
    for (int i = 0; patts[i]; i++) {
        /* push a new scope for the match branch */
        push_scope(r);
        
        resolve_patt(r, patts[i]);
        if (arms[i]->type == EXPR_ARM)
            resolve_expr(r, arms[i]->obj.e);
        else
            resolve_stmts(r, arms[i]->obj.p->stmts);

        /* pop the branch scope */
        pop_scope(r);
    }
}

static void resolve_if(Resolver *r, AST_if_expr if_expr) {
    resolve_expr(r, if_expr->cond);
    resolve_piece(r, if_expr->then);

    AST_elif *elifs = if_expr->elifs;
    for (int i = 0; elifs[i]; i++) {
        resolve_expr(r, elifs[i]->cond);
        resolve_piece(r, elifs[i]->then);
    }
        
    AST_piece alter = if_expr->alter;
    if (alter) resolve_piece(r, alter);
}

static void resolve_patt(Resolver *r, AST_patt patt) {
    switch (patt->type) {
        
    case IDENT_PATT:
        define(r, patt->where, patt->obj.ident);
        break;
        
    case HASH_PATT:
        resolve_patts(r, patt->obj.hash->patts);
        break;
        
    case LIST_PATT:
        resolve_patts(r, patt->obj.list->patts);
        break;

    case PAIR_PATT:
        resolve_patt(r, patt->obj.pair->hd);
        resolve_patt(r, patt->obj.pair->tl);
        break;

    default:
        break;
    }
}

static void resolve_patts(Resolver *r, AST_patt *patts) {
    for (int i = 0; patts[i]; i++)
        resolve_patt(r, patts[i]);
}

static void resolve_expr(Resolver *r, AST_expr expr) {
    switch (expr->type) {
        
    case ACCESS_EXPR:
        resolve_expr(r, expr->obj.access->object);
        break;
        
    case ASSIGN_EXPR:
        resolve_expr(r, expr->obj.assign->lvalue);
        resolve_expr(r, expr->obj.assign->value);
        break;
        
    case BINARY_EXPR:
        resolve_expr(r, expr->obj.binary->left);
        resolve_expr(r, expr->obj.binary->right);
        break;
        
    case CALL_EXPR:
        resolve_expr(r, expr->obj.call->func);
        resolve_exprs(r, expr->obj.call->args);
        break;
        
    case FOR_EXPR: {
        AST_for_expr for_expr = expr->obj.for_expr;
        unsigned prev_state = r->state;
        r->state |= IN_LOOP;
        
        resolve_expr(r, for_expr->iter);
        push_scope(r);
        resolve_patt(r, for_expr->patt);
        resolve_stmts(r, for_expr->body->stmts);
        pop_scope(r);
        
        r->state = prev_state;
        break;
    }
        
    case GROUP_EXPR:
        resolve_expr(r, expr->obj.group->expr);
        break;
        
    case IDENT_EXPR:
        resolve_local(r, expr);
        break;
        
    case IF_EXPR:
        resolve_if(r, expr->obj.if_expr);
        break;
        
    case INDEX_EXPR:
        resolve_expr(r, expr->obj.index->object);
        resolve_expr(r, expr->obj.index->index);
        break;
        
    case LIT_EXPR:
        resolve_lit(r, expr->obj.lit);
        break;
        
    case MATCH_EXPR:
        resolve_match(r, expr->obj.match);
        break;
        
    case UNARY_EXPR:
        resolve_expr(r, expr->obj.unary->operand);
        break;
        
    case WHILE_EXPR: {
        unsigned prev_state = r->state;
        r->state |= IN_LOOP;
        resolve_expr(r, expr->obj.while_expr->cond);
        resolve_piece(r, expr->obj.while_expr->body);
        r->state = prev_state;
        break;
    }

    default:
        assert(0);  /* invalid type */
    }
}

static void resolve_exprs(Resolver *r, AST_expr *exprs) {
    for (int i = 0; exprs[i]; i++)
        resolve_expr(r, exprs[i]);
}

static void resolve_stmt(Resolver *r, AST_stmt stmt) {
    switch (stmt->type) {
        
    case EXPR_STMT:
        resolve_expr(r, stmt->obj.expr->expr);
        break;
        
    case FIXED_STMT:
        if (!(r->state & IN_LOOP)) {
            char *msg = "'continue/break' usage outside a loop";
            reg_error(r, stmt->where, msg);
        }
        break;
        
    case FN_STMT: {
        AST_fn_stmt fn = stmt->obj.fn;
        define(r, stmt->where, fn->name);
        resolve_fn(r, fn->params, fn->body);
        break;
    }

    case LET_STMT:
        resolve_patt(r, stmt->obj.let->patt);
        resolve_expr(r, stmt->obj.let->value);
        break;

    case RET_STMT: {
        if (!(r->state & IN_FUNCTION)) {
            char *msg = "'return' usage outside a function";
            reg_error(r, stmt->where, msg);
        }
        
        AST_expr retval = stmt->obj.ret->value;
        if (retval != NULL)
            resolve_expr(r, retval);
        
        break;
    }

    default:
        assert(0);  /* invalid type */
    }
}

static void resolve_stmts(Resolver *r, AST_stmt *stmts) {
    for (int i = 0; stmts[i]; i++)    
        resolve_stmt(r, stmts[i]);
}

static void resolve_piece(Resolver *r, AST_piece piece) {
    push_scope(r);
    resolve_stmts(r, piece->stmts);
    pop_scope(r);
}

/** INTEFACE **/

void init_resolver(Resolver *r, Evaluator *e) {
    r->evaluator = e;
    r->been_error = 0;
    r->state = 0;
    
    ARR_INIT(&r->scopes, Table*);
    ARR_INIT(&r->errors, SErr);
}

void free_resolver(Resolver *r) {
    /* remove the global scope. */
    pop_scope(r);

    /* free the resolver arrays */
    ARR_FREE(&r->scopes);
    ARR_FREE(&r->errors);
}

int resolve(Resolver *r, AST_piece piece) {
    /* push the global scope. if it's not already
       pushed. the global scope should be persist 
       between resolve calls for the same resolver,
       until the resolver is freed. this is mainly
       useful in repl sessions. */
    if (r->scopes.len == 0) {
        push_scope(r);
    }
    
    resolve_stmts(r, piece->stmts);
    return r->been_error;
}
