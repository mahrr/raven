#ifndef raven_object_h
#define raven_object_h

// Raven Heap Object Representation

#include "common.h"
#include "value.h"
#include "vm.h"

typedef enum {
    OBJ_STRING,
} ObjectType;

// The header (metadata) of all objects.
struct Object {
    ObjectType type;
    struct Object *next;
};

struct ObjString {
    Object header;
    int length;
    uint32_t hash;
    char *chars;
};

#define Obj_Type(value) (As_Obj(value)->type)

#define Is_String(value) (is_objet_type(value, OBJ_STRING))

#define As_String(value)  ((ObjString *)As_Obj(value))
#define As_CString(value) ((As_String(value))->chars)


// Note the first parameter of constructing functions is a vm
// instance, that's because all the system allocated objects
// are chained together with intrusive linked list, and the
// head of that list is stored in a vm state. A better approach
// would to store that list in some sort of an allocator state,
// but I stick with the simple approach for the time being.

// Construct an ObjString using a copy of the given string.
ObjString *copy_string(VM *vm, const char *chars, int length);

// Construct an ObjString with the given string. the ObjString
// will have the ownership of the chars memory.
ObjString *box_string(VM *vm, char *chars, int length);

void print_object(Value value);

// Free an intrusive linked list of objects.
void free_objects(Object *objects);

// Check if a given raven value is an object with specified type.
static inline bool is_object_type(Value value, ObjectType type) {
    return Is_Obj(value) && Obj_Type(value) == type;
}

#endif
