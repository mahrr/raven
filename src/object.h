/*
 * (object.h | | )
 *
 * Raven Object Model.
 *
*/

#ifndef object_h
#define object_h

#include <stdint.h>

#include "array.h"
#include "ast.h"
#include "env.h"
#include "list.h"
#include "table.h"

/* forward declaration to avoid cyclic include 
   problem with 'env.h' */
typedef struct Env Env;

typedef struct Rav_obj Rav_obj;
typedef struct Cl_obj Cl_obj;
typedef struct Hash_obj Hash_obj;

/* 
 * void object returns from the statements 
 * execution if it's not an expression statement.
 * it's always discarded, and can't be used 
 * inside the langauges itself, as the language
 * syntax doesn't permit a statement in a place
 * of an expression.
 * it's different from a nil object in that
 * the console doesn't consider it for printting
 * unlike nil objects which are printed normally.
*/

typedef enum {
    BOOL_OBJ,
    VOID_OBJ,
    FLOAT_OBJ,
    CL_OBJ,
    HASH_OBJ,
    LIST_OBJ,
    INT_OBJ,
    NIL_OBJ,
    STR_OBJ,
} Rav_type;

/* closure object */
struct Cl_obj {
    /* the environemt in which the function was defined */
    Env *env;
    /* list of parameters (AST_patt) */
    List params;
    AST_piece body;
};

/*
 * Hash in raven allow any kind of object to be a key.
 * because every type of object has its own hash fucntion,
 * the hash object maintains a table for each kind of key,
 * strings, floats and objects (e.g. lists and hashes).
 * The integer keys are mapped to a dynamic array.
 * 
*/
struct Hash_obj {
    ARRAY(int) *array;
    Table *float_table;
    Table *str_table;
    Table *obj_table;
};

struct Rav_obj {
    Rav_type type;
    union {
        int b;          /* bool */
        long double f;  /* float */
        List l;         /* list */
        int64_t i;      /* integer */
        char *s;        /* string */
        Cl_obj *cl;     /* closure */
        Hash_obj *h;    /* hash */
    } obj;
};

#endif
