#include <stdio.h>

#include "common.h"
#include "mem.h"
#include "object.h"
#include "value.h"

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
    case VALUE_NIL:
    case VALUE_VOID: return true;
    case VALUE_OBJ:  return As_Obj(x) == As_Obj(y);
    }

    assert(!"invalid value type");
    return false; // For warnings
#endif
}
