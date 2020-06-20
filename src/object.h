#ifndef raven_object_h
#define raven_object_h

// Raven Heap Object Representation

#include "common.h"
#include "chunk.h"
#include "value.h"
#include "vm.h"

typedef enum {
    OBJ_STRING,
    OBJ_FUNCTION,
    OBJ_CLOSURE,
} ObjectType;

// The header (metadata) of all objects.
// TODO: consider using pointer tagging.
struct Object {
    ObjectType type;
    struct Object *next;
};

struct RavString {
    Object header;
    int length;
    uint32_t hash;
    char *chars;
};

struct RavFunction {
    Object header;
    RavString *name;
    int arity;
    Chunk chunk;
};

struct RavClosure {
    Object header;
    RavFunction *function;
};

#define Obj_Type(value) (As_Obj(value)->type)

#define Is_String(value)   is_object_type(value, OBJ_STRING)
#define Is_Function(value) is_object_type(value, OBJ_FUNCTION)
#define Is_Closure(value)  is_object_type(value, OBJ_CLOSURE)

#define As_String(value)   ((RavString *)As_Obj(value))
#define As_CString(value)  ((As_String(value))->chars)
#define As_Function(value) ((RavFunction *)As_Obj(value))
#define As_Closure(value)  ((RavClosure *)As_Obj(value))


// Note the first parameter of constructing functions is a vm
// instance, that's because all the system allocated objects
// are chained together with intrusive linked list, and the
// head of that list is stored in a vm state. A better approach
// would to store that list in some sort of an allocator state,
// but I stick with the simple approach for the time being.

// Construct an RavString with a copy of the given string.
RavString *new_string(VM *vm, const char *chars, int length);

// Construct an RavString wrapping the given string.
// The object will have an ownership of the chars memory.
RavString *box_string(VM *vm, char *chars, int length);

// Construct an empty function object.
RavFunction *new_function(VM *vm);

// Construct a closure object. The closure object doesn't own
// the function object memory, since multiple closures may be
// reference the same function object, besides the surrounding
// functions whose constant table may reference it.
RavClosure *new_closure(VM *vm, RavFunction *function);

void print_object(Value value);

// Free an intrusive linked list of objects.
void free_objects(Object *objects);

// Check if a given raven value is an object with specified type.
static inline bool is_object_type(Value value, ObjectType type) {
    return Is_Obj(value) && Obj_Type(value) == type;
}

#endif
