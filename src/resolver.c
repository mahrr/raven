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
static void
reg_error(Resolver *r, Err_Type type, Token *where, const char *msg) {
    r->been_error = 1;
    Err error = {type, *where, msg};
    ARR_ADD(&r->errors, error);
}

/* add a variable in the innermost scope of the resolver */
static void define(Resolver *r, Token *where, const char *name) {
    /* peek the scope on the top of stack */
    int depth = r->scopes.len - 1;
    Table *scope = r->scopes.elems[depth];

    /* already defined variable */
    if (table_lookup(scope, name)) {
        reg_error(r, SYNTAX_ERR, where, "already defined varaible");
        return;
    }
    
    Var *var = malloc(sizeof(Var));
    var->state = DEFINED;
    var->slot = table_elems(scope);
    
    /* add the variable to the scope */
    table_put(scope, name, var);

    /* add variable to the latest register */
    ARR_ADD(&r->latest, (char*)name);
}


/* register variable 'expr' and its environment steps and slot 
   to the local table of the evaluator if it's found,
   or register an error for not defined variable usage */
static void resolve_local(Resolver *r, AST_expr expr) {
    int inner = r->scopes.len -1;
    char *name = expr->ident;
    
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
    reg_error(r, NAME_ERR, expr->where, "undefined variable usage");
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
        AST_fn_lit fn = lit->fn;
        resolve_fn(r, fn->params, fn->body);
        break;
    }
        
    case HASH_LIT: {
        AST_hash_lit hash = lit->hash;
        for (int i = 0; hash->keys[i]; i++) {
            if (hash->keys[i]->type == EXPR_KEY) {
                AST_key key = hash->keys[i];
                resolve_expr(r, key->expr);
            }
            resolve_expr(r, hash->values[i]);
        }
        break;
    }
        
    case LIST_LIT:
        resolve_exprs(r, lit->list->values);
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
        push_scope(r);
        if (arms[i]->type == EXPR_ARM) {
            resolve_patt(r, patts[i]);
            resolve_expr(r, arms[i]->e);
        } else {
            resolve_patt(r, patts[i]);
            resolve_stmts(r, arms[i]->p->stmts);
        }
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
        
    case CONS_PATT:
        resolve_expr(r, patt->cons->tag);
        resolve_patts(r, patt->cons->patts);
        break;

    case IDENT_PATT:
        define(r, patt->where, patt->ident);
        break;
        
    case HASH_PATT: {
        AST_hash_patt hash = patt->hash;
        for (int i = 0; hash->keys[i]; i++) {
            if (hash->keys[i]->type == EXPR_KEY) {
                AST_key key = hash->keys[i];
                resolve_expr(r, key->expr);
            }
            resolve_patt(r, hash->patts[i]);
        }
        break;
    }
        
    case LIST_PATT:
        resolve_patts(r, patt->list->patts);
        break;

    case PAIR_PATT:
        resolve_patt(r, patt->pair->hd);
        resolve_patt(r, patt->pair->tl);
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
        
    case ASSIGN_EXPR:
        resolve_expr(r, expr->assign->lvalue);
        resolve_expr(r, expr->assign->value);
        break;
        
    case BINARY_EXPR:
        resolve_expr(r, expr->binary->left);
        resolve_expr(r, expr->binary->right);
        break;
        
    case CALL_EXPR:
        resolve_expr(r, expr->call->func);
        resolve_exprs(r, expr->call->args);
        break;

    case COND_EXPR: {
        AST_cond_expr cond = expr->cond;
        resolve_exprs(r, cond->exprs);
        
        AST_arm *arms = cond->arms;
        for (int i = 0; arms[i]; i++) {
            if (arms[i]->type == PIECE_ARM)
                resolve_piece(r, arms[i]->p);
            else
                resolve_expr(r, arms[i]->e);
        }
        break;
    }
        
    case FOR_EXPR: {
        AST_for_expr for_expr = expr->for_expr;
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
        resolve_expr(r, expr->group->expr);
        break;
        
    case IDENT_EXPR:
        resolve_local(r, expr);
        break;
        
    case IF_EXPR:
        resolve_if(r, expr->if_expr);
        break;
        
    case INDEX_EXPR:
        resolve_expr(r, expr->index->object);
        resolve_expr(r, expr->index->index);
        break;
        
    case LIT_EXPR:
        resolve_lit(r, expr->lit);
        break;
        
    case MATCH_EXPR:
        resolve_match(r, expr->match);
        break;
        
    case UNARY_EXPR:
        resolve_expr(r, expr->unary->operand);
        break;
        
    case WHILE_EXPR: {
        unsigned prev_state = r->state;
        r->state |= IN_LOOP;
        resolve_expr(r, expr->while_expr->cond);
        resolve_piece(r, expr->while_expr->body);
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
    /* reset the latest register */
    r->latest.len = 0;
    
    switch (stmt->type) {
    case EXPR_STMT:
        resolve_expr(r, stmt->expr->expr);
        break;
        
    case FIXED_STMT:
        if (!(r->state & IN_LOOP)) {
            char *msg = "'continue/break' usage outside a loop";
            reg_error(r, SYNTAX_ERR, stmt->where, msg);
        }
        break;
        
    case FN_STMT: {
        AST_fn_stmt fn = stmt->fn;
        define(r, stmt->where, fn->name);
        resolve_fn(r, fn->params, fn->body);
        break;
    }

    case LET_STMT:
        resolve_patt(r, stmt->let->patt);
        resolve_expr(r, stmt->let->value);
        break;

    case RET_STMT: {
        if (!(r->state & IN_FUNCTION)) {
            char *msg = "'return' usage outside a function";
            reg_error(r, SYNTAX_ERR, stmt->where, msg);
        }
        
        AST_expr retval = stmt->ret->value;
        if (retval != NULL)
            resolve_expr(r, retval);
        
        break;
    }

    case TYPE_STMT: {
        AST_type_stmt type_stmt = stmt->type_stmt;
        AST_variant *variants = type_stmt->variants;

        for (int i = 0; variants[i]; i++)
            define(r, stmt->where, variants[i]->name);
        
        break;
    }
        
    default:
        fprintf(stderr, "[INTERNAL] invalid stmt type (%d)\n",
                stmt->type);
        assert(0);
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
    ARR_INIT(&r->latest, char*);
    ARR_INIT(&r->errors, Err);

    /* push the global scope */
    push_scope(r);
}

void free_resolver(Resolver *r) {
    /* remove the global scope. */
    pop_scope(r);

    /* free the resolver arrays */
    ARR_FREE(&r->scopes);
    ARR_FREE(&r->latest);
    ARR_FREE(&r->errors);
}

void resolver_recover(Resolver *r) {
    char **defined = r->latest.elems;
    Table *scope = r->scopes.elems[r->scopes.len - 1];
    
    for (int i = 0; i < r->latest.len; i++)
        table_remove(scope, defined[i]);
    
    r->latest.len = 0;
}

int resolve_statement(Resolver *r, AST_stmt s) {
    resolve_stmt(r, s);
    
    /* an error occured and there are an already defined variables */
    if (r->been_error && r->latest.len != 0)
        resolver_recover(r);
    
    return r->been_error;
}

int resolve(Resolver *r, AST_piece piece) {    
    resolve_stmts(r, piece->stmts);
    return r->been_error;
}
