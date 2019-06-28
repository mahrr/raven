/*
 * (eval.c | 26 April 19 | Kareem Hamdy, Amr Anwar)
 *
 * Raven syntax tree interpreter implementation.
 *
 */
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "ast.h"
#include "env.h"
#include "eval.h"
#include "hashing.h"
#include "list.h"
#include "object.h"
#include "table.h"

/** INTERNALS **/

Rav_obj False_obj = {BOOL_OBJ, 0, {.b = 0}};
Rav_obj True_obj = {BOOL_OBJ, 0, {.b = 1}};
Rav_obj Nil_obj = {NIL_OBJ, 0, {}};
Rav_obj Empty_obj = {VOID_OBJ, 0, {}};

#define True (Rav_obj *)(&True_obj)
#define False (Rav_obj *)(&False_obj)
#define Nil (Rav_obj *)(&Nil_obj)

/* for return object and fixed stmt */
#define From_Return 0x01
#define From_break 0x02
#define From_continue 0x04

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

Rav_obj *new_object(Rav_type type, uint32_t mode) {
    Rav_obj *object = malloc(sizeof(*object));
    object->type = type;
    object->mode = mode;
    return object;
}

static Rav_obj *int_object(int64_t value) {
    Rav_obj *result = new_object(INT_OBJ, 0);
    result->i = value;
    return result;
}

static Rav_obj *float_object(long double value) {
    Rav_obj *result = new_object(FLOAT_OBJ, 0);
    result->f = value;
    return result;
}

static Rav_obj *str_object(char *value) {
    Rav_obj *result = new_object(STR_OBJ, 0);
    result->s = value;
    return result;
}

static Rav_obj *list_object(AST_expr *values, Evaluator *e) {
    List *list_objs = NULL;
    for (int i = 0; values[i]; i++)
        list_objs = list_add(list_objs, eval(e, values[i]));

    Rav_obj *result = new_object(LIST_OBJ, 0);
    result->l = list_objs;
    return result;
}
static Rav_obj *fn_object(Evaluator *e, AST_fn_lit expr) {

    Cl_obj *cl_obj = malloc(sizeof(Cl_obj));

    cl_obj->body = expr->body;
    cl_obj->env = e->current;
    cl_obj->params = expr->params;

    Rav_obj *result = new_object(CL_OBJ, 0);
    result->cl = cl_obj;

    return result;
}
/*hash_lit  */
static void add_hash_sympol(Hash_obj *h_obj, char *sympol,
                            Rav_obj *data) {
    if (h_obj->str_table == NULL) {
        h_obj->str_table = malloc(sizeof(Table));
        init_table(h_obj->str_table, 191, hash_str, free,
                   comp_str);
    }

    table_put(h_obj->str_table, sympol, data);
}

static void add_hash_obj(Evaluator *e, Hash_obj *h_obj,
                         AST_expr key_expr, Rav_obj *data) {
    Rav_obj *r_obj = eval(e, key_expr);
    switch (r_obj->type) {
    case STR_LIT:
        add_hash_sympol(h_obj, r_obj->s, data);
        break;
    case INT_LIT:
        /*NOT HANDLE YET */
    case FLOAT_LIT:
        /*NOT HANDEL YET */
        printf("NOT HANDEL YET \n");
        break;
    default:
        if (h_obj->obj_table == NULL) {
            h_obj->obj_table = malloc(sizeof(Table));
            init_table(h_obj->obj_table, 191, hash_ptr, free,
                       comp_ptr);
        }

        table_put(h_obj->obj_table, r_obj, data);
        break;
    }
}

static Rav_obj *hash_object(Evaluator *e,
                            AST_hash_lit hash_expr) {

    Hash_obj *hash_obj = malloc(sizeof(Hash_obj));

    AST_key *curr_key = hash_expr->keys;
    AST_expr *curr_expr = hash_expr->values;
    for (int i = 0; curr_key[i]; i++) {
        if (curr_key[i]->type == INDEX_KEY) {
            printf("INDEX Key \n");
        } else if (curr_key[i]->type == SYMBOL_KEY) {
            add_hash_sympol(hash_obj, curr_key[i]->symbol,
                            eval(e, curr_expr[i]));
        } else {
            add_hash_obj(e, hash_obj, curr_key[i]->expr,
                         eval(e, curr_expr[i]));
        }
    }
    Rav_obj *result = new_object(HASH_OBJ, 0);
    result->h = hash_obj;
    return result;
}

static Rav_obj *eval_lit(Evaluator *e, AST_lit_expr lit_expr) {
    switch (lit_expr->type) {
    case INT_LIT:
        return int_object(lit_expr->i);
    case FLOAT_LIT:
        return float_object(lit_expr->f);
    case TRUE_LIT:
        return True;
    case FALSE_LIT:
        return False;
    case NIL_LIT:
        return Nil;
    case STR_LIT:
        return str_object(lit_expr->s);
    case LIST_LIT:
        return list_object(lit_expr->list->values, e);
    case FN_LIT:
        return fn_object(e, lit_expr->fn);
    case HASH_LIT:
        return hash_object(e, lit_expr->hash);
    // Error but now NIL
    default:
        printf("ERROR FROM EVAL_LIT \n");
        return Nil;
    }
}

/* infix functions */

static long double float_of(Rav_obj *obj) {
    if (obj->type == FLOAT_OBJ) return obj->f;
    return (long double)obj->i;
}

static Rav_obj *calc_infix_float(long double l, long double r,
                                 TK_type op) {
    switch (op) {
    /* logic operations */
    case TK_GT:
        return l > r ? True : False;
    case TK_GT_EQ:
        return l >= r ? True : False;
    case TK_LT:
        return l < r ? True : False;
    case TK_LT_EQ:
        return l <= r ? True : False;
    case TK_EQ_EQ:
        return l == r ? True : False;
    case TK_BANG_EQ:
        return l != r ? True : False;
    /* arith operation */
    case TK_PLUS:
        return float_object(l + r);
    case TK_MINUS:
        return float_object(l - r);
    case TK_ASTERISK:
        return float_object(l * r);

    case TK_SLASH:
        return float_object(l / r);
        break;
    case TK_PERCENT:
        return float_object(fmodl(l, r));
        break;
    default:
        /* ERROR */
        return float_object(0);
    }
}

static Rav_obj *calc_infix_int(int64_t l, int64_t r,
                               TK_type op) {
    switch (op) {
        /* logic operations */
    case TK_GT:
        return l > r ? True : False;
    case TK_GT_EQ:
        return l >= r ? True : False;
    case TK_LT:
        return l < r ? True : False;
    case TK_LT_EQ:
        return l <= r ? True : False;
    case TK_EQ_EQ:
        return l == r ? True : False;
    case TK_BANG_EQ:
        return l != r ? True : False;

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
/*works for numbers only */
static Rav_obj *infix_op(Rav_obj *left, Rav_obj *right,
                         TK_type op) {
    long double r_fval, l_fval;
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
        /* runtime error : non numerical operand */
        return NULL;
    }

    /* check for zero division */
    if ((op == TK_SLASH || op == TK_PERCENT) &&
        float_of(right) == 0.0) {
        /* runtime error : zero divisor */
        return NULL;
    }

    return is_float ? calc_infix_float(l_fval, r_fval, op)
                    : calc_infix_int(l_ival, r_ival, op);
}

static void *shallow_copy(void *r) { return r; }

static Rav_obj *list_concat(Rav_obj *left, Rav_obj *right) {
    if (left->type != LIST_OBJ || right->type != LIST_OBJ) {
        /*ERROR*/
        return Nil;
    }
    List *new_list = NULL;
    new_list = list_copy(left->l, shallow_copy);
    new_list = list_append(new_list, right->l);

    Rav_obj *result = new_object(LIST_OBJ, 0);
    result->l = new_list;
    return result;
}

static Rav_obj *eval_binary(Evaluator *e,
                            AST_binary_expr binary_expr) {
    Rav_obj *left;
    Rav_obj *right;
    TK_type op = binary_expr->op;
    switch (op) {
    case TK_AND:
        left = eval(e, binary_expr->left);
        if (!is_true(left)) return left;
        return eval(e, binary_expr->right);
    case TK_OR:
        left = eval(e, binary_expr->left);
        if (is_true(left)) return left;
        return eval(e, binary_expr->right);
    case TK_AT:
        left = eval(e, binary_expr->left);
        right = eval(e, binary_expr->right);
        return list_concat(left, right);
    // case TK_DOT:
    //     return eval_call(left, right, op);
    default:
        left = eval(e, binary_expr->left);
        right = eval(e, binary_expr->right);
        return infix_op(left, right, op);
    }
}
static Rav_obj *walk_piece(Evaluator *e, AST_piece piece,
                           Env *env_new);

static Rav_obj *eval_if(Evaluator *e, AST_if_expr if_expr) {
    Rav_obj *cond = eval(e, if_expr->cond);
    Rav_obj *res_obj;
    if (is_true(cond)) {
        res_obj =
            walk_piece(e, if_expr->then, new_env(e->current));
        return res_obj;
    }

    /* eval elif*/
    AST_elif *elif = if_expr->elifs;
    for (int i = 0; elif[i]; i++) {
        cond = eval(e, elif[i]->cond);
        if (is_true(cond)) {
            res_obj = walk_piece(e, elif[i]->then,
                                 new_env(e->current));
            return res_obj;
        }
    }
    /*eval  else */
    if (if_expr->alter != NULL) {
        res_obj =
            walk_piece(e, if_expr->alter, new_env(e->current));
        return res_obj;
    }

    /*all conditions is false, null object */
    return Nil;
}

static Rav_obj *eval_unary(Evaluator *e,
                           AST_unary_expr unary_expr) {
    Rav_obj *operand = eval(e, unary_expr->operand);
    switch (unary_expr->op) {
    case TK_MINUS:
        if (operand->type == FLOAT_OBJ) {
            return float_object(-operand->f);
        } else if (operand->type == INT_OBJ) {
            return int_object(-operand->i);
        } else {
            /* ERORR NOT A NUMBER*/
            return Nil;
        }

    case TK_NOT:
        return is_true(operand) ? False : True;
    default:
        return NULL;
    }
}
/* Eval_index_Expr  used for  hash_object  only  */
static Rav_obj *get_hash_value(Evaluator *e, Rav_obj *index_obj,
                               Rav_obj *h_obj) {
    switch (index_obj->type) {
    case INT_OBJ:
        /*TODO*/
        printf("TODO INDEX \n");
        return Nil;
    case FLOAT_OBJ:
        /*TODO*/
        printf("TODO FLOAT KEY \n");
        return Nil;
    case STR_OBJ:
        return (Rav_obj *)table_get(h_obj->h->str_table,
                                    index_obj->s);
    default:
        return (Rav_obj *)table_get(h_obj->h->obj_table,
                                    index_obj);
    }
}

static Rav_obj *is_hash_obj(Evaluator *e, AST_expr expr) {
    Rav_obj *h_obj = eval(e, expr);
    if (h_obj == NULL || h_obj->type != HASH_OBJ) {
        /*Error not hash  */
        printf("Not a hash \n");
        return NULL;
    }
    return h_obj;
}

static Rav_obj *eval_index(Evaluator *e, AST_index_expr expr) {
    Rav_obj *h_obj = is_hash_obj(e, expr->object);
    if (h_obj == NULL) {
        /*Error NOT HASH  */
        printf("Error NOT HASH \n");
        return NULL;
    }
    Rav_obj *value =
        get_hash_value(e, eval(e, expr->index), h_obj);
    if (value == NULL) {
        printf("NOT FOUND \n");
        return Nil;
    }
    return value;
}

static Rav_obj *eval_ident(Evaluator *e, AST_expr expr) {
    /*
    table_get()
    take expr to allow using
    same name of variable on different scopes
    return data in this case it's array[2]
    */
    int *loc = (int *)table_get(e->vars, expr);
    if (e->current == NULL) {
        printf(" null env \n ");
        return Nil;
    }
    return env_get(e->current, loc[1], loc[0]);
}
/* works for ident and hash_obj only  */
static Rav_obj *eval_assign(Evaluator *e, AST_assign_expr expr) {
    if (expr->lvalue->type == IDENT_EXPR) {
        int *loc = (int *)table_get(e->vars, expr->lvalue);
        env_set(e->current, eval(e, expr->value), loc[1],
                loc[0]);
        return env_get(e->current, loc[1], loc[0]);
    } else if (expr->lvalue->type == INDEX_EXPR) {
        Rav_obj *r = eval(e, expr->lvalue->index->object);
        if (r->type != HASH_OBJ) {
            printf("Error not HASH_OBJ \n");
            return Nil;
        }
        Rav_obj *data = eval(e, expr->value);
        add_hash_obj(e, r->h, expr->lvalue->index->index, data);
        /*CASE of index Expr  */
        return data;
    }
    /*EROR */
    return NULL;
}

/* works for ident expr only for now */

static Rav_obj *eval_call(Evaluator *e, AST_call_expr call_exp) {
    Rav_obj *fun = eval(e, call_exp->func);
    if (fun->type != CL_OBJ) {
        printf("not function \n");
        /*ERROR */
        return Nil;
    }

    AST_expr *args = call_exp->args;
    AST_patt *params = fun->cl->params;
    Env *env_new = new_env(fun->cl->env);
    int i = 0;
    while (args[i]) {
        if (params[i] == NULL) {
            /*Error */
            printf(
                "paramter length not Equal to args length \n");
            return Nil;
        }
        env_add(env_new, eval(e, args[i]));
        i++;
    }
    if (params[i] != NULL) {
        /*Error */
        printf("paramter length not Equal to args length \n");
        return Nil;
    }

    Rav_obj *res_obj = walk_piece(e, fun->cl->body, env_new);
    if (res_obj->mode & From_Return) {
        res_obj->mode &= ~From_Return;
        return res_obj;
    }
    return res_obj;
}

static Rav_obj *eval_while(Evaluator *e, AST_while_expr w_expr) {
    Rav_obj *res = Nil;
    while (is_true(eval(e, w_expr->cond))) {
        res = walk_piece(e, w_expr->body, new_env(e->current));
        if (res->mode & From_break) {
            res->mode &= ~From_break;
            return res;
        } else if (res->mode & From_continue) {
            res->mode &= ~From_continue;
        } else if (res->mode & From_Return)
            return res;
    }
    return res;
}

static Rav_obj *exec_function(Evaluator *e,
                              AST_fn_stmt fn_stmt) {

    Cl_obj *cl_obj = malloc(sizeof(Cl_obj));

    cl_obj->body = fn_stmt->body;
    cl_obj->env = e->current;
    cl_obj->params = fn_stmt->params;

    Rav_obj *r_obj = new_object(CL_OBJ, 0);
    r_obj->cl = cl_obj;
    env_add(e->current, r_obj);

    return Nil;
}

static Rav_obj *exec_return(Evaluator *e,
                            AST_ret_stmt ret_stmt) {
    Rav_obj *result = eval(e, ret_stmt->value);
    result->mode = From_Return;
    return result;
}

static Rav_obj *exec_fixed(Evaluator *e, TK_type type) {
    Rav_obj *res;
    if (type == TK_BREAK) {
        res = Nil;
        res->mode = From_break;
        return res;
    } else {
        /*TK_CONTINUE */
        res = Nil;
        res->mode = From_continue;
        return res;
    }
}

static Rav_obj *walk_piece(Evaluator *e, AST_piece piece,
                           Env *env_new) {
    Rav_obj *result = NULL;

    Env *old_env = e->current;
    e->current = env_new;
    AST_stmt *stmts = piece->stmts;
    for (int i = 0; stmts[i]; i++) {
        result = execute(e, stmts[i]);

        if ((result->mode & From_Return) ||
            (result->mode & From_break) ||
            (result->mode & From_continue)) {
            e->current = old_env;
            return result;
        }
    }
    e->current = old_env;
    return result;
}
/*works for Rav_objs only  */
static void inspect_list(Rav_obj *r) {
    printf("[\n");
    List *curr_list = r->l;
    while (curr_list) {
        inspect(curr_list->head);
        curr_list = curr_list->tail;
    }
    printf("] \n ");
}

/** INTERFACE **/

void inspect(Rav_obj *r) {
    switch (r->type) {
    case INT_OBJ:
        printf("value :%ld, type: Int \n", r->i);
        break;
    case FLOAT_OBJ:
        printf("value :%Lf, type: float \n", r->f);
        break;
    case BOOL_OBJ:
        if (r->b) {
            printf("value: true  type: boolean\n");
        } else {
            printf("value: false  type: boolean\n");
        }
        break;
    case NIL_OBJ:
        printf("value: nil  type:NIL\n");
        break;
    case STR_OBJ:
        printf("value: %s type: string \n", r->s);
        break;

    case LIST_OBJ:
        inspect_list(r);
        printf("type: list \n");
        break;
    case VOID_OBJ:
        printf("type Void_object \n");
        break;
    case CL_OBJ:
        printf("value:<Fn>  type :function \n");
        break;
    case HASH_OBJ:
        printf("value:<hash> type :hash \n");
        break;
    default:

        printf("error \n");
        break;
    }
}

void init_eval(Evaluator *e, int vars) {
    e->global = new_env(NULL);
    e->current = e->global;

    e->vars = malloc(sizeof(Table));
    init_table(e->vars, vars, hash_ptr, free, comp_ptr);
}

void free_eval(Evaluator *e) {
    /* free_e frees its parent, so
       e->global will be free as well */
    free_env(e->current);
    free_table(e->vars);
}

Rav_obj *eval(Evaluator *e, AST_expr expr) {
    switch (expr->type) {
    case LIT_EXPR:
        return eval_lit(e, expr->lit);
    case GROUP_EXPR:
        return eval(e, expr->group->expr);
    case UNARY_EXPR:
        return eval_unary(e, expr->unary);
    case BINARY_EXPR:
        return eval_binary(e, expr->binary);
    case IDENT_EXPR:
        return eval_ident(e, expr);
    case ASSIGN_EXPR:
        return eval_assign(e, expr->assign);
    case CALL_EXPR:
        return eval_call(e, expr->call);
    case IF_EXPR:
        return eval_if(e, expr->if_expr);
    case INDEX_EXPR:
        return eval_index(e, expr->index);
    case WHILE_EXPR:
        return eval_while(e, expr->while_expr);
    default:
        return NULL;
    }
}

Rav_obj *execute(Evaluator *e, AST_stmt stmt) {
    switch (stmt->type) {
    case EXPR_STMT:
        return eval(e, stmt->expr->expr);
    case LET_STMT:
        env_add(e->current, eval(e, stmt->let->value));
        return (Rav_obj *)&Empty_obj;
    case FN_STMT:
        return exec_function(e, stmt->fn);
    case RET_STMT:
        return exec_return(e, stmt->ret);
    case FIXED_STMT:
        return exec_fixed(e, stmt->fixed);
    default:
        return NULL;
    }
}

Rav_obj *walk(Evaluator *e, AST_piece piece) {
    return walk_piece(e, piece, e->global);
}
