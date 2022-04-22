#include <string.h>
#include <stdio.h>

#include "common.h"
#include "chunk.h"
#include "table.h"
#include "mem.h"
#include "object.h"
#include "value.h"
#include "vm.h"

#define Alloc_Object(allocator, struct_type, object_type)               \
    (struct_type *)alloc_object(allocator,                              \
                                object_type,                            \
                                sizeof (struct_type))

static Object *alloc_object(Allocator *allocator, ObjectType type,
                            size_t size) {
    Object *object = (Object *)allocate(allocator, NULL, 0, size);
    object->type = type;
    object->marked = false;
    object->next = allocator->objects;

#ifdef DEBUG_TRACE_MEMORY
    printf("[Memory] %p : allocate %ld for %d\n", object, size, type);
#endif

    allocator->objects = object;
    return object;
}

static RavString *alloc_string(Allocator *allocator, int length, uint32_t hash, char *chars) {
    RavString *string = Alloc_Object(allocator, RavString, OBJ_STRING);
    string->length = length;
    string->hash = hash;
    string->chars = chars;

    table_set(&allocator->strings, string, Nil_Value);
    return string;
}

static uint32_t hash_string(const char *key, int length) {
    uint32_t hash = 2166136261u;

    for (int i = 0; i < length; i++) {
        hash ^= key[i];
        hash *= 16777619;
    }

    return hash;
}

RavString *new_string(Allocator *allocator, const char *chars, int length) {
    uint32_t hash = hash_string(chars, length);
    RavString *interned = table_interned(&allocator->strings, chars, hash, length);

    if (interned != NULL) return interned;

    char *copy = Alloc(allocator, char, length + 1);
    memcpy(copy, chars, length);
    copy[length] = '\0';

    return alloc_string(allocator, length, hash, copy);
}

RavString *box_string(Allocator *allocator, char *chars, int length) {
    uint32_t hash = hash_string(chars, length);
    RavString *interned = table_interned(&allocator->strings, chars, hash, length);

    // box_string takes ownership of chars memory, so if
    // the string is already interned, it frees this memory,
    // as it's no longer needed.
    if (interned != NULL) {
        Free_Array(allocator, char, chars, length + 1);
        return interned;
    }

    return alloc_string(allocator, length, hash, chars);
}

RavPair *new_pair(Allocator *allocator, Value head, Value tail) {
    RavPair *pair = Alloc_Object(allocator, RavPair, OBJ_PAIR);

    pair->head = head;
    pair->tail = tail;

    return pair;
}

RavArray *new_array(Allocator *allocator, Value *values, size_t count) {
    RavArray *array = Alloc_Object(allocator, RavArray, OBJ_ARRAY);

    array->header.marked = true; // for gc
    array->values = Alloc(allocator, Value, count);
    array->header.marked = false;
    array->count = count;
    array->capacity = count;

    memcpy(array->values, values, count * sizeof (Value));

    return array;
}

RavMap *new_map(Allocator *allocator) {
    RavMap *map = Alloc_Object(allocator, RavMap, OBJ_MAP);
    init_table(&map->table);
    return map;
}

RavFunction *new_function(Allocator *allocator) {
    RavFunction *function = Alloc_Object(allocator, RavFunction, OBJ_FUNCTION);

    function->name = NULL;
    function->arity = 0;
    function->upvalue_count = 0;

    init_chunk(&function->chunk);
    return function;
}

RavUpvalue *new_upvalue(Allocator *allocator, Value *location) {
    RavUpvalue *upvalue = Alloc_Object(allocator, RavUpvalue, OBJ_UPVALUE);

    upvalue->location = location;
    upvalue->captured = Void_Value;
    upvalue->next = NULL;

    return upvalue;
}

RavClosure *new_closure(Allocator *allocator, RavFunction *function) {
    RavUpvalue **upvalues = Alloc(allocator, RavUpvalue*, function->upvalue_count);

    for (int i = 0; i < function->upvalue_count; i++) {
        upvalues[i] = NULL;
    }

    RavClosure *closure = Alloc_Object(allocator, RavClosure, OBJ_CLOSURE);
    closure->function = function;
    closure->upvalues = upvalues;
    closure->upvalue_count = function->upvalue_count;

    return closure;
}

RavCFunction *new_cfunction(Allocator *allocator, CFunc func, int arity, bool variadic) {
    RavCFunction *cfunction = Alloc_Object(allocator, RavCFunction, OBJ_CFUNCTION);
    cfunction->func = func;
    cfunction->arity = arity;
    cfunction->variadic = variadic;
    return cfunction;
}

static void print_pair(RavPair *pair) {
    print_value(pair->head);

    // end of a proper list?
    if (Is_Nil(pair->tail)) {
        return;
    }

    if (Is_Pair(pair->tail)) {
        printf(", ");
        print_pair(As_Pair(pair->tail));
    } else {
        printf(" . ");
        print_value(pair->tail);
    }
}

// TODO: handling cyclic references

static void print_array(RavArray *array) {
    putchar('[');

    for (int i = 0; i < (int)array->count - 1; i++) {
        print_value(array->values[i]);
        printf(", ");
    }

    if (array->count > 0) {
        print_value(array->values[array->count - 1]);
    }

    putchar(']');
}

static void print_map(RavMap *map) {
    putchar('{');

    Entry *entries = map->table.entries;
    int last_index = map->table.hash_mask;

    for (int i = 0; i < last_index; i++) {
        if (entries[i].key == NULL) {
            continue;
        }

        print_object(Obj_Value(entries[i].key));
        printf(": ");
        print_value(entries[i].value);
        printf(", ");
    }

    if (last_index >= 0) {
        print_object(Obj_Value(entries[last_index].key));
        printf(": ");
        print_value(entries[last_index].value);
    }

    putchar('}');
}

static void print_function(RavFunction *function) {
    if (function->name == NULL) {
        printf("<top-level>");
        return;
    }

    printf("<fn (%d) %s>", function->arity, function->name->chars);
}

static void print_cfunction(RavCFunction *function) {
    printf("<native (%d) @ %p>", function->arity, function->func);
}

void print_object(Value value) {
    switch (Obj_Type(value)) {
    case OBJ_STRING:
        printf("'%s'", As_CString(value));
        break;

    case OBJ_PAIR:
        putchar('(');
        print_pair(As_Pair(value));
        putchar(')');
        break;

    case OBJ_ARRAY:
        print_array(As_Array(value));
        break;

    case OBJ_MAP:
        print_map(As_Map(value));
        break;

    case OBJ_FUNCTION:
        print_function(As_Function(value));
        break;

    case OBJ_UPVALUE:
        printf("<upvalue>");
        break;

    case OBJ_CLOSURE:
        print_function(As_Closure(value)->function);
        break;

    case OBJ_CFUNCTION:
        print_cfunction(As_CFunction(value));
        break;

    default:
        assert(!"invalid object type");
    }
}
