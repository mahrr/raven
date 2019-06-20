/*
 * (env.c | 10 May 19 | Ahmad Maher)
 *
 * Raven Block Environment
 *
*/

#include <stdlib.h>
#include <assert.h>

#include "array.h"
#include "env.h"
#include "object.h"

Env *new_env(Env *enclosing) {
    Env *env = malloc(sizeof(Env));
    env->enclosing = enclosing;
    ARR_INIT(&env->vars, Rav_obj*);

    return env;
}

void free_env(Env *env) {
    assert(env != NULL);
    while (env->enclosing != NULL) {
        free_env(env->enclosing);
        ARR_FREE(&env->vars);
        free(env);
    }
}

void env_add(Env *env, Rav_obj *obj) {
    assert(env != NULL);
    ARR_ADD(&env->vars, obj);
}

Rav_obj *env_get(Env *env, int distance, int slot) {
    assert(env != NULL);
    while (distance--) env = env->enclosing;
    return env->vars.elems[slot];
}

void env_set(Env *env, Rav_obj *obj, int distance, int slot) {
    assert(env != NULL);
    while (distance--) env = env->enclosing;
    env->vars.elems[slot] = obj;
}
