#include <stdio.h>

#include "common.h"
#include "mem.h"
#include "value.h"

void init_values(ValueArray *array) {
    array->count = 0;
    array->capacity = 0;
    array->values = NULL;
}

void free_values(ValueArray *array) {
    FREE_ARRAY(array->values, Value, array->capacity);
    init_values(array);
}

void push_value(ValueArray *array, Value value) {
    if (array->count == array->capacity) {
        int old_capacity = array->capacity;
        array->capacity = GROW_CAPACITY(old_capacity);
        array->values = GROW_ARRAY(array->values, Value,
                                    old_capacity, array->capacity);
    }

    array->values[array->count++] = value;
}

bool values_equal(Value x, Value y) {
    if (x.type != y.type) return false;
    
    switch (x.type) {
    case VALUE_NUM:  return AS_NUM(x) == AS_NUM(y);
    case VALUE_BOOL: return AS_BOOL(x) == AS_BOOL(y);
    case VALUE_NIL:  return true;
    default:
        assert(0);
    }
}

void print_value(Value value) {
    switch (value.type) {
    case VALUE_NUM:  printf("%g", AS_NUM(value)); break;
    case VALUE_BOOL: printf(AS_BOOL(value) ? "true" : "false"); break;
    case VALUE_NIL:  printf("nil"); break;
    default:
        assert(0);
    }
}
