#include <assert.h>
#include <stdio.h>

#include "common.h"
#include "mem.h"
#include "value.h"

void init_values(ValueBuffer *buffer) {
    buffer->count = 0;
    buffer->capacity = 0;
    buffer->values = NULL;
}

void free_values(ValueBuffer *buffer) {
    FREE_ARRAY(buffer->values, Value, buffer->capacity);
    init_values(buffer);
}

void push_value(ValueBuffer *buffer, Value value) {
    if (buffer->count == buffer->capacity) {
        int old_capacity = buffer->capacity;
        buffer->capacity = GROW_CAPACITY(old_capacity);
        buffer->values = GROW_ARRAY(buffer->values, Value,
                                    old_capacity,
                                    buffer->capacity);
    }

    buffer->values[buffer->count++] = value;
}

bool values_equal(Value x, Value y) {
    if (x.type != y.type) return false;
    
    switch (x.type) {
    case VALUE_INT:  return AS_INT(x) == AS_INT(y);
    case VALUE_REAL: return AS_REAL(x) == AS_REAL(y);
    case VALUE_BOOL: return AS_BOOL(x) == AS_BOOL(y);
    case VALUE_NIL:  return true;
    }

    assert(0);
}

void print_value(Value value) {
    switch (value.type) {
    case VALUE_INT:  printf("%ld", AS_INT(value)); break;
    case VALUE_REAL: printf("%f", AS_REAL(value)); break;
    case VALUE_BOOL: printf(AS_BOOL(value) ? "true" : "false"); break;
    case VALUE_NIL:  printf("nil"); break;
    }

    assert(0);
}
