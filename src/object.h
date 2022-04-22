#ifndef raven_object_h
#define raven_object_h

// Raven Heap Object Representation

#include "common.h"
#include "chunk.h"
#include "mem.h"
#include "table.h"
#include "value.h"
#include "vm.h"

typedef Value (*CFunc)(VM*);

typedef enum {
    OBJ_STRING,
    OBJ_PAIR,
    OBJ_ARRAY,
    OBJ_MAP,
    OBJ_FUNCTION,
    OBJ_UPVALUE,
    OBJ_CLOSURE,
    OBJ_CFUNCTION,
} ObjectType;

// The header (metadata) of all objects.
// TODO: consider using pointer tagging.
struct Object {
    ObjectType type;
    bool marked;
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

struct RavMap {
    Object header;
    Table table;
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

// The closure object doesn't own the function object memory,
// since multiple closures may be reference the same function
// object, besides the surrounding functions whose constant
// table may reference it.
struct RavClosure {
    Object header;
    RavFunction *function;
    RavUpvalue **upvalues;
    int upvalue_count;
};

struct RavCFunction {
    Object header;
    CFunc func;
    int arity;
};

#define Obj_Type(value) (As_Obj(value)->type)

#define Is_String(value)    is_object_type(value, OBJ_STRING)
#define Is_Pair(value)      is_object_type(value, OBJ_PAIR)
#define Is_Array(value)     is_object_type(value, OBJ_ARRAY)
#define Is_Map(value)       is_object_type(value, OBJ_MAP)
#define Is_Function(value)  is_object_type(value, OBJ_FUNCTION)
#define Is_Closure(value)   is_object_type(value, OBJ_CLOSURE)
#define Is_CFunction(value) is_object_type(value, OBJ_CFUNCTION)

#define As_String(value)    ((RavString *)As_Obj(value))
#define As_Pair(value)      ((RavPair *)As_Obj(value))
#define As_Array(value)     ((RavArray *)As_Obj(value))
#define As_Map(value)       ((RavMap *)As_Obj(value))
#define As_CString(value)   ((As_String(value))->chars)
#define As_Function(value)  ((RavFunction *)As_Obj(value))
#define As_Closure(value)   ((RavClosure *)As_Obj(value))
#define As_CFunction(value) ((RavCFunction *)As_Obj(value))

// Construct a RavString with a copy of the given string.
RavString *new_string(Allocator *allocator, const char *chars, int length);

// Construct a RavString wrapping the given string.
// The object will have an ownership of the chars memory.
RavString *box_string(Allocator *allocator, char *chars, int length);

// Construct a RavPair with the given head and tail.
RavPair *new_pair(Allocator *allocator, Value head, Value tail);

// Construct a RavArray from the provided sized array.
RavArray *new_array(Allocator *allocator, Value *array, size_t count);

// Construct an empty RavMap.
RavMap *new_map(Allocator *allocator);

// Construct an empty function object.
RavFunction *new_function(Allocator *allocator);

// Construct an upvalue object, with a reference to the captured value.
RavUpvalue *new_upvalue(Allocator *allocator, Value *location);

// Construct a closure object.
RavClosure *new_closure(Allocator *allocator, RavFunction *function);

// Construct a C function object.
RavCFunction *new_cfunction(Allocator *allocator, CFunc func, int arity);

// Pretty print a raven object.
void print_object(Value value);

// Check if a given raven value is an object with specified type.
static inline bool is_object_type(Value value, ObjectType type) {
    return Is_Obj(value) && Obj_Type(value) == type;
}

#endif
