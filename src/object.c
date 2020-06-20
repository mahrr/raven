#include <string.h>
#include <stdio.h>

#include "common.h"
#include "chunk.h"
#include "table.h"
#include "mem.h"
#include "object.h"
#include "value.h"
#include "vm.h"

#define Alloc_Object(vm, struct_type, object_type)                      \
    (struct_type *)alloc_object(vm, object_type, sizeof (struct_type))

static Object *alloc_object(VM *vm, ObjectType type, size_t size) {
    Object *object = (Object *)allocate(NULL, 0, size);
    object->type = type;
    object->next = vm->objects;

    vm->objects = object;
    return object;
}

static RavString *alloc_string(VM *vm, int length, uint32_t hash,
                               char *chars) {
    RavString *string = Alloc_Object(vm, RavString, OBJ_STRING);
    string->length = length;
    string->hash = hash;
    string->chars = chars;

    table_set(&vm->strings, string, Nil_Value);
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

RavString *new_string(VM *vm, const char *chars, int length) {
    uint32_t hash = hash_string(chars, length);
    RavString *interned = table_interned(&vm->strings, chars,
                                         hash, length);

    if (interned != NULL) return interned;

    char *copy = Alloc(char, length + 1);
    memcpy(copy, chars, length);
    copy[length] = '\0';

    return alloc_string(vm, length, hash, copy);
}

RavString *box_string(VM *vm, char *chars, int length) {
    uint32_t hash = hash_string(chars, length);
    RavString *interned = table_interned(&vm->strings, chars,
                                         hash, length);

    // box_string takes ownership of chars memory, so if
    // the string is already interned, it frees this memory,
    // as it's no longer needed.
    if (interned != NULL) {
        Free_Array(char, chars, length + 1);
        return interned;
    }

    return alloc_string(vm, length, hash, chars);
}

RavFunction *new_function(VM *vm) {
    RavFunction *function = Alloc_Object(vm, RavFunction, OBJ_FUNCTION);

    function->name = NULL;
    function->arity = 0;
    init_chunk(&function->chunk);
    return function;
}

RavClosure *new_closure(VM *vm, RavFunction *function) {
    RavClosure *closure = Alloc_Object(vm, RavClosure, OBJ_CLOSURE);

    closure->function = function;
    return closure;
}

static void print_function(RavFunction *function) {
    if (function->name == NULL) {
        printf("<top-level>");
        return;
    }

    printf("<fn %s>", function->name->chars);
}

void print_object(Value value) {
    switch (Obj_Type(value)) {
    case OBJ_STRING:   printf("'%s'", As_CString(value)); break;
    case OBJ_FUNCTION: print_function(As_Function(value)); break;
    case OBJ_CLOSURE:  print_function(As_Closure(value)->function); break;
    default:
        assert(!"invalid object type");
    }    
}

static void free_object(Object *object) {
    switch (object->type) {
    case OBJ_STRING: {
        RavString *string = (RavString *)object;
        Free_Array(char, string->chars, string->length + 1);
        Free(RavString, string);
        break;
    }

    case OBJ_FUNCTION: {
        RavFunction *function = (RavFunction *)object;
        // TODO: manually free the name or leave it to the GC?
        free_chunk(&function->chunk);
        Free(RavFunction, function);
        break;
    }

    case OBJ_CLOSURE: {
        Free(RavClosure, object);
        break;
    }

    default:
        assert(!"invalid object type");
    }    
}

void free_objects(Object *objects) {
    while (objects) {
        Object *next = objects->next;
        free_object(objects);
        objects = next;
    }
}
