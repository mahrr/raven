#ifndef raven_value_h
#define raven_value_h

// Raven Value Representation

#include "common.h"

// Forward declarations to avoid cyclic include
// problem with 'object.h'.
typedef struct Object Object;
typedef struct RavString RavString;
typedef struct RavPair RavPair;
typedef struct RavArray RavArray;
typedef struct RavMap RavMap;
typedef struct RavFunction RavFunction;
typedef struct RavUpvalue RavUpvalue;
typedef struct RavClosure RavClosure;
typedef struct RavCFunction RavCFunction;

#ifdef NAN_TAGGING

#include <string.h>

// TODO: short strings optimization

//
// IEEE 754 64-bit double-precision IEEE floating-point representation:
//
//      NaN bits    Quiet bit / Intel FP bit            Our Space to Use
//  <  -----------  --<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
// [.][...........][....................................................]
//  |          |                            |
//  V          V                            V
//  sign bit   11-bit exponent              52-bit mantissa
//

typedef uint64_t Value;

// Cannot use *(Value *)(&number), since it breaks strict aliasing rules.
static inline Value value_from_number(double number) {
    Value value;
    memcpy(&value, &number, sizeof (Value));
    return value;
}

static inline double number_from_value(Value value) {
    double number;
    memcpy(&number, &value, sizeof (double));
    return number;
}

// Exponent bits + Quiet NaN bit + Intel FP bit
#define QNaN ((uint64_t)0x7ffc000000000000)

// Sign Bit
#define SB ((uint64_t)0x8000000000000000)

#define TAG_NIL   1
#define TAG_FALSE 2
#define TAG_TRUE  3
#define TAG_VOID  4

#define Is_Num(value)  (((value) & QNaN) != QNaN)
#define Is_Bool(value) (((value) & False_Value) == False_Value)
#define Is_Nil(value)  ((value) == Nil_Value)
#define Is_Void(value) ((value) == Void_Value)
#define Is_Obj(value)  (((value) & (SB | QNaN)) == (SB | QNaN))

#define As_Num(value)  (number_from_value(value))
#define As_Bool(value) ((value) == True_Value)
#define As_Obj(value)  ((Object *)(uintptr_t)((value) & ~(SB | QNaN)))

#define Num_Value(value)  (value_from_number(value))
#define Bool_Value(value) ((Value)(QNaN | (value) | 2))
#define True_Value        ((Value)(QNaN | TAG_TRUE))
#define False_Value       ((Value)(QNaN | TAG_FALSE))
#define Nil_Value         ((Value)(QNaN | TAG_NIL))
#define Void_Value        ((Value)(QNaN | TAG_VOID))
#define Obj_Value(value)  ((Value)(SB | QNaN | (uint64_t)(uintptr_t)(value)))

#else

typedef enum {
    VALUE_NUM,
    VALUE_BOOL,
    VALUE_NIL,
    VALUE_VOID,  // Used only internally
    VALUE_OBJ,
} ValueType;

typedef struct {
    ValueType type;
    union {
        double number;
        bool boolean;
        Object *object; // Heap allocated
    } as;
} Value;

#define Is_Num(value)  ((value).type == VALUE_NUM)
#define Is_Bool(value) ((value).type == VALUE_BOOL)
#define Is_Nil(value)  ((value).type == VALUE_NIL)
#define Is_Void(value) ((value).type == VALUE_VOID)
#define Is_Obj(value)  ((value).type == VALUE_OBJ)

#define As_Num(value)  ((value).as.number)
#define As_Bool(value) ((value).as.boolean)
#define As_Obj(value)  ((value).as.object)

#define Num_Value(value)  ((Value){ VALUE_NUM, { .number = value }})
#define Bool_Value(value) ((Value){ VALUE_BOOL, { .boolean = value }})
#define Nil_Value         ((Value){ VALUE_NIL, { .number = 0 }})
#define Void_Value        ((Value){ VALUE_VOID, { .number = 0 }})
#define Obj_Value(value)  ((Value){ VALUE_OBJ, { .object = (Object *)value }})

#endif // NAN_TAGGING

void print_value(Value value);

bool equal_values(Value x, Value y);

#endif
