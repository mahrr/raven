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
    Free_Array(array->values, Value, array->capacity);
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
    switch (value.type) {
    case VALUE_NUM:  printf("%g", As_Num(value)); break;
    case VALUE_BOOL: printf(As_Bool(value) ? "true" : "false"); break;
    case VALUE_NIL:  printf("nil"); break;
    case VALUE_OBJ:  print_object(value); break;
    default:
        assert(0);
    }
}
