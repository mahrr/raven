#include <string.h>
#include <stdio.h>

#include "common.h"
#include "mem.h"
#include "object.h"
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

static ObjString *alloc_string(VM *vm, int length, uint32_t hash,
                               char *chars) {
    ObjString *string = Alloc_Object(vm, ObjString, OBJ_STRING);
    string->length = length;
    string->hash = hash;
    string->chars = chars;

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

ObjString *copy_string(VM *vm, const char *chars, int length) {
    char *copy = Alloc(char, length + 1);
    memcpy(copy, chars, length);
    copy[length] = '\0';

    uint32_t hash = hash_string(chars, length);
    return alloc_string(vm, length, hash, copy);
}

ObjString *box_string(VM *vm, char *chars, int length) {
    uint32_t hash = hash_string(chars, length);

    return alloc_string(vm, length, hash, chars);
}

void print_object(Value value) {
    switch (Obj_Type(value)) {
    case OBJ_STRING: printf("%s", As_CString(value)); break;
    default:
        assert(0);
    }
}

static void free_object(Object *object) {
    switch (object->type) {
    case OBJ_STRING: {
        ObjString *string = (ObjString *)object;
        Free_Array(string->chars, char, string->length + 1);
        Free(ObjString, string);
        break;
    }

    default:
        assert(0);
    }
}

void free_objects(Object *objects) {
    while (objects) {
        Object *next = objects->next;
        free_object(objects);
        objects = next;
    }
}
