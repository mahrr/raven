/*
 * (object.h | 14 Jun 19 | Amr Anwar, Ahmad Maher)
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
typedef struct Builtin_obj Builtin_obj;
typedef struct Closure_obj Closure_obj;
typedef struct Cons_obj Cons_obj;
typedef struct Hash_obj Hash_obj;
typedef struct Variant_obj Variant_obj;

/* 
 * built_in functions type, takes a NULL terminated array
 * of Rav_objects pointers. this allow a variable number 
 * of arguments (indefinite arity) functions.
*/
typedef Rav_obj *(*Builtin)(Rav_obj **objects);

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
    BLTIN_OBJ,
    CLOS_OBJ,
    CONS_OBJ,
    FLOAT_OBJ,
    HASH_OBJ,
    LIST_OBJ,
    INT_OBJ,
    NIL_OBJ,
    STR_OBJ,
    VARI_OBJ,
    VOID_OBJ
} Rav_type;

struct Builtin_obj {
    Builtin fn;    /* a pointer to the acutal function */
    int8_t arity;  /* -1 if variadic */
};

/* closure object */
struct Closure_obj {
    /* the environemt in which the function was defined */
    Env *env;
    /* list of parameters (AST_patt) */
    AST_patt *params;
    int8_t arity;
    AST_piece body;
};

/* constructor object, which are declared with type statements */
struct Cons_obj {
    char *type;       /* the constructor data type name */
    char *name;       /* constructor name */
    int8_t arity;     /* number of constructor parameters */
};

/* variant object, the result of calling a constructor object */
struct Variant_obj {
    Rav_obj *cons;    /* the variant constructor */
    Rav_obj **elems;  /* array of elements */
    int8_t count;     /* number of data elements */
};

/*
 * Hash in raven allows any kind of object to be a key.
 * Because every type of object has its own hash fucntion,
 * the hash object maintains a table for each kind of key,
 * strings, floats and ref objects (e.g. lists and hashes).
 * The integer keys are mapped to a dynamic array, or
 * a int hash table based on index range and convergent.
 * 
*/
struct Hash_obj {
    ARRAY(int) *array;
    Table *float_table;
    Table *int_table;
    Table *str_table;
    Table *obj_table;
};

struct Rav_obj {
    Rav_type type;
    uint8_t mode;   /* status bits used internally by the evaluator */
    union {
        int8_t b;        /* boolean */
        long double f;   /* float */
        int64_t i;       /* integer */
        char *s;         /* string */
        Builtin_obj *bl; /* builtin function */
        Closure_obj *cl; /* closure */
        Cons_obj *cn;    /* constructor */
        Hash_obj *h;     /* hash */
        List *l;         /* list */
        Variant_obj *vr; /* variant */
    };
};

#endif
