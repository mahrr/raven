#ifndef raven_value_h
#define raven_value_h

// Raven value representation

#include "common.h"

typedef enum {
    VALUE_NUM,
    VALUE_BOOL,
    VALUE_NIL,
} ValueType;

typedef struct {
    ValueType type;
    union {
        double number;
        bool boolean;
    } as;
} Value;

#define IS_NUM(value) ((value).type == VALUE_NUM)
#define IS_BOOL(value) ((value).type == VALUE_BOOL)
#define IS_NIL(value)  ((value).type == VALUE_NIL)

#define AS_NUM(value)  ((value).as.number)
#define AS_BOOL(value) ((value).as.boolean)

#define NUM_VALUE(value)  ((Value){ VALUE_NUM, { .number = value }})
#define BOOL_VALUE(value) ((Value){ VALUE_BOOL, { .boolean = value }})
#define NIL_VALUE         ((Value){ VALUE_NIL, { .number = 0 }})

typedef struct {
    int count;
    int capacity;
    Value *values;
} ValueArray;

// Initialize the values array state.
void init_values(ValueArray *array);

// Free the memory used by values array.
void free_values(ValueArray *array);

// Add a value element to the values array.
void push_value(ValueArray *array, Value value);

// Check if two values are equal.
bool values_equal(Value x, Value y);

void print_value(Value value);

#endif
