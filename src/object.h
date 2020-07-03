#ifndef raven_object_h
#define raven_object_h

// Raven Heap Object Representation

#include "common.h"
#include "chunk.h"
#include "value.h"
#include "vm.h"

typedef enum {
    OBJ_STRING,
    OBJ_PAIR,
    OBJ_ARRAY,
    OBJ_FUNCTION,
    OBJ_UPVALUE,
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

struct RavPair {
    Object header;
    Value head;
    Value tail;
};

struct RavArray {
    Object header;
    Value *values;
    size_t count;
    size_t capacity;
};

struct RavFunction {
    Object header;
    RavString *name;
    int arity;
    int upvalue_count;
    Chunk chunk;
};

struct RavUpvalue {
    Object header;
    Value *location;
    Value captured;
    struct RavUpvalue *next;
};

struct RavClosure {
    Object header;
    RavFunction *function;
    RavUpvalue **upvalues;
    int upvalue_count;
};

#define Obj_Type(value) (As_Obj(value)->type)

#define Is_String(value)   is_object_type(value, OBJ_STRING)
#define Is_Pair(value)     is_object_type(value, OBJ_PAIR)
#define Is_Array(value)    is_object_type(value, OBJ_ARRAY)
#define Is_Function(value) is_object_type(value, OBJ_FUNCTION)
#define Is_Closure(value)  is_object_type(value, OBJ_CLOSURE)

#define As_String(value)   ((RavString *)As_Obj(value))
#define As_Pair(value)     ((RavPair *)As_Obj(value))
#define As_Array(value)    ((RavArray *)As_Obj(value))
#define As_CString(value)  ((As_String(value))->chars)
#define As_Function(value) ((RavFunction *)As_Obj(value))
#define As_Closure(value)  ((RavClosure *)As_Obj(value))


// Note the first parameter of constructing functions is a vm
// instance, that's because all the system allocated objects
// are chained together with intrusive linked list, and the
// head of that list is stored in a vm state. A better approach
// would to store that list in some sort of an allocator state,
// but I stick with the simple approach for the time being.

// Construct a RavString with a copy of the given string.
RavString *new_string(VM *vm, const char *chars, int length);

// Construct a RavString wrapping the given string.
// The object will have an ownership of the chars memory.
RavString *box_string(VM *vm, char *chars, int length);

// Construct a RavPair with the given head and tail.
RavPair *new_pair(VM *vm, Value head, Value tail);

// Construct a RavArray from the provieded sized array.
RavArray *new_array(VM *vm, Value *array, size_t count);

// Construct an empty function object.
RavFunction *new_function(VM *vm);

// Construct an upvalue object, with a reference to the captured value.
RavUpvalue *new_upvalue(VM *vm, Value *location);

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
