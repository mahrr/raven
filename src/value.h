#ifndef raven_value_h
#define raven_value_h

// Raven value representation

#include "common.h"

typedef enum {
    VALUE_INT,
    VALUE_REAL,
    VALUE_BOOL,
    VALUE_NIL,
} ValueType;

typedef struct {
    ValueType type;
    union {
        int64_t integer;
        double real;
        bool boolean;
    } as;
} Value;

#define IS_INT(value)  ((value).type == VALUE_INT)
#define IS_REAL(value) ((value).type == VALUE_REAL)
#define IS_BOOL(value) ((value).type == VALUE_BOOL)
#define IS_NIL(value)  ((value).type == VALUE_NIL)

#define AS_INT(value)  ((value).as.integer)
#define AS_REAL(value) ((value).as.real)
#define AS_BOOL(value) ((value).as.boolean)

#define INT_VALUE(integer)  ((Value){ VALUE_INT, { .integer = integer }})
#define REAL_VALUE(real)    ((Value){ VALUE_REAL, { .real = real }})
#define BOOL_VALUE(boolean) ((Value){ VALUE_BOOL, { .boolean = boolean }})
#define NIL_VALUE           ((Value){ VALUE_NIL, { .integer = 0 }})

typedef struct {
    int count;
    int capacity;
    Value *values;
} ValueBuffer;

// Initialize the values buffer state.
void init_values(ValueBuffer *buffer);

// Free the memory used by values buffer.
void free_values(ValueBuffer *buffer);

// Add a value element to the values buffer.
void push_value(ValueBuffer *buffer, Value value);

// Check if two values are equal.
bool values_equal(Value x, Value y);

void print_value(Value value);

#endif
