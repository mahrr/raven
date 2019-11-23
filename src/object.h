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
 * inside the language itself, as the language
 * syntax doesn't permit a statement in a place
 * of an expression. It's a side effect of the
 * enforcment that the 'eval' function must
 * return a Rav_Obj pointer.
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

struct Rav_obj {
    Rav_type type;
    uint8_t mode;   /* status bits used internally by the evaluator */
    union {
        /* primitives */
        int b;           /* boolean */
        double f;        /* float */
        int64_t i;       /* integer */
        List *l;         /* list */

        /* Builtin functions */
        struct {
            Builtin fn;    /* a pointer to the acutal function */
            int bl_arity;  /* number of paramters,-1 if variadic */
        };

        /* Closure object */
        struct {
            /* the environemt in which the function was defined */
            Env *env;
            AST_patt *params; /* list of parameters (AST_patt) */
            AST_piece body;
            int cl_arity;
        };

        /* Constructor object, which are declared with type statements */
        struct {
            char *dtype;   /* the constructor data type name */
            char *name;    /* constructor name */
            int cs_arity;  /* number of constructor parameters */
        };
        
        /* String object */
        struct {
            char *str;    /* a pointer to the actual string */
            size_t len;   /* the length of the string (NULL excluded) */
            size_t size;  /* the actual size after without escaping */
        };

        
        /* Variant object, the result of calling a constructor object */
        struct {
            Rav_obj *cons;    /* the variant constructor */
            Rav_obj **elems;  /* array of elements */
            int count;        /* number of data elements */
        };

        
        /*
         * Hash object:
         * Hash in raven allows any kind of object to be a key.
         * Because every type of object has its own hash fucntion,
         * the hash object maintains a table for each kind of key,
         * ints, floats, strings and ref objects (e.g. lists and hashes).
         * 
        */
        struct {
            Table *float_table;
            Table *int_table;
            Table *str_table;
            Table *obj_table;
        };
        
    };
};

extern Rav_obj False_obj;
extern Rav_obj True_obj;
extern Rav_obj Nil_obj;
extern Rav_obj Void_obj;

#define RTrue  (Rav_obj *)(&True_obj)
#define RFalse (Rav_obj *)(&False_obj)
#define RNil   (Rav_obj *)(&Nil_obj)
#define RVoid  (Rav_obj *)(&Void_obj)

/* return a string representation of an object type */
char *object_type(Rav_obj *object);

/* print a string representation of an object to the stdout */
void echo_object(Rav_obj *object);

/* like echo_object, except it escape strings before printing */
void print_object(Rav_obj *object);

/* an object constructor */
Rav_obj *new_object(Rav_type type, uint8_t mode);

#endif
