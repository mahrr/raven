#ifndef raven_object_h
#define raven_object_h

// Raven Heap Object Representation

#include "common.h"
#include "chunk.h"
#include "mem.h"
#include "table.h"
#include "value.h"
#include "vm.h"

typedef bool (*CFunc)(VM*, Value*, size_t, Value*);

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
    int arity_min;
    int arity_max;
};

#define Obj_Type(value) (As_Obj(value)->type)

#define Is_String(value)    object_type_is(value, OBJ_STRING)
#define Is_Pair(value)      object_type_is(value, OBJ_PAIR)
#define Is_Array(value)     object_type_is(value, OBJ_ARRAY)
#define Is_Map(value)       object_type_is(value, OBJ_MAP)
#define Is_Function(value)  object_type_is(value, OBJ_FUNCTION)
#define Is_Closure(value)   object_type_is(value, OBJ_CLOSURE)
#define Is_CFunction(value) object_type_is(value, OBJ_CFUNCTION)

#define As_String(value)    ((RavString *)As_Obj(value))
#define As_Pair(value)      ((RavPair *)As_Obj(value))
#define As_Array(value)     ((RavArray *)As_Obj(value))
#define As_Map(value)       ((RavMap *)As_Obj(value))
#define As_CString(value)   ((As_String(value))->chars)
#define As_Function(value)  ((RavFunction *)As_Obj(value))
#define As_Closure(value)   ((RavClosure *)As_Obj(value))
#define As_CFunction(value) ((RavCFunction *)As_Obj(value))

/// Object API

// Construct a RavString with a copy of the given string.
RavString *object_string(Allocator *allocator, const char *chars, int length);

// Construct a RavString wrapping the given string.
// The object will have an ownership of the chars memory.
RavString *object_string_box(Allocator *allocator, char *chars, int length);

// Construct a RavPair with the given head and tail.
RavPair *object_pair(Allocator *allocator, Value head, Value tail);

// Construct a RavArray from the provided sized array.
RavArray *object_array(Allocator *allocator, Value *array, size_t count);

// Construct an empty RavMap.
RavMap *object_map(Allocator *allocator);

// Construct an empty function object.
RavFunction *object_function(Allocator *allocator);

// Construct an upvalue object, with a reference to the captured value.
RavUpvalue *object_upvalue(Allocator *allocator, Value *location);

// Construct a closure object.
RavClosure *object_closure(Allocator *allocator, RavFunction *function);

// Construct a C function object.
RavCFunction *object_cfunction(Allocator *allocator, CFunc func, int arity_min, int arity_max);

// Pretty print a raven object to the standard output.
void object_print(Value value);

// Check if a given raven value is an object with specified type.
static inline bool object_type_is(Value value, ObjectType type) {
    return Is_Obj(value) && Obj_Type(value) == type;
}

/// Object Utilites

typedef struct StringBuffer {
    Allocator *allocator;
    int count;
    int capacity;
    char *buffer;
} StringBuffer;

// Constructs a new string buffer
StringBuffer string_buf_new(Allocator *allocator);

// Frees the given string buffer
void string_buf_free(StringBuffer* self);

// Pushes a string representation of the given value into the buffer
void string_buf_push(StringBuffer *self, Value value);

// Consumes the given string buffer and converts it into a string object
RavString *string_buf_into(StringBuffer *self);

#endif
