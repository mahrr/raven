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

#define Is_Num(value)  ((value).type == VALUE_NUM)
#define Is_Bool(value) ((value).type == VALUE_BOOL)
#define Is_Nil(value)  ((value).type == VALUE_NIL)

#define As_Num(value)  ((value).as.number)
#define As_Bool(value) ((value).as.boolean)

#define Num_Value(value)  ((Value){ VALUE_NUM, { .number = value }})
#define Bool_Value(value) ((Value){ VALUE_BOOL, { .boolean = value }})
#define Nil_Value         ((Value){ VALUE_NIL, { .number = 0 }})

typedef struct {
    int count;
    int capacity;
    Value *values;
} ValueArray;

// Initialize the values array state.
void init_value_array(ValueArray *array);

// Free the memory used by values array.
void free_value_array(ValueArray *array);

// Add a value element to the values array.
void push_value(ValueArray *array, Value value);

void print_value(Value value);

#endif
