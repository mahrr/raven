/*
 * (eval.c | 26 April 19 | Kareem Hamdy, Amr Anwar)
 *
 * Raven syntax tree interpreter implementation.
 *
*/

#include <stdlib.h>

#include "ast.h"
#include "env.h"
#include "eval.h"
#include "hashing.h"
#include "object.h"
#include "table.h"

/** INTERNALS **/

const Rav_obj False_obj = { BOOL_OBJ, { .b = 0 } };
const Rav_obj True_obj  = { BOOL_OBJ, { .b = 1 } };
const Rav_obj Nil_obj   = { NIL_OBJ,  {} };
const Rav_obj Empty_obj = { VOID_OBJ, {} };

/** INTERFACE **/

void init_eval(Evaluator *e, int vars) {
    e->global = new_env(NULL);
    e->current = e->global;

    e->vars = malloc(sizeof(Table));
    init_table(e->vars, vars, hash_ptr, free, comp_ptr);
}

void free_eval(Evaluator *e) {
    /* free_env frees its parent, so 
       e->global will be free as well */
    free_env(e->current);
    free_table(e->vars);
}

Rav_obj *eval(Evaluator *e, AST_expr expr) {
    //TODO
    return NULL;
}

Rav_obj *execute(Evaluator *e, AST_stmt stmt) {
    //TODO
    return NULL;
}

Rav_obj *walk(Evaluator *e, AST_piece piece) {
    //TODO
    return NULL;
}
