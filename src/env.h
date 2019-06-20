/*
 * (env.h | 10 May 19 | Ahmad Maher)
 *
 * Block Environment Interface
 *
 * Only the evaluator uses these functions in the runtime
 * for assigning and retrieval of variables.
 *
 * Environments representation are simple dynamic arrays and
 * each variable is identified by an index which is stored at 
 * the locals lookup tables for every evaluator state.
 *
*/

#ifndef env_h
#define env_h

#include "array.h"
#include "object.h"

/* forward declaration to avoid cyclic include 
   problem with 'object.h' */
typedef struct Rav_obj Rav_obj;

typedef struct Env {
    struct Env *enclosing;
    ARRAY(Rav_obj*) vars;
} Env;

/**
 * return an allocated environment with 'enclosing' 
 * as its parent environment.
 */
Env *new_env(Env *enclosing);

/**
 * dispose env internal array and the 'env' structure space itself.
 * it applies recursively to the 'env' enclosing environments.
 *
*/
void free_env(Env *env);

/**
 * add an abject 'obj' to the environment. 
*/
void env_add(Env *env, Rav_obj *obj);

/**
 * get an object stored 'distance' steps outer 
 * 'env' and located at 'slot' index.
*/
Rav_obj *env_get(Env *env, int distance, int slot);

/**
 * put an 'obj' at 'distance' steps outer 'env'
 * at 'slot' index. 
*/
void env_set(Env *env, Rav_obj *obj, int distance, int slot);

#endif
