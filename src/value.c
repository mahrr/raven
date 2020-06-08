#include <stdio.h>

#include "common.h"
#include "mem.h"
#include "object.h"
#include "value.h"

void init_value_array(ValueArray *array) {
    array->count = 0;
    array->capacity = 0;
    array->values = NULL;
}

void free_value_array(ValueArray *array) {
    Free_Array(Value, array->values, array->capacity);
    init_value_array(array);
}

void push_value(ValueArray *array, Value value) {
    if (array->count == array->capacity) {
        int old_capacity = array->capacity;
        array->capacity = Grow_Capacity(old_capacity);
        array->values = Grow_Array(array->values, Value,
                                   old_capacity, array->capacity);
    }

    array->values[array->count++] = value;
}

void print_value(Value value) {
#ifdef NAN_TAGGING
    if (Is_Num(value)) {
        printf("%g", As_Num(value));
        return;
    }
    
    if (Is_Bool(value)) {
        printf(As_Bool(value) ? "true" : "false");
        return;
    }

    if (Is_Nil(value)) {
        printf("nil");
        return;
    }

    if (Is_Obj(value)) {
        print_object(value);
        return;
    }

    assert(!"invalid value type");
#else
    switch (value.type) {
    case VALUE_NUM:  printf("%g", As_Num(value)); break;
    case VALUE_BOOL: printf(As_Bool(value) ? "true" : "false"); break;
    case VALUE_NIL:  printf("nil"); break;
    case VALUE_OBJ:  print_object(value); break;
    default:
        assert(!"invalid value type");
    }
#endif
}

bool equal_values(Value x, Value y) {
#ifdef NAN_TAGGING
    return x == y;
#else
    if (x.type != y.type) return false;

    switch (x.type) {
    case VALUE_NUM:  return As_Num(x) == As_Num(y);
    case VALUE_BOOL: return As_Bool(x) == As_Bool(y);
    case VALUE_NIL:  return true;
    case VALUE_OBJ:  return As_Obj(x) == As_Obj(y);
    }

    assert(!"invalid value type");
    return false; // For warnings
#endif
}
