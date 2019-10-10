/*
 * (eval.c | 26 April 19 | Kareem Hamdy, Amr Anwar)
 *
 * Raven syntax tree interpreter implementation.
 *
 */

#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>  /* strcmp */

#include "ast.h"
#include "builtin.h"
#include "env.h"
#include "eval.h"
#include "hashing.h"
#include "list.h"
#include "object.h"
#include "strutil.h"
#include "table.h"

/*** INTERNALS ***/

/** Constants **/

Rav_obj False_obj = {BOOL_OBJ, 0, {.b = 0}};
Rav_obj True_obj  = {BOOL_OBJ, 0, {.b = 1}};
Rav_obj Nil_obj   = {NIL_OBJ,  0, {0}};
Rav_obj Void_obj  = {VOID_OBJ, 0, {0}};

#define RTrue  (Rav_obj *)(&True_obj)
#define RFalse (Rav_obj *)(&False_obj)
#define RNil   (Rav_obj *)(&Nil_obj)
#define RVoid  (Rav_obj *)(&Void_obj)

/* status bits for object mode */
#define From_Return   0x01
#define From_Break    0x02
#define From_Continue 0x04

/* Object predicate for truthness */
static int is_true(Rav_obj *o) {
    switch (o->type) {
    case NIL_OBJ:
        return 0;
    case BOOL_OBJ:
        return o->b;
    default:
        return 1;
    }
}

/* define an object on a local environment 'env' if it's not NULL,
   otherwise it defines the object on the global environemt */
static void define(Evaluator *e, char *name, Rav_obj *object, Env *env) {
    /* not on the global environmet */
    if (env) {
        env_add(env, object);
        return;
    }

    table_put(e->global, name, object);
}

/** Constructors Functions **/

static Rav_obj *clos_object(Evaluator *e, AST_fn_lit expr) {
    Closure_obj *clos = malloc(sizeof (*clos));
    clos->body = expr->body;
    clos->env = e->current;
    clos->params = expr->params;
    clos->arity = expr->count;
    
    Rav_obj *result = new_object(CLOS_OBJ, 0);
    result->cl = clos;
    return result;
}

static Rav_obj *cons_object(char *type, char *name, int8_t arity) {
    Cons_obj *cons = malloc(sizeof (*cons));
    cons->type = type;
    cons->name = name;
    cons->arity = arity;
    
    Rav_obj *result = new_object(CONS_OBJ, 0);
    result->cn = cons;
    return result;
}

static Rav_obj *float_object(double value) {
    Rav_obj *result = new_object(FLOAT_OBJ, 0);
    result->f = value;
    return result;
}

static Rav_obj *list_object(Evaluator *e, AST_expr *values) {
    List *list_objs = NULL;

    for (int i = 0 ; values[i]; i++)
        list_objs = list_add(list_objs, eval(e, values[i]));
    
    Rav_obj *result = new_object(LIST_OBJ, 0);
    result->l = list_objs;
    
    return result;
}

static Rav_obj *int_object(int64_t value) {
    Rav_obj *result = new_object(INT_OBJ, 0);
    result->i = value;
    return result;
}

static Rav_obj *str_object(char *value) {
    Rav_obj *result = new_object(STR_OBJ, 0);
    result->s = value;
    return result;
}

static Rav_obj*
variant_object(Rav_obj *cons, int8_t count, Rav_obj **elems) {
    Variant_obj *variant = malloc(sizeof (*variant));
    variant->cons = cons;
    variant->count = count;
    variant->elems = elems;

    Rav_obj *result = new_object(VARI_OBJ, 0);
    result->vr = variant;
    return result;
}

static void hash_add_float(Hash_obj *hash, double k, Rav_obj *data) {
    double *key = malloc(sizeof (double));
    *key = (double)k;

    if (hash->float_table == NULL) {
        hash->float_table = malloc(sizeof (Table));
        init_table(hash->float_table, 191, hash_float, free, comp_float);
    }

    /* if the table has already a value associated with that key,
       free the newly allocated key */
    if (table_put(hash->float_table, key, data) != NULL)
        free(key);
}

static void hash_add_int(Hash_obj *hash, int k, Rav_obj *data) {
    uint64_t *key = malloc(sizeof (uint64_t));
    *key = k;
    
    if (hash->int_table == NULL) {
        hash->int_table = malloc(sizeof (Table));
        init_table(hash->int_table, 191, hash_int, free, comp_int);
    }

    /* if the table has already a value associated with that key,
       free the newly allocated key */
    if (table_put(hash->int_table, key, data) != NULL)
        free(key);
}

static void hash_add_sym(Hash_obj *hash, char *k, Rav_obj *data) {
        if (hash->str_table == NULL) {
        hash->str_table = malloc(sizeof (Table));
        init_table(hash->str_table, 191, hash_str, free, comp_str);
    }
    table_put(hash->str_table, k, data);
}

static void hash_add_obj(Hash_obj *hash, Rav_obj *k, Rav_obj *data) {
    if (hash->obj_table == NULL) {
        hash->obj_table = malloc(sizeof(Table));
        init_table(hash->obj_table, 191, hash_ptr, free, comp_ptr);
    }
    table_put(hash->obj_table, k, data);
}

/* evaluate a key expression and adds object 'data' to a hash
   object using that key */
static void
hash_add(Evaluator *e, Hash_obj *hash, AST_expr key, Rav_obj *data) {
    Rav_obj *key_obj = eval(e, key);
    
    switch (key_obj->type) {
    case STR_OBJ:
        hash_add_sym(hash, key_obj->s, data);
        break;
    case INT_OBJ:
        hash_add_int(hash, key_obj->i, data);
        break;
    case FLOAT_OBJ:
        hash_add_float(hash, key_obj->f, data);
        break;
        
        /* pointer hashing for bool literals, nil, lists and hashes */
    default:
        hash_add_obj(hash, key_obj, data);
        break;
    }
}

static Rav_obj *hash_object(Evaluator *e, AST_hash_lit hash_lit) {
    Hash_obj *hash_obj = malloc(sizeof (Hash_obj));
    AST_key *keys = hash_lit->keys;
    AST_expr *exprs = hash_lit->values;
    
    for (int i = 0; keys[i]; i++) {
        if (keys[i]->type == SYMBOL_KEY) {
            Rav_obj *object = eval(e, exprs[i]);
            hash_add_sym(hash_obj, keys[i]->symbol, object);
        } else if (keys[i]->type == EXPR_KEY) {
            Rav_obj *object = eval(e, exprs[i]);
            hash_add(e, hash_obj, keys[i]->expr, object);
        } else {
            fprintf(stderr, "[INTERNAL] invalid key type (%d)\n",
                    keys[i]->type);
            assert(0);
        }
    }
    
    Rav_obj *result = new_object(HASH_OBJ, 0);
    result->h = hash_obj;
    return result;
}


/** Object Operations **/

/* Binary Operations */

static double float_of(Rav_obj *obj) {
    if (obj->type == FLOAT_OBJ) return obj->f;
    return (double)obj->i;
}

static Rav_obj* calc_bin_float(double l, double r, TK_type op) {
    switch (op) {
        /* logic operations */
    case TK_GT:
        return l > r ? RTrue : RFalse;
    case TK_GT_EQ:
        return l >= r ? RTrue : RFalse;
    case TK_LT:
        return l < r ? RTrue : RFalse;
    case TK_LT_EQ:
        return l <= r ? RTrue : RFalse;
    case TK_EQ_EQ:
        return l == r ? RTrue : RFalse;
    case TK_BANG_EQ:
        return l != r ? RTrue : RFalse;
        /* arithemtic operation */
    case TK_PLUS:
        return float_object(l + r);
    case TK_MINUS:
        return float_object(l - r);
    case TK_ASTERISK:
        return float_object(l * r);
    case TK_SLASH:
        return float_object(l / r);
    case TK_PERCENT:
        return float_object(fmod(l, r));
    default:
        /* ERROR */
        return float_object(0);
    }
}

static Rav_obj *calc_bin_int(int64_t l, int64_t r, TK_type op) {
    switch (op) {
        /* logic operations */
    case TK_GT:
        return l > r ? RTrue : RFalse;
    case TK_GT_EQ:
        return l >= r ? RTrue : RFalse;
    case TK_LT:
        return l < r ? RTrue : RFalse;
    case TK_LT_EQ:
        return l <= r ? RTrue : RFalse;
    case TK_EQ_EQ:
        return l == r ? RTrue : RFalse;
    case TK_BANG_EQ:
        return l != r ? RTrue : RFalse;
        /* arithemtic operations */
    case TK_PLUS:
        return int_object(l + r);
    case TK_MINUS:
        return int_object(l - r);
    case TK_ASTERISK:
        return int_object(l * r);
    case TK_SLASH:
        return int_object(l / r);
    case TK_PERCENT:
        return int_object(l % r);
    default:
        /*ERROR */
        return int_object(0);
    }
}

static Rav_obj *arth_bin(Rav_obj *left, Rav_obj *right, TK_type op) {
    /* left and right values */
    double r_fval, l_fval;
    int64_t r_ival, l_ival;
    
    int is_float = 0; /* flag for floating point operation */

    if ((left->type == FLOAT_OBJ && right->type == FLOAT_OBJ) ||
        (left->type == FLOAT_OBJ && right->type == INT_OBJ) ||
        (left->type == INT_OBJ && right->type == FLOAT_OBJ)) {
        is_float = 1;
        l_fval = float_of(left);
        r_fval = float_of(right);
    } else if (left->type == INT_OBJ && right->type == INT_OBJ) {
        is_float = 0;
        l_ival = left->i;
        r_ival = right->i;
    } else {
        // TODO: runtime error handeling
        fprintf(stderr, "Error: arithmetics to non-numerical operands\n");
        return RVoid;
    }

    /* check for zero division */
    if ((op == TK_SLASH || op == TK_PERCENT) &&
        float_of(right) == 0.0) {
        // TODO: runtime error handeling
        fprintf(stderr, "Error: Zero divisor\n");
        return RVoid;
    }

    return is_float ? calc_bin_float(l_fval, r_fval, op)
                    : calc_bin_int(l_ival, r_ival, op);
}

/* Equality Check */
static Rav_obj *check_equality(Rav_obj *left, Rav_obj *right) {
    /* there is no implicit type conversion in raven */
    if (left->type != right->type)
        return RFalse;

    switch (left->type) {
    case FLOAT_OBJ:
        return left->f == right->f ? RTrue : RFalse;
    case INT_OBJ:
        return left->i == right->i ? RTrue : RFalse;
    case STR_OBJ:
        return !strcmp(left->s, right->s) ? RTrue : RFalse;

    /* any other object are compared by their addresses,
       so it needs to be the same object to pass the
       check. for element to element check in the
       collection object (e.g., lists and hashes),
       compare functions from the standard library
       could be used. */
    default:
        return left == right ? RTrue : RFalse;
    }
}

/* List Operations */

static Rav_obj *list_cons(Rav_obj *left, Rav_obj *right) {
    if (right->type != LIST_OBJ) {
        // TODO: runtime error handling
        fprintf(stderr, "Error: Cons to an object that is not a list\n");
        return RNil;
    }

    List *l = list_push(right->l, left);
    Rav_obj *list = new_object(LIST_OBJ, 0);
    list->l = l;

    return list;
}

static void *shallow_copy(void *r) { return r; }

static Rav_obj *list_concat(Rav_obj *left, Rav_obj *right) {
    if (left->type != LIST_OBJ || right->type != LIST_OBJ) {
        // TODO: runtime error handling
        fprintf(stderr, "Error: Concat operands are not list objects\n");
        return RVoid;
    }
    
    List *new_list = NULL;
    new_list = list_copy(left->l, shallow_copy);
    new_list = list_append(new_list, right->l);

    Rav_obj *result = new_object(LIST_OBJ, 0);
    result->l = new_list;
    return result;
}

/* Hash Operations */

Rav_obj *hash_get(Rav_obj *hash, Rav_obj *index) {
    Rav_obj *result = RNil;
    
    switch (index->type) {
    case FLOAT_OBJ:
        if (hash->h->float_table)
            result = table_get(hash->h->float_table, &(index->f));
        break;
    case INT_OBJ:
        if (hash->h->int_table)
            result = table_get(hash->h->int_table, &(index->i));
        break;
    case STR_OBJ:
        if (hash->h->str_table)
            result = table_get(hash->h->str_table, index->s);
        break;
    default:
        if (hash->h->obj_table)
            result = table_get(hash->h->obj_table, index);
        break;
    }

    return result;
}

/** Pattern Matching Functions **/

/* check if a pattern and an object have a compatible 
   types for matching. */
static int same_types(AST_patt patt, Rav_obj *object) {
    return ((patt->type == IDENT_PATT) ||
            (patt->type == BOOL_CPATT && object->type == BOOL_OBJ) ||
            (patt->type == CONS_PATT && object->type == VARI_OBJ)  ||
            (patt->type == FLOAT_CPATT && object->type == FLOAT_OBJ) ||
            (patt->type == HASH_PATT && object->type == HASH_OBJ) ||
            (patt->type == LIST_PATT && object->type == LIST_OBJ) ||
            (patt->type == PAIR_PATT && object->type == LIST_OBJ) ||
            (patt->type == INT_CPATT && object->type == INT_OBJ) ||
            (patt->type == NIL_CPATT && object->type == NIL_OBJ) ||
            (patt->type == STR_CPATT && object->type == STR_OBJ));
}

static int
match(Evaluator *e, AST_patt patt, Rav_obj *object, Env *env);

static int
match_cons(Evaluator *e, AST_patt patt, Rav_obj *variant, Env *env) {
    AST_patt *patts = patt->cons->patts;
    Rav_obj **elems = variant->vr->elems;

    Rav_obj *cons = eval(e, patt->cons->tag);
    /* check if it's the same constructor */
    if (variant->vr->cons != cons)
        return 0;

    /* check for correct number of parameter patterns */
    if (patt->cons->count != variant->vr->count)
        return 0;

    for (int i = 0; patts[i]; i++) {
        if (!match(e, patts[i], elems[i], env))
            return 0;
    }

    return 1;
}

static int
match_list(Evaluator *e, AST_patt patt, Rav_obj *list, Env *env) {
    AST_patt *patts = patt->list->patts;
    List *obj_list = list->l;

    /* check length first */
    if (patt->list->count != list_len(obj_list))
        return 0;

    for (int i = 0; patts[i]; i++) {
        if (!match(e, patts[i], obj_list->head, env))
            return 0;
        obj_list = obj_list->tail;
    }

    return 1;
}

static int
match_pair(Evaluator *e, AST_patt patt, Rav_obj *list_obj, Env *env) {
    AST_patt hd = patt->pair->hd;
    AST_patt tl = patt->pair->tl;
    List *list = list_obj->l;

    if (list_len(list) == 0)
        return 0;

    if (!match(e, hd, list->head, env))
        return 0;

    Rav_obj *tail_list = new_object(LIST_OBJ, 0);
    tail_list->l = list_obj->l->tail;
    if (!match(e, tl, tail_list, env))
        return 0;

    return 1;
}

static int
match_hash(Evaluator *e, AST_patt patt, Rav_obj *hash, Env *env) {
    AST_patt *patts = patt->hash->patts;
    AST_key *keys = patt->hash->keys;

    for (int i = 0; keys[i]; i++) {
        Rav_obj *obj;
        if (keys[i]->type == EXPR_KEY) {
            Rav_obj *index = eval(e, keys[i]->expr);
            obj = hash_get(hash, index);
        } else if (keys[i]->type == SYMBOL_KEY) {
            obj = table_get(hash->h->str_table, keys[i]->symbol);
        } else {
            fprintf(stderr, "[INTERNAL] invalid key type (%d)\n",
                    keys[i]->type);
            assert(0);
        }

        /* key not found in the hash */
        if (obj == NULL) return 0;

        if (!match(e, patts[i], obj, env))
            return 0;
    }
    
    return 1;
}

static int
match(Evaluator *e, AST_patt patt, Rav_obj *object, Env *env) {
    if (!same_types(patt, object))
        return 0;

    switch (patt->type) {
    /* constants patterns, compare against values */
    case BOOL_CPATT:
        return patt->b == object->b;
    case FLOAT_CPATT:
        return patt->f == object->f;
    case INT_CPATT:
        return patt->i == object->i;
    case STR_CPATT:
        return !strcmp(patt->s, object->s);
    case NIL_CPATT:
        return 1;

    /* identifier pattern matches against any value.
       it then adds the value to the environment. */
    case IDENT_PATT:
        define(e, patt->ident, object, env);
        return 1;
        
    /* recursives patterns */
    case CONS_PATT:
        return match_cons(e, patt, object, env);
        
    case LIST_PATT:
        return match_list(e, patt, object, env);

    case PAIR_PATT:
        return match_pair(e, patt, object, env);

    case HASH_PATT:
        return match_hash(e, patt, object, env);
    }
    
    fprintf(stderr, "[INTERNAL] invalid pattern type (%d)\n", patt->type);
    assert(0);
}

/** Evaluating Expressions Nodes **/

static Rav_obj *walk_piece(Evaluator *e, AST_piece piece, Env *env_new);

static Rav_obj *eval_assign(Evaluator *e, AST_assign_expr expr) {
    Rav_obj *value = eval(e, expr->value);
        
    if (expr->lvalue->type == IDENT_EXPR) {
        /* check the locals environments first */
        int *loc = table_get(e->vars, expr->lvalue);
        if (loc != NULL) {
            env_set(e->current, value, loc[1], loc[0]);
        } else {
            char *name = expr->lvalue->ident;
            /* check the global environment */
            if (table_lookup(e->global, name))
                table_put(e->global, name, value);
            else {
                //TODO: runtime error handling
                fprintf(stderr, "Error: assign to a not defined name\n");
                return RVoid;
            }
        }
        return value;
    }
    
    /* INDEX_EXPR */
    Rav_obj *obj = eval(e, expr->lvalue->index->object);
    if (obj->type != HASH_OBJ) {
        // TODO: runtime error handeling
        fprintf(stderr, "Error: index operation for non-hash type\n");
        return RVoid;
    }
    hash_add(e, obj->h, expr->lvalue->index->index, value);
    return value;
}

static Rav_obj *eval_binary(Evaluator *e, AST_binary_expr expr) {
    Rav_obj *left;
    Rav_obj *right;
    
    switch (expr->op) {
    case TK_AND:
        left = eval(e, expr->left);
        if (!is_true(left)) return left;
        return eval(e, expr->right);
        
    case TK_OR:
        left = eval(e, expr->left);
        if (is_true(left)) return left;
        return eval(e, expr->right);

    /* list concatentation */
    case TK_AT:
        left = eval(e, expr->left);
        right = eval(e, expr->right);
        return list_concat(left, right);

    /* list cons */
    case TK_PIPE:
        left = eval(e, expr->left);
        right = eval(e, expr->right);
        return list_cons(left, right);

    /* equality */
    case TK_EQ_EQ:
        left = eval(e, expr->left);
        right = eval(e, expr->right);
        return check_equality(left, right);

    case TK_BANG_EQ:
        left = eval(e, expr->left);
        right = eval(e, expr->right);
        return check_equality(left, right) == RTrue ? RFalse : RTrue;
        
    /* arithemtic operators */
    default:
        left = eval(e, expr->left);
        right = eval(e, expr->right);
        return arth_bin(left, right, expr->op);
    }
}

static Rav_obj*
call_builtin(Evaluator *e, Rav_obj *fn, AST_call_expr call) {
    AST_expr *args = call->args;
    int args_num = call->count;
    int arity = fn->bl->arity;

    if (arity != -1 && arity != args_num) {
        //TODO: runtime error handeling
        fprintf(stderr, "Error: Fucntion arity mismatch\n");
        return RVoid;
    }

    Rav_obj **args_objs = NULL;

    /* if not a zero parameters function, allocate
       space for the arguments objects pointers. */    
    if (args_num != 0)
        args_objs = malloc(sizeof (*args_objs) * args_num);

    for (int i = 0; args[i]; i++) {
        Rav_obj *arg = eval(e, args[i]);
        args_objs[i] = arg;
    }

    if (args_objs)
        args_objs[args_num] = NULL;

    Rav_obj *result = fn->bl->fn(args_objs);
    free(args_objs);

    return result;
}

static Rav_obj *call_fn(Evaluator *e, Rav_obj *fn, AST_call_expr call) {
    AST_expr *args = call->args;
    int args_num = call->count;
    
    AST_patt *params = fn->cl->params;
    int arity = fn->cl->arity;

    if (args_num != arity) {
        //TODO: runtime error handeling
        fprintf(stderr, "Error: Fucntion arity mismatch\n");
        return RVoid;
    }
    
    Env *env_new = new_env(fn->cl->env);
    for (int i = 0; args[i]; i++) {
        Rav_obj *arg = eval(e, args[i]);
        if (!match(e, params[i], arg, env_new)) {
            // TODO: runtime error handling
            fprintf(stderr, "Error: Argument pattern mismatch\n");
        }
    }

    Rav_obj *res = walk_piece(e, fn->cl->body, env_new);
    if (res->mode & From_Return) {
        res->mode &= ~From_Return;
        return res;
    }
    
    return res;
}

static Rav_obj*
call_cons(Evaluator *e, Rav_obj *cons, AST_call_expr call) {
    AST_expr *args = call->args;
    int args_num = call->count;

    if (cons->cn->arity != args_num) {
        //TODO: runtime error handeling
        fprintf(stderr, "Error: Constructor arity mismatch\n");
        return RVoid;
    }

    Rav_obj **elems = malloc(sizeof (*elems) * args_num);
    for (int i = 0; args[i]; i++) {
        Rav_obj *arg = eval(e, args[i]);
        elems[i] = arg;
    }

    Rav_obj *variant = variant_object(cons, args_num, elems);
    return variant;
}

static Rav_obj *eval_call(Evaluator *e, AST_call_expr call) {
    Rav_obj *callee = eval(e, call->func);
    
    if (callee->type != BLTIN_OBJ &&
        callee->type != CLOS_OBJ && callee->type != CONS_OBJ) {
        //TODO: runtime error handeling
        fprintf(stderr, "Error: Call a non-callable object\n");
        return RVoid;
    }

    if (callee->type == BLTIN_OBJ)
        return call_builtin(e, callee, call);

        
    if (callee->type == CLOS_OBJ)
        return call_fn(e, callee, call);

    return call_cons(e, callee, call);
}

static Rav_obj *eval_cond_arm(Evaluator *e, AST_arm arm) {
    if (arm->type == EXPR_ARM)
        return eval(e, arm->e);

    return walk_piece(e, arm->p, new_env(e->current));
}

static Rav_obj *eval_cond(Evaluator *e, AST_cond_expr cond) {
    for (int i = 0; cond->exprs[i]; i++) {
        Rav_obj *obj = eval(e, cond->exprs[i]);
        if (is_true(obj))
            return eval_cond_arm(e, cond->arms[i]);
    }

    /* no condition evaluates to true */
    return RNil;
}

/* return a raven object represent a given table key */
static Rav_obj *key_object(Rav_type type, const void *key) {
    switch (type) {
    case FLOAT_OBJ: {
        Rav_obj *key_obj = new_object(FLOAT_OBJ, 0);
        key_obj->f = *((float*)key);
        return key_obj;
    }
    case INT_OBJ: {
        Rav_obj *key_obj = new_object(INT_OBJ, 0);
        key_obj->i = *((int*)key);
        return key_obj;
    }
    case STR_OBJ: {
        Rav_obj *key_obj = new_object(STR_OBJ, 0);
        key_obj->s = (char*)key;
        return key_obj;
    }
        /* the key itself is a raven object */
    default:
        return (Rav_obj*)key;
    }
}

static void
iter_table(Evaluator *e, Table *table, Rav_type key_type,
           AST_for_expr for_expr) {

    if (table == NULL) return;
    
    for (int i = 0; i < table->indexes.len; i++) {
        int index = table->indexes.elems[i];
        Entry *entry = table->entries[index];

        for ( ; entry; entry = entry->link) {
            /* building an two element list (key, value) */
            Rav_obj *object = new_object(LIST_OBJ, 0);

            /* add the raven object that represent the key */
            Rav_obj *key = key_object(key_type, entry->elem->key);
            object->l = list_add(NULL, key);

            /* add the raven object that represent the value */
            object->l = list_add(object->l, entry->elem->data);
            
            Env *env = new_env(e->current);
            if (match(e, for_expr->patt, object, env))
                walk_piece(e, for_expr->body, env);
            else {
                free_env(env);
                free_list(&object->l, free);
                free(object);
                return;
            }
        }
    }
    
}

static void
iter_hash(Evaluator *e, Rav_obj *iter, AST_for_expr for_expr) {
    Hash_obj *hash = iter->h;

    //iter_array(e, hash->array, for_expr);
    //iter_table(e, hash->float_table, FLOAT_OBJ, for_expr);
    //iter_table(e, hash->int_table, INT_OBJ, for_expr);
    iter_table(e, hash->str_table, STR_OBJ, for_expr);
    iter_table(e, hash->obj_table, HASH_OBJ, for_expr);
}

static void
iter_list(Evaluator *e, Rav_obj *iter, AST_for_expr for_expr) {
    List *list = iter->l;
    
    for ( ; list; list = list->tail) {
        Env *env = new_env(e->current);
        if (match(e, for_expr->patt, list->head, env))
            walk_piece(e, for_expr->body, env);
        else {
            free_env(env);
            return;
        }
    }
}

static void
iter_str(Evaluator *e, Rav_obj *iter, AST_for_expr for_expr) {
    char *str = iter->s;
    
    for ( ; *str != '\0'; str++) {
        Rav_obj *ch = str_object(strndup(str, 1));
        Env *env = new_env(e->current);

        if (match(e, for_expr->patt, ch, env))
            walk_piece(e, for_expr->body, env);
        else {
            free_env(env);
            free(ch->s);
            free(ch);
            return;
        }
    }
}

static Rav_obj *eval_for(Evaluator *e, AST_for_expr for_expr) {
    Rav_obj *iter = eval(e, for_expr->iter);

    switch (iter->type) {
    case HASH_OBJ:
        iter_hash(e, iter, for_expr);
        break;
    case LIST_OBJ:
        iter_list(e, iter, for_expr);
        break;
    case STR_OBJ:
        iter_str(e, iter, for_expr);
        break;
    default:
        fprintf(stderr, "non-iterable object in for\n");
        return RVoid;
    }

    return RNil;
}

static Rav_obj *eval_ident(Evaluator *e, AST_expr expr) {
    /* check if it's a local first */
    int *loc = table_get(e->vars, expr);
    if (loc != NULL)
        return env_get(e->current, loc[1], loc[0]);

    /* check if it's a global */
    Rav_obj *value = table_get(e->global, expr->ident);
    if (value != NULL)
        return value;

    // TODO: runtime 
    fprintf(stderr, "Name Error '%s': undefined variable usage.\n",
            expr->ident);
    return RVoid;
}

static Rav_obj *eval_if(Evaluator *e, AST_if_expr if_expr) {
    Rav_obj *cond = eval(e, if_expr->cond);

    /* then clause */
    if (is_true(cond))
        return walk_piece(e, if_expr->then, new_env(e->current));

    /* elif clauses */
    AST_elif *elif = if_expr->elifs;
    for (int i = 0; elif[i]; i++) {
        cond = eval(e, elif[i]->cond);
        if (is_true(cond))
            return walk_piece(e, elif[i]->then, new_env(e->current));
    }
    
    /* else clause */
    if (if_expr->alter != NULL)
        return walk_piece(e, if_expr->alter, new_env(e->current));

    /* all conditions is false, and there is no else */
    return RNil;
}

static Rav_obj *eval_index(Evaluator *e, AST_index_expr expr) {
    Rav_obj *obj = eval(e, expr->object);
    if (obj->type != HASH_OBJ) {
        fprintf(stderr, "Error: index a non-hash object\n");
        return RVoid;
    }
    
    Rav_obj *index = eval(e, expr->index);
    Rav_obj *value = hash_get(obj, index);
    if (value == NULL)
        return RNil;
    return value;
}

static Rav_obj *eval_lit(Evaluator *e, AST_lit_expr lit) {
    switch (lit->type) {
    case INT_LIT:
        return int_object(lit->i);
    case FLOAT_LIT:
        return float_object(lit->f);
    case TRUE_LIT:
        return RTrue;
    case FALSE_LIT:
        return RFalse;
    case NIL_LIT:
        return RNil;
    case STR_LIT:
    case RSTR_LIT:  //TODO
        return str_object(lit->s);
    case LIST_LIT:
        return list_object(e, lit->list->values);
    case FN_LIT:
        return clos_object(e, lit->fn);
    case HASH_LIT:
        return hash_object(e, lit->hash);
    }
    
    fprintf(stderr, "[INTERNAL] invalid lit_expr type (%d)\n", lit->type);
    assert(0);
}

/* evaluate a match arm on specified envirnoment */
static Rav_obj *eval_match_arm(Evaluator *e, AST_arm arm, Env *env) {
    if (arm->type == EXPR_ARM) {
        Rav_obj *res = eval(e, arm->e);
        return res;
    }

    return walk_piece(e, arm->p, env);
}

static Rav_obj *eval_match(Evaluator *e, AST_match_expr match_expr) {
    AST_patt *patts = match_expr->patts;
    AST_arm *arms = match_expr->arms;
    Rav_obj *value = eval(e, match_expr->value);
    
    for (int i = 0; patts[i]; i++) {
        Env *env = new_env(e->current);
        Env *prev = e->current;
        e->current = env;
        if (match(e, patts[i], value, env)) {
            return eval_match_arm(e, arms[i], env);
        }
        free(env);
        e->current = prev;
    }

    /* no match occurs */
    return RNil;
}

static Rav_obj *eval_unary(Evaluator *e, AST_unary_expr unary) {
    Rav_obj *operand = eval(e, unary->operand);
    
    switch (unary->op) {
    case TK_MINUS:
        if (operand->type == FLOAT_OBJ) {
            return float_object(-operand->f);
        } else if (operand->type == INT_OBJ) {
            return int_object(-operand->i);
        } else {
            fprintf(stderr, "Error: apply (-) to non-numeric object\n");
            return RVoid;
        }
    case TK_NOT:
        return is_true(operand) ? RFalse : RTrue;
        
    default:
        fprintf(stderr, "[INTERNAL] invalid unary->op (%d)\n", unary->op);
        assert(0);
    }
}

static Rav_obj *eval_while(Evaluator *e, AST_while_expr expr) {
    Rav_obj *res = RNil; /* by default */
    
    while (is_true(eval(e, expr->cond))) {
        res = walk_piece(e, expr->body, new_env(e->current));
        
        if (res->mode & From_Break) {
            res->mode &= ~From_Break;
            return res;
        } else if (res->mode & From_Continue) {
            res->mode &= ~From_Continue;
        } else if (res->mode & From_Return)
            return res;
    }
    
    return res;
}

/** Executing Statements Nodes **/

static void def_function(Evaluator *e, AST_fn_stmt fn_stmt) {
    Closure_obj *clos = malloc(sizeof (*clos));
    clos->body = fn_stmt->body;
    clos->params = fn_stmt->params;
    clos->arity = fn_stmt->count;
    clos->env = e->current;

    Rav_obj *object = new_object(CLOS_OBJ, 0);
    object->cl = clos;
    define(e, fn_stmt->name, object, e->current);
}

static void match_let(Evaluator *e, AST_let_stmt let) {
    Rav_obj *value = eval(e, let->value);
    if (!match(e, let->patt, value, e->current)) {
        // TODO: runtime handling
        fprintf(stderr, "Error: Let pattern mismatch\n");
    }
}

static void def_type(Evaluator *e, AST_type_stmt type_stmt) {
    char *cons_type = type_stmt->name;
    AST_variant *vars = type_stmt->variants;
    
    for (int i = 0; vars[i]; i++) {
        Rav_obj *cons = cons_object(cons_type, vars[i]->name,
                                    vars[i]->count);
        define(e, vars[i]->name, cons, e->current);
    }
}

static Rav_obj *exec_return(Evaluator *e, AST_ret_stmt ret_stmt) {
    Rav_obj *result = eval(e, ret_stmt->value);
    result->mode = From_Return;
    return result;
}

static Rav_obj *exec_fixed(TK_type type) {
    Rav_obj *res;
    if (type == TK_BREAK) {
        res = RNil;
        res->mode = From_Break;
        return res;
    } else {
        /* TK_CONTINUE */
        res = RNil;
        res->mode = From_Continue;
        return res;
    }
}

static Rav_obj *walk_piece(Evaluator *e, AST_piece piece, Env *env_new) {
    Rav_obj *result = RNil; /* by default */
    
    Env *old_env = e->current;
    e->current = env_new;
    
    AST_stmt *stmts = piece->stmts;
    for (int i = 0; stmts[i]; i++) {
        result = execute(e, stmts[i]);
        if ((result->mode & From_Return) ||
            (result->mode & From_Break) ||
            (result->mode & From_Continue)) {
            e->current = old_env;
            return result;
        }
    }
    
    e->current = old_env;
    return result->type == VOID_OBJ ? RNil : result;
}

static void
define_builtin(Evaluator *e, char *name, Builtin fn, int arity) {
    Builtin_obj *builtin = malloc(sizeof (*builtin));
    builtin->fn = fn;
    builtin->arity = arity;

    Rav_obj *object = new_object(BLTIN_OBJ, 0);
    object->bl = builtin;
    
    define(e, name, object, NULL);
}

/* define the built-in functions in the global scope */
static void define_builtins(Evaluator *e) {
    /* printting functions */
    define_builtin(e, "print", Rav_print, -1);
    define_builtin(e, "println", Rav_println, -1);
    
    /* list functions */
    define_builtin(e, "hd", Rav_hd, 1);
    define_builtin(e, "tl", Rav_tl, 1);
}

/*** INTERFACE ***/

void init_eval(Evaluator *e, int vars) {
    e->global = malloc(sizeof(Table));
    e->vars = malloc(sizeof(Table));
    init_table(e->global, 191, hash_str, free, comp_str);
    init_table(e->vars, vars, hash_ptr, free, comp_ptr);

    define_builtins(e);
    e->current = NULL;
}

void free_eval(Evaluator *e) {
    free_table(e->global);
    free_table(e->vars);
}

Rav_obj *eval(Evaluator *e, AST_expr expr) {
    switch (expr->type) {
    case ASSIGN_EXPR:
        return eval_assign(e, expr->assign);
    case BINARY_EXPR:
        return eval_binary(e, expr->binary);
    case CALL_EXPR:
        return eval_call(e, expr->call);
    case COND_EXPR:
        return eval_cond(e, expr->cond);
    case FOR_EXPR:
        return eval_for(e, expr->for_expr);
    case GROUP_EXPR:
        return eval(e, expr->group->expr);
    case IDENT_EXPR:
        return eval_ident(e, expr);
    case IF_EXPR:
        return eval_if(e, expr->if_expr);
    case INDEX_EXPR:
        return eval_index(e, expr->index);
    case LIT_EXPR:
        return eval_lit(e, expr->lit);
    case MATCH_EXPR:
        return eval_match(e, expr->match);
    case UNARY_EXPR:
        return eval_unary(e, expr->unary);
    case WHILE_EXPR:
        return eval_while(e, expr->while_expr);
    }
    
    fprintf(stderr, "[INTERNAL] invalid expr_type (%d)\n", expr->type);
    assert(0);
}

Rav_obj *execute(Evaluator *e, AST_stmt stmt) {
    switch (stmt->type) {
    case EXPR_STMT:
        return eval(e, stmt->expr->expr);
    
    case FN_STMT:
        def_function(e, stmt->fn);
        return RVoid;

    case LET_STMT:
        match_let(e, stmt->let);
        return RVoid;

    case TYPE_STMT:
        def_type(e, stmt->type_stmt);
        return RVoid;

    case RET_STMT:
        return exec_return(e, stmt->ret);
    case FIXED_STMT:
        return exec_fixed(stmt->fixed);
    }
    
    fprintf(stderr, "[INTERNAL] invalid stmt_type (%d)\n", stmt->type);
    assert(0);
}

Rav_obj *walk(Evaluator *e, AST_piece piece) {
    return walk_piece(e, piece, e->current);
}
